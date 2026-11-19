#include "ChartPanel.h"
#include "main/util/TimeUtil.h"
#include "main/util/QtUtil.h"
#include <QMdiSubWindow>
#include "main/util/TradeUtil.h"
#include "main/NTRT.h"
#include "main/ui/chart/ChartView.h"
#include <QComboBox>
#include <QCompleter>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>

namespace trtui
{
    ChartPanel::ChartPanel(QWidget* parent) : QFrame(parent), monitor(ntrt::dataMonitor)
    {
        setObjectName("chartPanel");
        setMinimumSize(460, 320);
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        initToolbar(layout);
        initContent(layout);
        initStatus(layout);
        displayDeadline.setSingleShot(true);
        displayDeadline.setTimerType(Qt::PreciseTimer);
        connect(&displayDeadline, &QTimer::timeout, this, [this]
        {
            const QMdiSubWindow* subWindow = qobject_cast<QMdiSubWindow*>(parentWidget());
            if (isVisible() && !window()->isMinimized() && (!subWindow || !subWindow->isMinimized()))
                updateData();
        });
        rebuildInstruments();
        connect(ntrt::configManager.get(), &trtdata::ConfigManager::instrumentsChanged, this, [this]
        {
            rebuildInstruments();
        });
    }

    ChartPanel::~ChartPanel()
    {
        releaseSeries();
    }

    void ChartPanel::initToolbar(QVBoxLayout* layout)
    {
        QWidget* toolbar = new QWidget(this);
        toolbar->setObjectName("chartToolbar");
        QHBoxLayout* controls = new QHBoxLayout(toolbar);
        controls->setContentsMargins(8, 6, 8, 6);
        instrumentBox = new QComboBox(toolbar);
        instrumentBox->setObjectName("chartInstrument");
        instrumentBox->setEditable(true);
        instrumentBox->setInsertPolicy(QComboBox::NoInsert);
        instrumentBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        instrumentBox->setMinimumWidth(140);
        instrumentBox->completer()->setFilterMode(Qt::MatchContains);
        instrumentBox->completer()->setCompletionMode(QCompleter::PopupCompletion);
        instrumentBox->completer()->setCaseSensitivity(Qt::CaseInsensitive);
        controls->addWidget(instrumentBox, 1);
        timeframeBox = new QComboBox(toolbar);
        timeframeBox->setObjectName("chartTimeframe");
        const std::pair<const char*, trttrade::CandleTimeframe> frames[] = {
            {"1m", trttrade::CandleTimeframe::Minute1},
            {"3m", trttrade::CandleTimeframe::Minute3},
            {"5m", trttrade::CandleTimeframe::Minute5},
            {"15m", trttrade::CandleTimeframe::Minute15},
            {"30m", trttrade::CandleTimeframe::Minute30},
            {"1h", trttrade::CandleTimeframe::Hour1},
            {"2h", trttrade::CandleTimeframe::Hour2},
            {"4h", trttrade::CandleTimeframe::Hour4},
            {"1D", trttrade::CandleTimeframe::Day1},
            {"1W", trttrade::CandleTimeframe::Week1},
            {"1M", trttrade::CandleTimeframe::Month1}
        };
        for (const std::pair<const char*, trttrade::CandleTimeframe>& frame : frames)
            timeframeBox->addItem(frame.first, static_cast<int>(frame.second));
        timeframeBox->setCurrentIndex(3);
        controls->addWidget(timeframeBox);
        QToolButton* volume = new QToolButton(toolbar);
        volumeButton = volume;
        volume->setText("Vol");
        volume->setCheckable(true);
        volume->setChecked(true);
        volume->setToolTip("Show candle volume in exchange units (base currency for spot, contracts for derivatives)");
        controls->addWidget(volume);
        followButton = new QToolButton(toolbar);
        followButton->setObjectName("chartFollow");
        followButton->setText("Latest");
        followButton->setCheckable(true);
        followButton->setChecked(true);
        followButton->setToolTip("Return to the latest candle and follow new bars (End)");
        controls->addWidget(followButton);
        QToolButton* reset = new QToolButton(toolbar);
        reset->setText("Reset");
        reset->setToolTip("Reset zoom and follow latest (Home or double-click chart)");
        controls->addWidget(reset);
        QToolButton* history = new QToolButton(toolbar);
        history->setText("History");
        controls->addWidget(history);
        connect(history, &QToolButton::clicked, this, [this]
        {
            const std::shared_ptr<trtcore::DataMonitor> backend = monitor.lock();
            if (backend && instrument)
                backend->requestChartHistory(*instrument, timeframe);
        });
        layout->addWidget(toolbar);
        connect(instrumentBox, &QComboBox::activated, this, [this](int index)
        {
            if (index > 0 && index <= static_cast<int>(choices.size()))
                setInstrument(choices[static_cast<std::size_t>(index - 1)]);
            else
                rebuildInstruments();
        });
        connect(timeframeBox, &QComboBox::activated, this, [this](int)
        {
            setTimeframe(static_cast<trttrade::CandleTimeframe>(timeframeBox->currentData().toInt()));
        });
        connect(volume, &QToolButton::toggled, this, [this](bool visible)
        {
            chart->setVolumeVisible(visible);
        });
        connect(followButton, &QToolButton::clicked, this, [this]
        {
            chart->followLatest();
        });
        connect(reset, &QToolButton::clicked, this, [this]
        {
            chart->resetView();
        });
    }

