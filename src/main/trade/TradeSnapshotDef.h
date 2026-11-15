#ifndef NTRT_TRADESNAPSHOTDEF_H
#define NTRT_TRADESNAPSHOTDEF_H

#include "TradeDataDef.h"
#include "main/data/TradingDataDef.h"
#include <QElapsedTimer>
#include <QString>
#include <map>
#include <memory>
#include <vector>

namespace trttrade
{
    struct BookSnapshot
    {
        std::vector<BookLevel> bids;
        std::vector<BookLevel> asks;
        qint64 updatedAt = 0;
        qint64 sequence = -1;
        QString error;
    };
}

namespace trtcore
{
    struct CandleSnapshot
    {
        std::vector<trttrade::CandleData> candles;
        QString error;
        qint64 updatedAt = 0;
        bool historyLoading = false;
        bool historyExhausted = false;
    };

    struct MarketSnapshot
    {
        trtdata::Exchange exchange;
        std::optional<trtdata::TickerData> ticker{};
        QString error{};
        QElapsedTimer priceChangeAge{};
    };

    class MarketDataSnapshot
    {
    public:
        using Identifier2SnapshotMap = std::map<trtdata::InstIdentifier, MarketSnapshot>;

        const trttrade::BookSnapshot* getBook(const trtdata::InstIdentifier& identifier) const;

        const MarketSnapshot* getSnapshot(const trtdata::InstIdentifier& identifier) const;

        const Identifier2SnapshotMap* getSnapshots(trttrade::InstType instType) const;

    private:
        // Required friendship declaration: including the owning manager would create an include cycle.
        friend class MarketData;
        std::map<trttrade::InstType, Identifier2SnapshotMap> tickersData;
        std::map<trtdata::Exchange, MarketSnapshot> failures;
        std::map<trtdata::InstIdentifier, trttrade::BookSnapshot> books;
    };

    class CandlesSnapshot final
    {
        // Required friendship declaration: including the owning manager would create an include cycle.
        friend class CandlesManager;
        std::map<CandleSeriesKey, std::shared_ptr<const CandleSnapshot>> series;
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

#endif //NTRT_TRADESNAPSHOTDEF_H
