#ifndef NTRT_TRADESNAPSHOTDEF_H
#define NTRT_TRADESNAPSHOTDEF_H

#include "TradeDataDef.h"
#include "main/market/abstract/Instrument.h"
#include "main/data/core/TradingTimeDef.h"
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
    inline constexpr qint64 marketFeedTimeoutMs = 15000;

    struct CandleSnapshot
    {
        qint64 revision = 0;
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

    struct InstrumentCatalogSnapshot
    {
        std::vector<std::shared_ptr<trtdata::Instrument>> instruments;
        QString error;
        bool loading = false;
        qint64 revision = 0;
    };

    class MarketDataSnapshot
    {
    public:
        const InstrumentCatalogSnapshot* getCatalog(trtdata::Exchange exchange, trttrade::InstType type) const;

        qint64 getTickerRevision() const { return tickerRevision; }

        using Identifier2SnapshotMap = std::map<trtdata::InstIdentifier, MarketSnapshot>;

        std::shared_ptr<const trttrade::BookSnapshot> getBook(const trtdata::InstIdentifier& identifier) const;

        const MarketSnapshot* getSnapshot(const trtdata::InstIdentifier& identifier) const;

        const Identifier2SnapshotMap* getSnapshots(trttrade::InstType instType) const;

    private:
        // Required friendship declaration: including the owning manager would create an include cycle.
        friend class MarketData;
        std::map<std::pair<trtdata::Exchange, trttrade::InstType>, InstrumentCatalogSnapshot> catalogs;
        qint64 tickerRevision = 0;
        std::map<trttrade::InstType, Identifier2SnapshotMap> tickersData;
        std::map<trtdata::Exchange, MarketSnapshot> failures;
        std::map<trtdata::InstIdentifier, std::shared_ptr<const trttrade::BookSnapshot>> books;
    };

    class CandlesSnapshot final
    {
        // Required friendship declaration: including the owning manager would create an include cycle.
        friend class CandlesManager;
        std::map<CandleSeriesKey, std::shared_ptr<const CandleSnapshot>> series;
        trtdata::TradingTimeZone timeZone = trtdata::TradingTimeZone::UTC;

    public:
        trtdata::TradingTimeZone getTimeZone() const;

        const CandleSnapshot* getSnapshot(const trtdata::InstIdentifier& identifier,
                                          trttrade::CandleTimeframe timeframe = trttrade::CandleTimeframe::Day1) const;

        const trttrade::CandleData* getDailyCandle(const trtdata::InstIdentifier& identifier) const;

        std::optional<double>
        getDailyChangePercent(const trtdata::InstIdentifier& identifier, double latestPrice) const;
    };
}

#endif //NTRT_TRADESNAPSHOTDEF_H