    void ChartPanel::initContent(QVBoxLayout* layout)
    {
        content = new QStackedWidget(this);
        QWidget* empty = new QWidget(content);
        empty->setObjectName("chartEmpty");
        QVBoxLayout* prompt = new QVBoxLayout(empty);
        prompt->setContentsMargins(24, 24, 24, 24);
        prompt->addStretch();
        QLabel* title = new QLabel("Choose an instrument", empty);
        title->setObjectName("chartEmptyTitle");
        title->setAlignment(Qt::AlignCenter);
        prompt->addWidget(title);
        QLabel* help = new QLabel(
            "Select an instrument above, or double-click one in the Instrument List.\nUse Elements → New Chart to compare instruments.",
            empty);
        help->setAlignment(Qt::AlignCenter);
        help->setWordWrap(true);
        prompt->addWidget(help);
        QPushButton* choose = new QPushButton("Choose instrument…", empty);
        prompt->addWidget(choose, 0, Qt::AlignHCenter);
        prompt->addStretch();
        content->addWidget(empty);
        chart = new ChartView(content);
        chart->setObjectName("chartView");
        content->addWidget(chart);
        layout->addWidget(content, 1);
        connect(chart, &ChartView::historyRequested, this, [this]
        {
            const std::shared_ptr<trtcore::DataMonitor> backend = monitor.lock();
            if (backend && instrument)
                backend->requestChartHistory(*instrument, timeframe);
        });
        connect(choose, &QPushButton::clicked, this, [this]
        {
            selectInstrument();
        });
        connect(chart, &ChartView::followChanged, this, [this](bool following)
        {
            followButton->setChecked(following);
        });
        connect(chart, &ChartView::renderingFailed, this, [this](const QString& error)
        {
            renderingError = error;
            updateData();
        });
    }

    void ChartPanel::initStatus(QVBoxLayout* layout)
    {
        status = new QLabel("Select an instrument to begin", this);
        status->setObjectName("chartStatus");
        status->setWordWrap(true);
        layout->addWidget(status);
    }

    void ChartPanel::rebuildInstruments()
    {
        const QSignalBlocker blocker(instrumentBox);
        choices.clear();
        instrumentBox->clear();
        instrumentBox->addItem("Choose instrument…");
        for (const std::shared_ptr<trtdata::Instrument>& tracked : ntrt::configManager->getInstruments())
        {
            choices.push_back(tracked->getUniqueIdentifier());
            instrumentBox->addItem(trtutils::instrumentLabel(choices.back()));
        }
        if (instrument)
        {
            const std::vector<trtdata::InstIdentifier>::const_iterator found = std::find(
                choices.cbegin(), choices.cend(), *instrument);
            if (found == choices.cend())
            {
                choices.push_back(*instrument);
                instrumentBox->addItem(trtutils::instrumentLabel(*instrument));
                instrumentBox->setCurrentIndex(static_cast<int>(choices.size()));
            } else
                instrumentBox->setCurrentIndex(static_cast<int>(found - choices.cbegin()) + 1);
        } else
            status->setText(choices.empty()
                                ? "Add an instrument with Elements → Add Instruments, then select it here."
                                : "Select an instrument to begin");
    }

    void ChartPanel::selectInstrument()
    {
        instrumentBox->setFocus();
        if (!choices.empty())
            instrumentBox->showPopup();
        else
            status->setText("No instruments yet. Use Elements → Add Instruments to track an instrument.");
    }

