#include "main/api/AbstractApi.h"
#include "DataMonitor.h"
#include "main/data/persistence/ConfigManager.h"

namespace trtcore
{
    DataMonitor::Worker::Worker(DataMonitor& monitor, const trtdata::AccountManager& accounts)
        : monitor(monitor), accountData(accounts)
    {
        accountTimer.setInterval(AccountSnapshot::refreshIntervalMs);
        accountTimer.setTimerType(Qt::PreciseTimer);
        QObject::connect(&accountTimer, &QTimer::timeout, &accountTimer, [this] { accountData.refresh(); });
        connect(&marketData, &MarketData::snapshotChanged, this, [this]
        {
            marketChanged = true;
            schedulePublication();
        });
        connect(&candlesManager, &CandlesManager::snapshotChanged, this, [this]
        {
            candlesChanged = true;
            schedulePublication();
        });
        connect(&accountData, &AccountData::snapshotChanged, this, [this]
        {
            accountChanged = true;
            schedulePublication();
        });
        connect(&instrumentIcons, &InstrumentIconManager::snapshotChanged, this, [this]
        {
            iconsChanged = true;
            schedulePublication();
        });
    }

    void DataMonitor::Worker::enableAccounts()
    {
        if (accountTimer.isActive()) return;
        accountData.refresh();
        accountTimer.start();
    }

    void DataMonitor::Worker::queryAccountHistory(const QString& id, const trttrade::AccountHistoryQuery& query, bool append)
    {
        accountData.queryHistory(id, query, append);
    }

    void DataMonitor::Worker::releaseAccountHistory(const QString& id) { accountData.releaseHistory(id); }

    void DataMonitor::queryAccountHistory(const QString& id, const trttrade::AccountHistoryQuery& query, bool append)
    {
        enableAccountMonitoring();
        if (running) QMetaObject::invokeMethod(workerContext, [this, id, query, append] { worker->queryAccountHistory(id, query, append); }, Qt::QueuedConnection);
    }

    void DataMonitor::releaseAccountHistory(const QString& id)
    {
        if (running) QMetaObject::invokeMethod(workerContext, [this, id] { worker->releaseAccountHistory(id); }, Qt::QueuedConnection);
    }

    void DataMonitor::enableAccountMonitoring()
    {
        if (accountsEnabled) return;
        accountsEnabled = true;
        if (running) QMetaObject::invokeMethod(workerContext, [this] { worker->enableAccounts(); }, Qt::QueuedConnection);
    }

    DataMonitor::Worker::~Worker()
    {
        instrumentIcons.stop();
        marketData.stop();
        candlesManager.stop();
    }

    void DataMonitor::Worker::setInstruments(const std::vector<std::shared_ptr<trtdata::Instrument>>& instruments)
    {
        marketData.setInstruments(instruments);
        instrumentIcons.setInstruments(instruments);
        candlesManager.setInstruments(instruments);
        marketChanged = candlesChanged = true;
        schedulePublication();
    }

    void DataMonitor::Worker::setBookInstruments(const BookStream::Instruments& instruments)
    {
        marketData.setBookInstruments(instruments);
        marketChanged = true;
        schedulePublication();
    }

    void DataMonitor::Worker::setTimeZone(trtdata::TradingTimeZone zone)
    {
        candlesManager.setTimeZone(zone);
        candlesChanged = true;
        schedulePublication();
    }

    void DataMonitor::Worker::acquireSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        candlesManager.subscribe(instrument, timeframe);
        candlesChanged = true;
        schedulePublication();
    }

    void DataMonitor::Worker::releaseSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        candlesManager.unsubscribe(instrument, timeframe);
        candlesChanged = true;
        schedulePublication();
    }

    void DataMonitor::Worker::requestHistory(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        candlesManager.requestHistory(instrument, timeframe);
        candlesChanged = true;
        schedulePublication();
    }

    void DataMonitor::Worker::schedulePublication()
    {
        if (publicationPending)
            return;
        publicationPending = true;
        QMetaObject::invokeMethod(this, &Worker::publish, Qt::QueuedConnection);
    }

    void DataMonitor::Worker::publish()
    {
        publicationPending = false;
        if (marketChanged)
        {
            const std::shared_ptr<const MarketDataSnapshot> next = std::make_shared<const MarketDataSnapshot>(marketData.getSnapshot());
            candlesChanged |= next->getTickerRevision() != published.marketData->getTickerRevision();
            published.marketData = next;
        }
        if (candlesChanged)
            published.candles = candlesManager.getReadSnapshot(*published.marketData);
        if (accountChanged)
            published.account = std::make_shared<const AccountSnapshot>(accountData.getSnapshot());
        if (iconsChanged)
            published.icons = std::make_shared<const QHash<QString, QImage>>(instrumentIcons.getImages());
        marketChanged = candlesChanged = accountChanged = iconsChanged = false;
        monitor.publishSnapshot(published);
    }

    void DataMonitor::publishSnapshot(const DataSnapshot& data)
    {
        snapshot.store(std::make_shared<const DataSnapshot>(data));
        if (notificationPending.exchange(true))
            return;
        QMetaObject::invokeMethod(this, [this]
        {
            notificationPending.store(false);
            if (running)
                emit snapshotChanged();
        }, Qt::QueuedConnection);
    }

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
        const bool enableAccounts = accountsEnabled;
        QMetaObject::invokeMethod(workerContext, [this, readers, enableAccounts]
        {
            worker = std::make_unique<Worker>(*this, *configManager.getAccountManager());
            if (enableAccounts) worker->enableAccounts();
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
        QMetaObject::invokeMethod(workerContext, [this, instruments]
        {
            worker->setInstruments(instruments);
        }, Qt::QueuedConnection);
    }

    void DataMonitor::updateTimeZone()
    {
        if (!running)
            return;
        const trtdata::TradingTimeZone zone = configManager.getTradingDataTimeZone();
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
        return getSnapshot()->marketData;
    }

    std::shared_ptr<const CandlesSnapshot> DataMonitor::getCandlesManager() const
    {
        return getSnapshot()->candles;
    }

    void DataMonitor::Worker::requestCatalog(trtdata::Exchange exchange, trttrade::InstType type)
    {
        marketData.requestCatalog(exchange, type);
        marketChanged = true;
        schedulePublication();
    }
}

void trtcore::DataMonitor::requestCatalog(trtdata::Exchange exchange, trttrade::InstType type)
{
    if (running) QMetaObject::invokeMethod(workerContext, [this, exchange, type] { worker->requestCatalog(exchange, type); }, Qt::QueuedConnection);
}
std::vector<trttrade::InstType> trtcore::DataMonitor::getInstrumentTypes(trtdata::Exchange exchange) const { return exchangeApi(exchange).instrumentTypes(); }
std::vector<trtdata::Exchange> trtcore::DataMonitor::getExchanges() const { return availableExchanges(); }
