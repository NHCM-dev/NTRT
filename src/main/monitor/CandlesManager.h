#ifndef NTRT_CANDLESMANAGER_H
#define NTRT_CANDLESMANAGER_H

#include "main/trade/TradeSnapshotDef.h"
#include "main/market/abstract/Instrument.h"
#include "main/data/core/TradingTimeDef.h"
#include <QFutureWatcher>
#include <QTimer>
#include "main/api/CandleStream.h"
#include <map>
#include <memory>
#include <span>
#include <vector>
#include "main/trade/TradeDef.h"

namespace trtcore
{
    class CandlesManager final : public QObject
    {
    public:
        using CandleMap = std::map<std::int64_t, trttrade::CandleData>;

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

        std::map<CandleSeriesKey, Subscription> subscriptions;
        QTimer requestTimer;
        qint64 nextStreamStart = 0;
        trtdata::TradingTimeZone timeZone = trtdata::TradingTimeZone::UTC;

        void ensureStream(const CandleSeriesKey& key, Subscription& subscription);

        void beginRecovery(Subscription& subscription);

        void fetchHistory(const CandleSeriesKey& key, Subscription& subscription);

        void fetchNext();

        void fetchCandles(const CandleSeriesKey& key, Subscription& subscription);

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

        void setTimeZone(trtdata::TradingTimeZone value);

        trtdata::TradingTimeZone getTimeZone() const;

        const CandleSnapshot* getSnapshot(const trtdata::InstIdentifier& identifier,
                                          trttrade::CandleTimeframe timeframe = trttrade::CandleTimeframe::Day1) const;

        const trttrade::CandleData* getDailyCandle(const trtdata::InstIdentifier& identifier) const;

        std::optional<double>
        getDailyChangePercent(const trtdata::InstIdentifier& identifier, double latestPrice) const;
    };

}

#endif //NTRT_CANDLESMANAGER_H
