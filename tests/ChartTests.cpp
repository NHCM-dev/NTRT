#include "main/NTRT.h"
#include "main/api/AbstractApi.h"
#include "main/api/TickerStream.h"
#include "main/ui/Workspace.h"
#include "main/ui/panels/ChartPanel.h"
#include "main/ui/panels/InstrumentListPanel.h"
#include "main/ui/chart/ChartView.h"
#include "main/ui/chart/ChartLayer.h"
#include <QComboBox>
#include <QFile>
#include <QImage>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QOpenGLContext>
#include <QSignalSpy>
#include <QScreen>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTabWidget>
#include <QPointer>
#include <QStyle>
#include <QTabBar>
#include <QToolButton>
#include <QMenuBar>
#include <cmath>

class ChartTestInstrument final : public trtdata::Instrument
{
public:
    using Instrument::operator=;
    ChartTestInstrument(const std::string& id, trttrade::InstType type) : Instrument(id, type, trtdata::Exchange::OKX) {}
    QFuture<trtdata::TickerData> requestTicker() override { return {}; }
    std::string getInstTypeStr() const override { return "SPOT"; }
};

class ChartTestStream final : public trtcore::TickerStream
{
public:
    void setInstruments(const Instruments&) override {}
    void stop() override {}
};

class ChartTestApi final : public trtcore::AbstractApi
{
public:
    int getRequestInterval() const override { return 100; }
    std::vector<trttrade::InstType> instrumentTypes() const override { return {trttrade::InstType::Spot}; }
    std::unique_ptr<trtcore::TickerStream> createTickerStream() const override { return std::make_unique<ChartTestStream>(); }
    QFuture<std::shared_ptr<std::vector<trtdata::TickerData>>> fetchTickers(trttrade::InstType) const override { return {}; }
    QFuture<std::vector<std::shared_ptr<trtdata::Instrument>>> fetchInstruments(trttrade::InstType) const override { return {}; }
    std::shared_ptr<trtdata::Instrument> createInstrument(const std::string& id, trttrade::InstType type) const override
    {
        return std::make_shared<ChartTestInstrument>(id, type);
    }
    trttrade::InstType parseInstrumentType(std::string_view) const override { return trttrade::InstType::Spot; }
    QFuture<std::vector<trttrade::CandleData>> fetchCandles(const trtdata::InstIdentifier&, trttrade::CandleTimeframe,
        trtdata::TradingDataTimeZone, int) const override { return {}; }
};

namespace trtcore
{
    const AbstractApi& exchangeApi(trtdata::Exchange)
    {
        static const ChartTestApi api;
        return api;
    }
    std::vector<trtdata::Exchange> availableExchanges() { return {trtdata::Exchange::OKX}; }
}

namespace ntrt
{
    void quitApp() { QApplication::quit(); }
    void displayAbout(QWidget*) {}
}

class ChartTests final : public QObject
{
    Q_OBJECT

    QTemporaryDir directory;

    static std::vector<trttrade::CandleData> sampleCandles()
    {
        std::vector<trttrade::CandleData> result;
        for (int index = 0; index < 300; index++)
        {
            const double open = 100 + index * 0.1 + std::sin(index * 0.15) * 5;
            const double close = open + (index % 3 == 0 ? -1.4 : 1.6);
            result.push_back({1700000000000LL + index * 900000LL, open, std::max(open, close) + 1,
                std::min(open, close) - 1, close, 200.0 + index % 20 * 30, 0, 0, index != 299});
        }
        return result;
    }

