#ifndef NTRT_CANDLESMANAGER_H
#define NTRT_CANDLESMANAGER_H

#include "main/trade/TradeDataDef.h"
#include "main/market/abstract/Instrument.h"
#include "main/data/TradingDataConfig.h"
#include <QFutureWatcher>
#include <QTimer>
#include "main/api/CandleStream.h"
#include <map>
#include <memory>
#include <span>
#include <vector>
#include "main/trade/CandleTimeframe.h"

namespace trtcore
{
    class CandlesSnapshot;

    struct CandleSnapshot
    {
        std::vector<trttrade::CandleData> candles;
        QString error;
        qint64 updatedAt = 0;
        bool historyLoading = false;
        bool historyExhausted = false;
    };

    class CandlesManager final : public QObject
    {
    public:
        using CandleMap = std::map<std::int64_t, trttrade::CandleData>;

        struct SeriesKey
        {
            trtdata::InstIdentifier instrument;
            trttrade::CandleTimeframe timeframe = trttrade::CandleTimeframe::Day1;

            std::strong_ordering operator<=>(const SeriesKey&) const = default;
        };

    private:
        struct Subscription
        {
            CandleSnapshot snapshot;
            std::shared_ptr<const CandleSnapshot> publishedSnapshot;
            QFutureWatcher<std::vector<trttrade::CandleData>>* watcher = nullptr;
            qint64 nextRequest = 0;
            std::size_t readers = 0;
            bool tracked = false;
            std::unique_ptr<CandleStream> stream;
            bool streamReady = false;
            QString streamError;
            bool refill = true;
            std::optional<qint64> recoveryFrom;
            std::optional<qint64> recoveryCursor;
            std::uint64_t recoveryGeneration = 0;
            std::vector<trttrade::CandleData> requestLiveCandles;
            std::vector<trttrade::CandleData> historyLiveCandles;
            QFutureWatcher<std::vector<trttrade::CandleData>>* historyWatcher = nullptr;
            qint64 historyTarget = 0;
            qint64 historyRetry = 0;
            bool historyExhausted = false;
        };

        std::map<SeriesKey, Subscription> subscriptions;
        QTimer requestTimer;
        qint64 nextStreamStart = 0;
        trtdata::TradingDataTimeZone timeZone = trtdata::TradingDataTimeZone::UTC;

        void ensureStream(const SeriesKey& key, Subscription& subscription);

        void beginRecovery(Subscription& subscription);

        void fetchHistory(const SeriesKey& key, Subscription& subscription);

        void fetchNext();

        void fetchCandles(const SeriesKey& key, Subscription& subscription);

        void mergeCandles(Subscription& subscription, const std::vector<trttrade::CandleData>& candles);

        void cancelRequest(Subscription& subscription);

    public:
        CandlesManager();

        CandlesSnapshot getReadSnapshot();

        void setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments);

        void stop();

        void subscribe(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

        void requestHistory(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

        void unsubscribe(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe);

        // Cache-only queries. Use on the manager thread; views expire on updates, removal or time-zone changes.
        const trttrade::CandleData* getCandle(const trtdata::InstIdentifier& instrument,
                                              trttrade::CandleTimeframe timeframe, std::int64_t timestamp) const;

        std::span<const trttrade::CandleData> getCandles(const trtdata::InstIdentifier& instrument,
                                                         trttrade::CandleTimeframe timeframe) const;

        std::span<const trttrade::CandleData> getCandles(const trtdata::InstIdentifier& instrument,
                                                         trttrade::CandleTimeframe timeframe, std::int64_t from,
                                                         std::int64_t to) const;

        std::span<const trttrade::CandleData> getLatestCandles(const trtdata::InstIdentifier& instrument,
                                                               trttrade::CandleTimeframe timeframe,
                                                               std::size_t count) const;

        CandleMap getCandleMap(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe) const;

        void setTimeZone(trtdata::TradingDataTimeZone value);

        trtdata::TradingDataTimeZone getTimeZone() const;

        const CandleSnapshot* getSnapshot(const trtdata::InstIdentifier& identifier,
                                          trttrade::CandleTimeframe timeframe = trttrade::CandleTimeframe::Day1) const;

        const trttrade::CandleData* getDailyCandle(const trtdata::InstIdentifier& identifier) const;

        std::optional<double>
        getDailyChangePercent(const trtdata::InstIdentifier& identifier, double latestPrice) const;
    };

    class CandlesSnapshot final
    {
        friend class CandlesManager;
        std::map<CandlesManager::SeriesKey, std::shared_ptr<const CandleSnapshot>> series;
        trtdata::TradingDataTimeZone timeZone = trtdata::TradingDataTimeZone::UTC;

    public:
        trtdata::TradingDataTimeZone getTimeZone() const;

        const CandleSnapshot* getSnapshot(const trtdata::InstIdentifier& identifier,
                                          trttrade::CandleTimeframe timeframe = trttrade::CandleTimeframe::Day1) const;

        const trttrade::CandleData* getDailyCandle(const trtdata::InstIdentifier& identifier) const;

        std::optional<double>
        getDailyChangePercent(const trtdata::InstIdentifier& identifier, double latestPrice) const;
    };
}

#endif //NTRT_CANDLESMANAGER_H
