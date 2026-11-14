#ifndef NTRT_CANDLESMANAGER_H
#define NTRT_CANDLESMANAGER_H

#include "main/data/market/CandleData.h"
#include "main/data/market/abstract/Instrument.h"
#include "main/data/TradingDataConfig.h"
#include <QFutureWatcher>
#include <QTimer>
#include <map>
#include <memory>
#include <span>
#include <vector>

namespace trtcore
{
    struct CandleSnapshot
    {
        std::vector<trtdata::CandleData> candles;
        QString error;
        qint64 updatedAt = 0;
    };

    class CandlesManager final : public QObject
    {
        struct Subscription
        {
            CandleSnapshot snapshot;
            QFutureWatcher<std::vector<trtdata::CandleData>>* watcher = nullptr;
            qint64 nextRequest = 0;
        };

        std::map<trtdata::InstIdentifier, Subscription> subscriptions;
        QTimer requestTimer;
        trtdata::TradingDataTimeZone timeZone = trtdata::TradingDataTimeZone::UTC;

        void fetchNext();
        void fetchCandles(const trtdata::InstIdentifier& identifier, Subscription& subscription);
        void mergeCandles(Subscription& subscription, const std::vector<trtdata::CandleData>& candles);
        void cancelRequest(Subscription& subscription);

    public:
        CandlesManager();
        void setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments);
        void stop();
        void setTimeZone(trtdata::TradingDataTimeZone value);
        trtdata::TradingDataTimeZone getTimeZone() const;
        const CandleSnapshot* getSnapshot(const trtdata::InstIdentifier& identifier) const;
        const trtdata::CandleData* getDailyCandle(const trtdata::InstIdentifier& identifier) const;
        std::optional<double> getDailyChangePercent(const trtdata::InstIdentifier& identifier, double latestPrice) const;
    };
}

#endif //NTRT_CANDLESMANAGER_H
