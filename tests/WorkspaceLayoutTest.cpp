#include "main/NTRT.h"
#include "main/api/AbstractApi.h"
#include "main/ui/Workspace.h"
#include "main/ui/chart/ChartView.h"
#include "main/ui/panels/ChartPanel.h"
#include "main/ui/dialog/SettingsDialog.h"
#include "main/util/FileUtil.h"
#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMdiArea>
#include <QMouseEvent>
#include <QMdiSubWindow>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

namespace ntrt
{
    void quitApp() { QApplication::quit(); }
    void displayAbout(QWidget* parent) { Q_UNUSED(parent); }
}

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void settleLayout()
{
    for (int index = 0; index < 8; index++)
    {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
}

const trtdata::WorkspaceWindowState& findWindow(const trtdata::WorkspaceState& state, const QString& id)
{
    for (const trtdata::WorkspaceWindowState& window : state.windows)
    {
        if (window.id == id)
            return window;
    }
    throw std::runtime_error("missing saved window");
}

void saveLayout(trtui::MainWindow& mainWindow)
{
    for (QAction* action : mainWindow.findChildren<QAction*>())
    {
        if (action->text() == "Save Workspace Layout")
        {
            action->trigger();
            return;
        }
    }
    throw std::runtime_error("missing save action");
}

void savePreview(QWidget& widget, const QString& name)
{
    const QString directory = qEnvironmentVariable("NTRT_REVIEW_IMAGES");
    if (!directory.isEmpty())
    {
        require(QDir().mkpath(directory), "create preview directory");
        require(widget.grab().save(QDir(directory).filePath(name + ".png")), "save rendered preview");
    }
}

void verifyThemes(trtui::MainWindow& mainWindow, trtui::Workspace& workspace)
{
    trtui::SettingsDialog settings(&mainWindow);
    settings.show();
    QListWidget* categories = settings.findChild<QListWidget*>("settingsCategories");
    require(categories, "settings navigation");
    categories->setCurrentRow(1);
    QComboBox* theme = nullptr;
    for (QComboBox* control : settings.findChildren<QComboBox*>())
    {
        if (control->count() == 2 && control->itemText(0) == "Light" && control->itemText(1) == "Dark")
            theme = control;
    }
    require(theme, "theme setting");
    QMdiSubWindow* hidden = workspace.findChild<QMdiSubWindow*>("chart7");
    require(hidden, "retained hidden chart");
    const int themes[] = {0, 1, 0};
    int step = 0;
    for (int selection : themes)
    {
        hidden->hide();
        theme->setCurrentIndex(selection);
        settleLayout();
        hidden->show();
        workspace.createChartWindow();
        QMdiArea* desktop = workspace.findChild<QMdiArea*>("workspaceDesktop");
        QMdiSubWindow* created = desktop->activeSubWindow();
        require(created, "create themed child");
        trtui::ChartPanel* panel = created->findChild<trtui::ChartPanel*>();
        require(panel, "new chart content");
        panel->setInstrument({trtdata::Exchange::OKX, trttrade::InstType::Swap, "BTC-USDT-SWAP"});
        for (trtui::ChartView* view : workspace.findChildren<trtui::ChartView*>())
        {
            // Exercise OpenGL geometry with both rising and falling candles after palette changes.
            std::vector<trttrade::CandleData> data;
            for (int index = 0; index < 60; index++)
            {
                const double open = 1.2345 + index * 0.0001;
                data.push_back({1700000000000 + index * 60000LL, open, open + 0.0003, open - 0.0003,
                    open + (index % 2 == 0 ? 0.0002 : -0.0002), 10, 10, 10, true});
            }
            view->setCandles(data);
        }
        settleLayout();
        const QColor background = mainWindow.palette().color(QPalette::Window);
        require((background.lightness() < 128) == (selection == 1), "runtime theme palette");
        require(hidden->palette().color(QPalette::Window) == background, "hidden child follows theme");
        require(created->palette().color(QPalette::Window) == background, "new child follows theme");
        savePreview(*created, QString("new-chart-theme-%1").arg(step));
        created->hide();
        settleLayout();
        savePreview(mainWindow, QString("workspace-theme-%1").arg(step));
        QMdiSubWindow* list = workspace.findChild<QMdiSubWindow*>("instrumentList");
        savePreview(*list, QString("table-theme-%1").arg(step));
        savePreview(settings, QString("settings-theme-%1").arg(step));
        QMenu* menu = mainWindow.menuBar()->actions().front()->menu();
        menu->popup(mainWindow.mapToGlobal(QPoint(10, 30)));
        settleLayout();
        savePreview(*menu, QString("menu-theme-%1").arg(step));
        menu->hide();
        created->close();
        settleLayout();
        step++;
    }
    settings.close();
}

int main(int argc, char** argv)
{
    ntrt::app = std::make_shared<QApplication>(argc, argv);
    ntrt::app->setQuitOnLastWindowClosed(false);
    QApplication::setStyle("Fusion");
    try
    {
        QTemporaryDir directory;
        require(directory.isValid(), "temporary directory");
        ntrt::configManager = std::make_shared<trtdata::ConfigManager>(directory.path());
        ntrt::configManager->tryInitOnce();
        require(ntrt::configManager->loadAll(), "load isolated configuration");
        QFile stylesheet(trtutils::relativePath("res/style.qss"));
        require(stylesheet.open(QIODevice::ReadOnly), "load application stylesheet");
        ntrt::app->setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
        const std::shared_ptr<trtdata::Instrument> instrument = trtcore::exchangeApi(trtdata::Exchange::OKX).
            createInstrument("BTC-USDT-SWAP", trttrade::InstType::Swap);
        ntrt::configManager->addInstrument(*instrument);
        ntrt::dataMonitor = std::make_shared<trtcore::DataMonitor>(*ntrt::configManager);
        trtdata::WorkspaceConfig config;
        config.nextWorkspaceId = 10;
        trtdata::WorkspaceState first;
        first.id = "workspace4";
        first.name = QString::fromUtf8("布局测试");
        first.nextChartId = 8;
        first.activeWindow = "chart3";
        first.lastChart = "chart3";
        first.instruments.search = "BTC";
        first.instruments.typeFilter = 2;
        first.instruments.selected = instrument->getUniqueIdentifier();
        trtdata::WorkspaceWindowState list;
        list.id = "instrumentList";
        list.type = trtdata::WorkspaceWindowType::Instruments;
        list.geometry = QRect(20, 30, 420, 550);
        first.windows.push_back(list);
        trtdata::WorkspaceWindowState chart;
        chart.id = "chart6";
        chart.geometry = QRect(70, 80, 680, 430);
        chart.mode = trtdata::WorkspaceWindowMode::Minimized;
        chart.chart.instrument = instrument->getUniqueIdentifier();
        chart.chart.timeframe = trttrade::CandleTimeframe::Hour4;
        chart.chart.volume = false;
        first.windows.push_back(chart);
        chart.id = "chart7";
        chart.mode = trtdata::WorkspaceWindowMode::Normal;
        chart.visible = false;
        first.windows.push_back(chart);
        chart.id = "chart3";
        chart.geometry = QRect(460, 30, 700, 550);
        chart.visible = true;
        first.windows.push_back(chart);
        config.workspaces.push_back(first);
        trtdata::WorkspaceState second;
        second.id = "workspace2";
        second.name = "Background maximized";
        second.nextChartId = 6;
        second.activeWindow = "chart5";
        second.lastChart = "chart5";
        chart.id = "chart5";
        chart.geometry = QRect(90, 70, 720, 450);
        chart.mode = trtdata::WorkspaceWindowMode::Maximized;
        second.windows.push_back(chart);
        config.workspaces.push_back(second);
        trtdata::WorkspaceState empty;
        empty.id = "workspace8";
        empty.name = "Empty";
        config.workspaces.push_back(empty);
        require(ntrt::configManager->getWorkspaceManager()->saveConfig(config), "save layout fixture");
        {
            trtui::MainWindow mainWindow;
            mainWindow.show();
            settleLayout();
            QTabWidget* tabs = mainWindow.findChild<QTabWidget*>("workspaceTabs");
            require(tabs && tabs->count() == 3 && tabs->currentIndex() == 0, "tab order and selection");
            trtui::Workspace* workspace = static_cast<trtui::Workspace*>(tabs->widget(0));
            trtui::Workspace* background = static_cast<trtui::Workspace*>(tabs->widget(1));
            trtdata::WorkspaceState state = workspace->getState();
            require(state.activeWindow == "chart3", "restore active child");
            require(state.instruments.selected == first.instruments.selected, "restore list selection");
            require(state.instruments.search == "BTC" && state.instruments.typeFilter == 2, "restore filters");
            require(!findWindow(state, "chart7").visible, "retain hidden chart");
            require(findWindow(state, "chart6").mode == trtdata::WorkspaceWindowMode::Minimized, "restore minimized chart");
            require(findWindow(state, "chart6").geometry == QRect(70, 80, 680, 430), "minimized normal geometry");
            require(findWindow(background->getState(), "chart5").geometry == QRect(90, 70, 720, 450), "preserve hidden workspace normal geometry");
            tabs->setCurrentIndex(1);
            settleLayout();
            require(workspace->getState().activeWindow == "chart3", "retain inactive workspace selection");
            require(findWindow(background->getState(), "chart5").geometry == QRect(90, 70, 720, 450), "maximized geometry after first display");
            QMdiSubWindow* maximized = background->findChild<QMdiSubWindow*>("chart5");
            require(maximized && maximized->isMaximized(), "background chart is maximized");
            maximized->showNormal();
            settleLayout();
            require(maximized->geometry() == QRect(90, 70, 720, 450), "maximize to normal restores exact rectangle");
            tabs->setCurrentIndex(0);
            settleLayout();
            require(workspace->getState().activeWindow == "chart3", "restore active child when switching back");
            QMdiSubWindow* minimized = workspace->findChild<QMdiSubWindow*>("chart6");
            minimized->showNormal();
            settleLayout();
            require(minimized->geometry() == QRect(70, 80, 680, 430), "minimize to normal restores exact rectangle");
            minimized->setGeometry(80, 90, 690, 440);
            minimized->showMinimized();
            settleLayout();
            require(findWindow(workspace->getState(), "chart6").geometry == QRect(80, 90, 690, 440), "save moved and resized normal rectangle");
            QMdiSubWindow* hidden = workspace->findChild<QMdiSubWindow*>("chart7");
            hidden->show();
            settleLayout();
            require(findWindow(workspace->getState(), "chart7").visible, "hidden chart can be reopened");
            hidden->hide();
            settleLayout();
            require(!findWindow(workspace->getState(), "chart7").visible, "reopened chart can be hidden again");
            QMdiArea* area = workspace->findChild<QMdiArea*>("workspaceDesktop");
            const QPointF backgroundPoint(area->viewport()->width() - 2, 2);
            QMouseEvent backgroundClick(QEvent::MouseButtonPress, backgroundPoint, backgroundPoint,
                Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(area->viewport(), &backgroundClick);
            require(workspace->getState().activeWindow.isEmpty(), "background click clears persisted child selection");
            workspace->createChartWindow();
            settleLayout();
            QMdiSubWindow* created = workspace->findChild<QMdiSubWindow*>("chart8");
            require(created, "new chart identity does not collide");
            created->close();
            settleLayout();
            require(!workspace->findChild<QMdiSubWindow*>("chart8"), "closed charts are destroyed");
            QMdiArea* desktop = workspace->findChild<QMdiArea*>("workspaceDesktop");
            desktop->setActiveSubWindow(workspace->findChild<QMdiSubWindow*>("chart3"));
            tabs->setTabText(0, QString::fromUtf8("重命名"));
            tabs->tabBar()->moveTab(0, 1);
            tabs->setCurrentWidget(workspace);
            saveLayout(mainWindow);
            const trtdata::WorkspaceConfig& saved = ntrt::configManager->getWorkspaceManager()->getConfig();
            require(saved.currentWorkspace == 1 && saved.workspaces[1].id == "workspace4", "save reordered selected tab");
            require(saved.workspaces[1].name == QString::fromUtf8("重命名"), "save renamed tab");
            require(saved.workspaces[1].windows.size() == 4, "closed chart excluded from saved layout");
            mainWindow.close();
        }
        require(ntrt::configManager->getWorkspaceManager()->loadConfig(), "reload layout from disk");
        {
            trtui::MainWindow restored;
            restored.show();
            settleLayout();
            QTabWidget* tabs = restored.findChild<QTabWidget*>("workspaceTabs");
            require(tabs->count() == 3 && tabs->currentIndex() == 1, "restore reordered tabs after restart");
            trtui::Workspace* workspace = static_cast<trtui::Workspace*>(tabs->widget(1));
            require(workspace->getState().activeWindow == "chart3", "restart preserves active child");
            tabs->setCurrentIndex(2);
            settleLayout();
            require(static_cast<trtui::Workspace*>(tabs->widget(2))->getState().windows.empty(), "empty workspace stays empty");
            tabs->setCurrentIndex(1);
            settleLayout();
            verifyThemes(restored, *workspace);
            restored.close();
        }
        ntrt::dataMonitor.reset();
        ntrt::configManager.reset();
        std::cout << "workspace UI layout checks passed\n";
        return 0;
    } catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
