#include "DataMonitor.h"
#include "main/data/ConfigManager.h"

namespace trtcore
{
    class DataMonitor::Worker final
    {
        std::atomic<std::shared_ptr<const DataSnapshot>>& destination;
        QTimer publicationTimer;
        MarketData marketData;
        CandlesManager candlesManager;

    public:
        explicit Worker(std::atomic<std::shared_ptr<const DataSnapshot>>& destination) : destination(destination)
        {
            publicationTimer.setInterval(100);
            QObject::connect(&publicationTimer, &QTimer::timeout, &publicationTimer, [this]
            {
                publish();
            });
            publicationTimer.start();
        }

        ~Worker()
        {
            publicationTimer.stop();
            marketData.stop();
            candlesManager.stop();
        }

        void setInstruments(const std::vector<std::shared_ptr<trtdata::Instrument>>& instruments)
        {
            marketData.setInstruments(instruments);
            candlesManager.setInstruments(instruments);
            publish();
        }

        void setBookInstruments(const BookStream::Instruments& instruments)
        {
            marketData.setBookInstruments(instruments);
            publish();
        }

        void setTimeZone(trtdata::TradingDataTimeZone zone)
        {
            candlesManager.setTimeZone(zone);
            publish();
        }

        void acquireSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
        {
            candlesManager.subscribe(instrument, timeframe);
            publish();
        }

        void releaseSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
        {
            candlesManager.unsubscribe(instrument, timeframe);
            publish();
        }

        void requestHistory(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
        {
            candlesManager.requestHistory(instrument, timeframe);
            publish();
        }

        void publish()
        {
            destination.store(std::make_shared<const DataSnapshot>(DataSnapshot{
                marketData.getSnapshot(),
                candlesManager.getReadSnapshot()
            }));
        }
    };

    DataMonitor::DataMonitor(const trtdata::ConfigManager& configManager)
    : configManager(configManager), snapshot(std::make_shared<const DataSnapshot>())
    {
        workerThread.setObjectName("marketDataWorker");
        subscriptionTimer.setSingleShot(true);
        subscriptionTimer.setInterval(150);
        connect(&configManager, &trtdata::ConfigManager::tradingDataChanged, this, [this]
        {
            updateTimeZone();
        });
        connect(&configManager, &trtdata::ConfigManager::instrumentsChanged, this, [this]
        {
            if (running)
                subscriptionTimer.start();
        });
        connect(&subscriptionTimer, &QTimer::timeout, this, [this]
        {
            updateInstruments();
        });
    }

    DataMonitor::~DataMonitor()
    {
        stop();
    }

    void DataMonitor::start()
    {
        if (running)
            return;
        running = true;
        workerContext = new QObject();
        workerContext->moveToThread(&workerThread);
        connect(&workerThread, &QThread::finished, workerContext, &QObject::deleteLater);
        workerThread.start();
        const std::map<CandleSeriesKey, std::size_t> readers = chartReaders;
        QMetaObject::invokeMethod(workerContext, [this, readers]
        {
            worker = std::make_unique<Worker>(snapshot);
            for (const std::pair<const CandleSeriesKey, std::size_t>& entry : readers)
            {
                for (std::size_t index = 0; index < entry.second; index++)
                    worker->acquireSeries(entry.first.instrument, entry.first.timeframe);
            }
        }, Qt::QueuedConnection);
        updateTimeZone();
        updateInstruments();
        updateBookInstruments();
    }

    void DataMonitor::stop()
    {
        if (!running)
            return;
        running = false;
        subscriptionTimer.stop();
        instrumentIconManager.stop();
        // Drain commands and destroy sockets, timers and watchers on their owning thread.
        QMetaObject::invokeMethod(workerContext, [this]
        {
            worker.reset();
        }, Qt::BlockingQueuedConnection);
        workerThread.quit();
        workerThread.wait();
        workerContext = nullptr;
    }

    void DataMonitor::updateInstruments()
    {
        if (!running)
            return;
        const std::span<const std::shared_ptr<trtdata::Instrument>> configured = configManager.getInstruments();
        const std::vector<std::shared_ptr<trtdata::Instrument>> instruments(configured.begin(), configured.end());
        instrumentIconManager.setInstruments(instruments);
        QMetaObject::invokeMethod(workerContext, [this, instruments]
        {
            worker->setInstruments(instruments);
        }, Qt::QueuedConnection);
    }

    void DataMonitor::updateTimeZone()
    {
        if (!running)
            return;
        const trtdata::TradingDataTimeZone zone = configManager.getTradingDataTimeZone();
        QMetaObject::invokeMethod(workerContext, [this, zone]
        {
            worker->setTimeZone(zone);
        }, Qt::QueuedConnection);
    }

    void DataMonitor::acquireBook(const trtdata::InstIdentifier& instrument)
    {
        if (bookReaders[instrument]++ == 0)
            updateBookInstruments();
    }

    void DataMonitor::releaseBook(const trtdata::InstIdentifier& instrument)
    {
        const std::map<trtdata::InstIdentifier, std::size_t>::iterator found = bookReaders.find(instrument);
        if (found == bookReaders.end())
            return;
        found->second--;
        if (found->second == 0)
        {
            bookReaders.erase(found);
            updateBookInstruments();
        }
    }

    void DataMonitor::updateBookInstruments()
    {
        if (!running)
            return;
        BookStream::Instruments instruments;
        for (const std::pair<const trtdata::InstIdentifier, std::size_t>& entry : bookReaders)
            instruments.insert(entry.first);
        QMetaObject::invokeMethod(workerContext, [this, instruments]
        {
            worker->setBookInstruments(instruments);
        }, Qt::QueuedConnection);
    }

    void DataMonitor::acquireChartSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        chartReaders[{instrument, timeframe}]++;
        if (running)
            QMetaObject::invokeMethod(workerContext, [this, instrument, timeframe]
            {
                worker->acquireSeries(instrument, timeframe);
            }, Qt::QueuedConnection);
    }

    void DataMonitor::requestChartHistory(const trtdata::InstIdentifier& instrument,
                                          trttrade::CandleTimeframe timeframe)
    {
        if (running)
            QMetaObject::invokeMethod(workerContext, [this, instrument, timeframe]
            {
                worker->requestHistory(instrument, timeframe);
            }, Qt::QueuedConnection);
    }

    void DataMonitor::releaseChartSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        const std::map<CandleSeriesKey, std::size_t>::iterator entry = chartReaders.find({
            instrument,
            timeframe
        });
        if (entry == chartReaders.end())
            return;
        entry->second--;
        if (entry->second == 0)
            chartReaders.erase(entry);
        if (running)
            QMetaObject::invokeMethod(workerContext, [this, instrument, timeframe]
            {
                worker->releaseSeries(instrument, timeframe);
            }, Qt::QueuedConnection);
    }

    std::shared_ptr<const DataSnapshot> DataMonitor::getSnapshot() const
    {
        return snapshot.load();
    }

    std::shared_ptr<const MarketDataSnapshot> DataMonitor::getMarketData() const
    {
        const std::shared_ptr<const DataSnapshot> current = getSnapshot();
        return std::shared_ptr<const MarketDataSnapshot>(current, &current->marketData);
    }

    std::shared_ptr<const CandlesSnapshot> DataMonitor::getCandlesManager() const
    {
        const std::shared_ptr<const DataSnapshot> current = getSnapshot();
        return std::shared_ptr<const CandlesSnapshot>(current, &current->candles);
    }

    const InstrumentIconManager& DataMonitor::getInstrumentIconManager() const
    {
        return instrumentIconManager;
    }
}
