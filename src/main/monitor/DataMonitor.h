#ifndef NTRT_DATAMONITOR_H
#define NTRT_DATAMONITOR_H

#include "MarketData.h"
#include "CandlesManager.h"
#include "InstrumentIconManager.h"
#include <QThread>
#include <QTimer>
#include <atomic>

namespace trtdata
{
    class ConfigManager;
}

namespace trtcore
{
    struct DataSnapshot
    {
        MarketDataSnapshot marketData;
        CandlesSnapshot candles;
    };

    class DataMonitor final : public QObject
    {
        class Worker;
        const trtdata::ConfigManager& configManager;
        InstrumentIconManager instrumentIconManager;
        QTimer subscriptionTimer;
        QThread workerThread;
        QObject* workerContext = nullptr;
        std::unique_ptr<Worker> worker;
        std::atomic<std::shared_ptr<const DataSnapshot>> snapshot;
        std::map<CandlesManager::SeriesKey, std::size_t> chartReaders;
        bool running = false;

        void updateInstruments();

        void updateTimeZone();

    public:
        explicit DataMonitor(const trtdata::ConfigManager& configManager);

        ~DataMonitor() override;

        void start();

        void stop();

        void acquireChartSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

        void requestChartHistory(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

        void releaseChartSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

        // Retain the snapshot handle for as long as any borrowed pointers are used.
        std::shared_ptr<const DataSnapshot> getSnapshot() const;

        std::shared_ptr<const MarketDataSnapshot> getMarketData() const;

        std::shared_ptr<const CandlesSnapshot> getCandlesManager() const;

        const InstrumentIconManager& getInstrumentIconManager() const;
    };
}

#endif //NTRT_DATAMONITOR_H