    static QAction* findAction(QMenu& menu, const QString& text)
    {
        for (QAction* action : menu.actions())
        {
            if (action->text() == text)
                return action;
        }
        return nullptr;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(directory.isValid());
        QApplication::setStyle("Fusion");
        ntrt::configManager = std::make_shared<trtdata::ConfigManager>(directory.path());
        ntrt::configManager->tryInitOnce();
        QVERIFY(ntrt::configManager->loadAll());
        const std::vector<std::shared_ptr<trtdata::Instrument>> instruments{
            std::make_shared<ChartTestInstrument>("BTC-USDT", trttrade::InstType::Spot),
            std::make_shared<ChartTestInstrument>("ETH-USDT", trttrade::InstType::Spot),
            std::make_shared<ChartTestInstrument>("BTC-USDT-SWAP", trttrade::InstType::Swap)};
        QVERIFY(ntrt::configManager->replaceInstruments(instruments));
        ntrt::dataMonitor = std::make_shared<trtcore::DataMonitor>(*ntrt::configManager);
        trtdata::AppearanceManager::applyPalette(trtdata::EnumTheme::Dark);
        QFile stylesheet(QCoreApplication::applicationDirPath() + "/res/style.qss");
        QVERIFY(stylesheet.open(QIODevice::ReadOnly));
        qApp->setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
    }

    void viewportTransforms()
    {
        trtui::ChartViewport viewport;
        const std::vector<trttrade::CandleData> candles = sampleCandles();
        viewport.setSize(QSize(960, 540));
        viewport.setSeriesSize(candles.size());
        viewport.updateRange(candles);
        QVERIFY(viewport.isFollowing());
        QVERIFY(viewport.getLastVisible() == 300);
        QVERIFY(viewport.getPriceMin() < viewport.getPriceMax());
        QVERIFY(std::abs(viewport.indexFromX(viewport.xFromIndex(250)) - 250) < 1e-9);
        QVERIFY(std::abs(viewport.priceFromY(viewport.yFromPrice(125)) - 125) < 1e-9);
        const double anchorX = viewport.getPriceRect().center().x();
        const double anchor = viewport.indexFromX(anchorX);
        viewport.zoom(1.5, anchorX);
        QVERIFY(!viewport.isFollowing());
        QVERIFY(std::abs(viewport.indexFromX(anchorX) - anchor) < 1e-9);
        const double beforePan = viewport.indexFromX(anchorX);
        viewport.pan(48);
        QVERIFY(viewport.indexFromX(anchorX) < beforePan);
        const double beforeRollover = viewport.indexFromX(anchorX);
        viewport.setSeriesSize(300, 1);
        QVERIFY(std::abs(viewport.indexFromX(anchorX) - beforeRollover + 1) < 1e-9);
        viewport.pan(1e6);
        QCOMPARE(viewport.getFirstVisible(), std::size_t(0));
        const double leftLimit = viewport.indexFromX(viewport.getPriceRect().left());
        QVERIFY(leftLimit < -1 && leftLimit >= -41);
        viewport.pan(1e6);
        QCOMPARE(viewport.indexFromX(viewport.getPriceRect().left()), leftLimit);
        viewport.updateRange(candles);
        const double step = viewport.getPriceStep();
        QVERIFY(step > 0);
        for (double tick : viewport.getPriceTicks())
        {
            QVERIFY(std::abs(tick / step - std::round(tick / step)) < 1e-8);
            QVERIFY(tick >= viewport.getPriceMin() && tick <= viewport.getPriceMax());
        }
        viewport.pan(-1e6);
        QCOMPARE(viewport.getLastVisible(), std::size_t(300));
        viewport.followLatest();
        QVERIFY(viewport.isFollowing());
        viewport.setVolumeVisible(false, QSize(960, 540));
        QVERIFY(!viewport.isVolumeVisible());
        viewport.setSeriesSize(0);
        viewport.updateRange({});
        QVERIFY(std::isfinite(viewport.yFromPrice(0)));
        const std::vector<trttrade::CandleData> flat{{0, 0, 0, 0, 0, 0, 0, 0, true}};
        viewport.setSeriesSize(1);
        viewport.updateRange(flat);
        QVERIFY(viewport.getPriceMin() < viewport.getPriceMax());
        QVERIFY(std::isfinite(viewport.yFromVolume(0)));
    }

