#include "TradeSnapshotDef.h"
#include "main/util/CandleUtil.h"
#include <QDateTime>

namespace trtcore
{
    const trttrade::BookSnapshot* MarketDataSnapshot::getBook(const trtdata::InstIdentifier& identifier) const
    {
        const std::map<trtdata::InstIdentifier, trttrade::BookSnapshot>::const_iterator found = books.find(identifier);
        return found == books.end() ? nullptr : &found->second;
    }

    const MarketSnapshot* MarketDataSnapshot::getSnapshot(const trtdata::InstIdentifier& identifier) const
    {
        const Identifier2SnapshotMap* snapshots = getSnapshots(identifier.type);
        if (snapshots)
        {
            const Identifier2SnapshotMap::const_iterator entry = snapshots->find(identifier);
            if (entry != snapshots->end())
                return &entry->second;
        }

        const std::map<trtdata::Exchange, MarketSnapshot>::const_iterator failure = failures.find(identifier.exchange);
        return failure == failures.end() ? nullptr : &failure->second;
    }

    const MarketDataSnapshot::Identifier2SnapshotMap* MarketDataSnapshot::getSnapshots(
        trttrade::InstType instType) const
    {
        const std::map<trttrade::InstType, Identifier2SnapshotMap>::const_iterator entry = tickersData.find(instType);
        return entry == tickersData.end() ? nullptr : &entry->second;
    }

    trtdata::TradingTimeZone CandlesSnapshot::getTimeZone() const
    {
        return timeZone;
    }

    const CandleSnapshot* CandlesSnapshot::getSnapshot(const trtdata::InstIdentifier& identifier,
                                                       trttrade::CandleTimeframe timeframe) const
    {
        const std::map<CandleSeriesKey, std::shared_ptr<const CandleSnapshot>>::const_iterator entry = series.
            find({identifier, timeframe});
        return entry == series.end() ? nullptr : entry->second.get();
    }

    const trttrade::CandleData* CandlesSnapshot::getDailyCandle(const trtdata::InstIdentifier& identifier) const
    {
        const CandleSnapshot* snapshot = getSnapshot(identifier);
        if (!snapshot)
            return nullptr;
        return trtutils::dailyCandle(snapshot->candles, QDateTime::currentMSecsSinceEpoch(), timeZone);
    }

    std::optional<double> CandlesSnapshot::getDailyChangePercent(const trtdata::InstIdentifier& identifier,
                                                                 double latestPrice) const
    {
        const trttrade::CandleData* candle = getDailyCandle(identifier);
        return trtutils::dailyChangePercent(candle, latestPrice);
    }
}