    void ChartPanel::releaseSeries()
    {
        const std::shared_ptr<trtcore::DataMonitor> backend = monitor.lock();
        if (backend && instrument)
            backend->releaseChartSeries(*instrument, timeframe);
    }

    void ChartPanel::setInstrument(const trtdata::InstIdentifier& identifier)
    {
        // identifier may refer to choices, which rebuildInstruments replaces.
        const trtdata::InstIdentifier selected = identifier;
        if (instrument&& * instrument == selected)
        {
            rebuildInstruments();
            return;
        }
        releaseSeries();
        instrument = selected;
        chart->setInstrument(selected);
        displayedUpdate = -1;
        chart->setCandles({});
        chart->resetView();
        const std::shared_ptr<trtcore::DataMonitor> backend = monitor.lock();
        if (backend)
            backend->acquireChartSeries(*instrument, timeframe);
        rebuildInstruments();
        content->setCurrentWidget(chart);
        updateData();
        emit titleChanged(getTitle());
    }

    void ChartPanel::setTimeframe(trttrade::CandleTimeframe value)
    {
        if (timeframe == value)
            return;
        releaseSeries();
        timeframe = value;
        displayedUpdate = -1;
        chart->setCandles({});
        chart->resetView();
        const std::shared_ptr<trtcore::DataMonitor> backend = monitor.lock();
        if (backend && instrument)
            backend->acquireChartSeries(*instrument, timeframe);
        updateData();
        emit titleChanged(getTitle());
    }

    const std::optional<trtdata::InstIdentifier>& ChartPanel::getInstrument() const
    {
        return instrument;
    }

    trtdata::ChartPanelState ChartPanel::getState() const
    {
        return {instrument, timeframe, volumeButton->isChecked()};
    }

    void ChartPanel::restoreState(const trtdata::ChartPanelState& state)
    {
        timeframeBox->setCurrentIndex(timeframeBox->findData(static_cast<int>(state.timeframe)));
        setTimeframe(state.timeframe);
        if (state.instrument)
            setInstrument(*state.instrument);
        volumeButton->setChecked(state.volume);
    }

    QString ChartPanel::getTitle() const
    {
        if (!instrument)
            return "Choose instrument";
        return trtutils::instrumentLabel(*instrument) + " · " + timeframeBox->currentText();
    }

    void ChartPanel::updateData()
    {
        displayDeadline.stop();
        if (!instrument)
            return;
        if (!renderingError.isEmpty())
        {
            status->show();
            status->setText("OpenGL unavailable: " + renderingError);
            status->setToolTip(renderingError);
            return;
        }
        const std::shared_ptr<trtcore::DataMonitor> backend = monitor.lock();
        if (!backend)
            return;
        const std::shared_ptr<const trtcore::CandlesSnapshot> data = backend->getCandlesManager();
        const trtcore::CandlesSnapshot& manager = *data;
        chart->setTimeZone(QTimeZone(QByteArray::fromStdString(ntrt::configManager->getDisplayTimeZone())));
        const trtcore::CandleSnapshot* snapshot = manager.getSnapshot(*instrument, timeframe);
        if (!snapshot)
            return;
        chart->setHistoryState(snapshot->historyLoading, snapshot->historyExhausted);
        if (displayedUpdate != snapshot->revision)
        {
            chart->setCandles(snapshot->candles);
            displayedUpdate = snapshot->revision;
        }
        const QString zone = QString::fromStdString(ntrt::configManager->getDisplayTimeZone());
        QString text;
        if (!snapshot->error.isEmpty())
        {
            text = snapshot->candles.empty()
                       ? "Unable to load candles · Retrying automatically"
                       : "Cached candles · Feed unavailable · Retrying automatically";
            chart->setMessage("Unable to load candles\n" + snapshot->error + "\nRetrying automatically");
        } else if (snapshot->candles.empty())
        {
            text = "Loading candles… · " + zone;
            chart->setMessage("Loading candles…");
        } else
        {
            const qint64 expiry = snapshot->updatedAt + trtcore::marketFeedTimeoutMs;
            if (snapshot->historyLoading)
                text = "Loading history…";
            if (expiry <= QDateTime::currentMSecsSinceEpoch())
                text = "Waiting for feed";
            else
                trtutils::scheduleDeadline(displayDeadline, expiry);
        }
        if (status->text() != text)
            status->setText(text);
        status->setVisible(!text.isEmpty());
        status->setToolTip(snapshot->error);
    }
}