    void layerClipping()
    {
        trtui::ChartViewport viewport;
        const std::vector<trttrade::CandleData> candles = sampleCandles();
        viewport.setSize(QSize(960, 540));
        viewport.setSeriesSize(candles.size());
        viewport.updateRange(candles);
        const trtui::ChartLayerContext context{viewport, candles, QColor(0x4ade80), QColor(0xfb7185), 0.5};
        trtui::CandlestickLayer candlesLayer;
        std::vector<trtui::ChartVertex> vertices;
        candlesLayer.build(context, vertices);
        QVERIFY(!vertices.empty());
        QCOMPARE(vertices.size() % 6, std::size_t(0));
        for (const trtui::ChartVertex& vertex : vertices)
        {
            QVERIFY(viewport.getPriceRect().adjusted(-0.001, -0.001, 0.001, 0.001).contains(QPointF(vertex.x, vertex.y)));
            QVERIFY(std::isfinite(vertex.x) && std::isfinite(vertex.y));
        }
        trtui::VolumeLayer volume;
        vertices.clear();
        viewport.setVolumeVisible(false, QSize(960, 540));
        volume.build(context, vertices);
        QVERIFY(vertices.empty());
    }

    void nativeRendering()
    {
        trtui::ChartView view;
        QSignalSpy failure(&view, &trtui::ChartView::renderingFailed);
        QSignalSpy follow(&view, &trtui::ChartView::followChanged);
        view.resize(960, 540);
        view.setCandles(sampleCandles());
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QTRY_VERIFY(view.isValid());
        const QImage image = view.grabFramebuffer();
        QVERIFY2(failure.isEmpty(), failure.isEmpty() ? "" : qPrintable(failure.front().front().toString()));
        QVERIFY(!image.isNull());
        int rising = 0;
        int falling = 0;
        for (int y = 0; y < image.height(); y++)
        {
            for (int x = 0; x < image.width(); x++)
            {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.green() > 180 && pixel.red() < 100)
                    rising++;
                if (pixel.red() > 210 && pixel.green() < 140)
                    falling++;
            }
        }
        QVERIFY2(rising > 200 && falling > 200, "GPU framebuffer did not contain both candle colors");
        QVERIFY(image.save(QCoreApplication::applicationDirPath() + "/chart-test-dark.png"));
        view.makeCurrent();
        QCOMPARE(view.context()->functions()->glGetError(), GLenum(GL_NO_ERROR));
        view.doneCurrent();
        QTest::mouseMove(&view, QPoint(420, 220));
        QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, QPoint(500, 240));
        QTest::mouseMove(&view, QPoint(600, 240));
        QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, QPoint(600, 240));
        QVERIFY(!follow.isEmpty() && !follow.back().front().toBool());
        for (int index = 0; index < 40; index++)
            QTest::keyClick(&view, Qt::Key_Left);
        view.setHistoryState(true, false);
        QVERIFY(view.grabFramebuffer().save(QCoreApplication::applicationDirPath() + "/chart-loading-test.png"));
        view.setHistoryState(false, false);
        QTest::keyClick(&view, Qt::Key_End);
        QVERIFY(follow.back().front().toBool());
        trtdata::AppearanceManager::applyPalette(trtdata::EnumTheme::Light);
        QCoreApplication::processEvents();
        QVERIFY(view.grabFramebuffer().save(QCoreApplication::applicationDirPath() + "/chart-test-light.png"));
        trtdata::AppearanceManager::applyPalette(trtdata::EnumTheme::Dark);
    }

    void workspaceRouting()
    {
        trtui::Workspace workspace;
        QMenu menu;
        workspace.addWindowActions(&menu);
        workspace.resize(1500, 900);
        workspace.show();
        QVERIFY(QTest::qWaitForWindowExposed(&workspace));
        workspace.updateData();
        QMdiArea* desktop = workspace.findChild<QMdiArea*>();
        QMdiSubWindow* first = workspace.findChild<QMdiSubWindow*>("chart1");
        QVERIFY(desktop && first);
        trtui::ChartPanel* firstPanel = first->findChild<trtui::ChartPanel*>();
        QVERIFY(firstPanel && !firstPanel->getInstrument());
        QStackedWidget* stack = firstPanel->findChild<QStackedWidget*>();
        QCOMPARE(stack->currentIndex(), 0);
        QCoreApplication::processEvents();
        QVERIFY(workspace.grab().save(QCoreApplication::applicationDirPath() + "/workspace-empty-test.png"));
        QTableWidget* list = workspace.findChild<QTableWidget*>();
        QVERIFY(list && list->rowCount() == 3);
        const QPoint firstRow = list->visualItemRect(list->item(0, 3)).center();
        QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, firstRow);
        QTest::mouseDClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, firstRow);
        QVERIFY(firstPanel->getInstrument());
        QCOMPARE(firstPanel->getInstrument()->id, std::string("BTC-USDT"));
        workspace.createChartWindow();
        QMdiSubWindow* second = workspace.findChild<QMdiSubWindow*>("chart2");
        QVERIFY(second);
        trtui::ChartPanel* secondPanel = second->findChild<trtui::ChartPanel*>();
        QVERIFY(!secondPanel->getInstrument());
        QCOMPARE(desktop->activeSubWindow(), second);
        desktop->setActiveSubWindow(workspace.findChild<QMdiSubWindow*>("instrumentList"));
        QVERIFY(QMetaObject::invokeMethod(list, "cellDoubleClicked", Qt::DirectConnection, Q_ARG(int, 1), Q_ARG(int, 3)));
        QCOMPARE(secondPanel->getInstrument()->id, std::string("ETH-USDT"));
        QCOMPARE(firstPanel->getInstrument()->id, std::string("BTC-USDT"));
        desktop->setActiveSubWindow(first);
        QVERIFY(QMetaObject::invokeMethod(list, "cellDoubleClicked", Qt::DirectConnection, Q_ARG(int, 2), Q_ARG(int, 3)));
        QCOMPARE(firstPanel->getInstrument()->id, std::string("BTC-USDT-SWAP"));
        QComboBox* picker = secondPanel->findChild<QComboBox*>("chartInstrument");
        picker->setCurrentIndex(1);
        QVERIFY(QMetaObject::invokeMethod(picker, "activated", Qt::DirectConnection, Q_ARG(int, 1)));
        QCOMPARE(secondPanel->getInstrument()->id, std::string("BTC-USDT"));
        QAction* secondAction = findAction(menu, second->windowTitle());
        QVERIFY(secondAction && secondAction->isChecked());
        second->hide();
        QVERIFY(second->isHidden());
        QVERIFY(!secondAction->isChecked());
        secondAction->trigger();
        QVERIFY(!second->isHidden() && secondAction->isChecked());
        QVERIFY(secondPanel->isVisible());
        QCOMPARE(desktop->activeSubWindow(), second);
        QVERIFY(secondPanel->getInstrument());
        second->showMinimized();
        QVERIFY(QMetaObject::invokeMethod(list, "cellDoubleClicked", Qt::DirectConnection, Q_ARG(int, 1), Q_ARG(int, 3)));
        QVERIFY(!second->isMinimized());
        QVERIFY2(secondPanel->isVisible(), qPrintable(QString("Restored chart content: hidden=%1 geometry=%2,%3 %4x%5")
            .arg(secondPanel->isHidden()).arg(secondPanel->x()).arg(secondPanel->y()).arg(secondPanel->width()).arg(secondPanel->height())));
        QCOMPARE(secondPanel->getInstrument()->id, std::string("ETH-USDT"));
        QVERIFY(findAction(menu, "Stack Windows"));
        QVERIFY(findAction(menu, "Sort Window"));
        workspace.resetLayout();
        workspace.updateData();
        firstPanel->findChild<trtui::ChartView*>()->setCandles(sampleCandles());
        secondPanel->findChild<trtui::ChartView*>()->setCandles(sampleCandles());
        QTest::qWait(80);
        QVERIFY(firstPanel->isVisible());
        QVERIFY(secondPanel->isVisible());
        QVERIFY(secondPanel->findChild<trtui::ChartView*>()->isVisible());
        QVERIFY(firstPanel->findChild<trtui::ChartView*>()->isValid());
        QVERIFY(secondPanel->findChild<trtui::ChartView*>()->isValid());
        QVERIFY(workspace.screen()->grabWindow(workspace.winId()).save(QCoreApplication::applicationDirPath() + "/workspace-test.png"));
        QPointer<QMdiSubWindow> closed(second);
        QPointer<QAction> closedAction(secondAction);
        second->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(closed.isNull());
        QVERIFY(closedAction.isNull());
        QMdiSubWindow* listWindow = workspace.findChild<QMdiSubWindow*>("instrumentList");
        listWindow->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        workspace.updateData();
        workspace.showInstrumentList();
        QVERIFY(workspace.findChild<QMdiSubWindow*>("instrumentList"));
    }

    void workspaceTabs()
    {
        trtui::MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTabWidget* tabs = window.findChild<QTabWidget*>("workspaceTabs");
        QVERIFY(tabs);
        QCOMPARE(tabs->count(), 1);
        QWidget* first = tabs->currentWidget();
        QMdiSubWindow* maximized = first->findChild<QMdiSubWindow*>("chart1");
        maximized->showMaximized();
        QTest::qWait(60);
        QVERIFY(maximized->isMaximized());
        QCOMPARE(maximized->style()->styleHint(QStyle::SH_Workspace_FillSpaceOnMaximize, nullptr, maximized), 1);
        QVERIFY(window.screen()->grabWindow(window.winId()).save(QCoreApplication::applicationDirPath() + "/workspace-maximized-test.png"));
        maximized->showNormal();
        QVERIFY(!maximized->isMaximized());
        QToolButton* add = window.findChild<QToolButton*>("newWorkspaceButton");
        QVERIFY(add);
        QTest::mouseClick(add, Qt::LeftButton);
        QCOMPARE(tabs->count(), 2);
        QWidget* second = tabs->currentWidget();
        QVERIFY(first != second);
        QAction* newChart = nullptr;
        for (QAction* action : window.menuBar()->actions())
        {
            if (action->text() == "New Chart")
                newChart = action;
        }
        QVERIFY(newChart);
        newChart->trigger();
        QCOMPARE(second->findChildren<trtui::ChartPanel*>().size(), 2);
        QCOMPARE(first->findChildren<trtui::ChartPanel*>().size(), 1);
        tabs->tabBar()->moveTab(1, 0);
        QCOMPARE(tabs->widget(0), second);
        QCOMPARE(tabs->currentWidget(), second);
        tabs->setTabText(0, "Research");
        QCOMPARE(tabs->tabText(0), QString("Research"));
        QVERIFY(QMetaObject::invokeMethod(tabs, "tabCloseRequested", Qt::DirectConnection, Q_ARG(int, 0)));
        QCOMPARE(tabs->count(), 1);
        QCOMPARE(tabs->currentWidget(), first);
        QVERIFY(QMetaObject::invokeMethod(tabs, "tabCloseRequested", Qt::DirectConnection, Q_ARG(int, 0)));
        QCOMPARE(tabs->count(), 1);
        QVERIFY(tabs->currentWidget() != first);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTest::qWait(50);
        QVERIFY(window.screen()->grabWindow(window.winId()).save(QCoreApplication::applicationDirPath() + "/workspace-tabs-test.png"));
    }

    void cleanupTestCase()
    {
        ntrt::dataMonitor.reset();
        ntrt::configManager.reset();
    }
};

QTEST_MAIN(ChartTests)
#include "ChartTests.moc"
