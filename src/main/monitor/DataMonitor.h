#ifndef NTRT_DATAMONITOR_H
#define NTRT_DATAMONITOR_H

#include "MarketData.h"
#include "AccountData.h"
#include "DataSnapshotDef.h"
#include "CandlesManager.h"
#include "InstrumentIconManager.h"
#include <QThread>
#include <QTimer>
#include <atomic>
#include "main/data/persistence/ConfigManager.h"

namespace trtcore
{
    class DataMonitor final : public QObject
    {
        Q_OBJECT

        class Worker final : public QObject
        {
            DataMonitor& monitor;
            DataSnapshot published;
            bool publicationPending = false;
            bool marketChanged = true;
            bool candlesChanged = true;
            bool accountChanged = true;
            bool iconsChanged = true;

            void schedulePublication();
            void publish();
            QTimer accountTimer;
            AccountData accountData;
            MarketData marketData;
            InstrumentIconManager instrumentIcons;
            CandlesManager candlesManager;

        public:
            explicit Worker(DataMonitor& monitor, const trtdata::AccountManager& accounts);
            void requestCatalog(trtdata::Exchange exchange, trttrade::InstType type);
            void enableAccounts();
            void queryAccountHistory(const QString& id, const trttrade::AccountHistoryQuery& query);
            void releaseAccountHistory(const QString& id);

            ~Worker();

            void setInstruments(const std::vector<std::shared_ptr<trtdata::Instrument>>& instruments);

            void setBookInstruments(const BookStream::Instruments& instruments);

            void setTimeZone(trtdata::TradingTimeZone zone);

            void acquireSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

            void releaseSeries(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

            void requestHistory(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

        };

        const trtdata::ConfigManager& configManager;
        QTimer subscriptionTimer;
        QThread workerThread;
        QObject* workerContext = nullptr;
        std::unique_ptr<Worker> worker;
        std::atomic<std::shared_ptr<const DataSnapshot>> snapshot;
        std::atomic<bool> notificationPending = false;

        void publishSnapshot(const DataSnapshot& data);
        std::map<CandleSeriesKey, std::size_t> chartReaders;
        std::map<trtdata::InstIdentifier, std::size_t> bookReaders;
        bool running = false;
        bool accountsEnabled = false;

        void updateBookInstruments();

        void updateInstruments();

        void updateTimeZone();

    public:
        explicit DataMonitor(const trtdata::ConfigManager& configManager);

        ~DataMonitor() override;

        void start();
        void requestCatalog(trtdata::Exchange exchange, trttrade::InstType type);
        std::vector<trttrade::InstType> getInstrumentTypes(trtdata::Exchange exchange) const;
        std::vector<trtdata::Exchange> getExchanges() const;

        void enableAccountMonitoring();
        void queryAccountHistory(const QString& id, const trttrade::AccountHistoryQuery& query);
        void releaseAccountHistory(const QString& id);

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

    signals:
        void snapshotChanged();
    };
}

#endif //NTRT_DATAMONITOR_H
