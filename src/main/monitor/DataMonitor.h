#ifndef NTRT_DATAMONITOR_H
#define NTRT_DATAMONITOR_H

#include "MarketData.h"
#include "DataSnapshotDef.h"
#include "CandlesManager.h"
#include "InstrumentIconManager.h"
#include <QThread>
#include <QTimer>
#include <atomic>
#include "main/data/ConfigManager.h"

namespace trtcore
{
    class DataMonitor final : public QObject
    {
        class Worker final
        {
            std::atomic<std::shared_ptr<const DataSnapshot>>& destination;
            QTimer publicationTimer;
            MarketData marketData;
            CandlesManager candlesManager;

        public:
            explicit Worker(std::atomic<std::shared_ptr<const DataSnapshot>>& destination);

            ~Worker();

            void setInstruments(const std::vector<std::shared_ptr<trtdata::Instrument>>& instruments);

            void setBookInstruments(const BookStream::Instruments& instruments);

            void setTimeZone(trtdata::TradingDataTimeZone zone);

            void acquireSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

            void releaseSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

            void requestHistory(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

            void publish();
        };

        const trtdata::ConfigManager& configManager;
        InstrumentIconManager instrumentIconManager;
        QTimer subscriptionTimer;
        QThread workerThread;
        QObject* workerContext = nullptr;
        std::unique_ptr<Worker> worker;
        std::atomic<std::shared_ptr<const DataSnapshot>> snapshot;
        std::map<CandleSeriesKey, std::size_t> chartReaders;
        std::map<trtdata::InstIdentifier, std::size_t> bookReaders;
        bool running = false;

        void updateBookInstruments();

        void updateInstruments();

        void updateTimeZone();

    public:
        explicit DataMonitor(const trtdata::ConfigManager& configManager);

        ~DataMonitor() override;

        void start();

        void stop();

        void acquireBook(const trtdata::InstIdentifier& instrument);

        void releaseBook(const trtdata::InstIdentifier& instrument);

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
