//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_MARKETDATA_H
#define NTRT_MARKETDATA_H

#include "main/api/TickerStream.h"
#include "main/trade/TradeDef.h"
#include <QElapsedTimer>
#include <map>
#include <memory>
#include <optional>
#include <span>

namespace trtcore
{
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

        const MarketSnapshot* getSnapshot(const trtdata::InstIdentifier& identifier) const;

        const Identifier2SnapshotMap* getSnapshots(trttrade::InstType instType) const;

    private:
        friend class MarketData;
        std::map<trttrade::InstType, Identifier2SnapshotMap> tickersData;
        std::map<trtdata::Exchange, MarketSnapshot> failures;
    };

    class MarketData final : public QObject
    {
    public:
        using Identifier2SnapshotMap = MarketDataSnapshot::Identifier2SnapshotMap;

        MarketData();

        void setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments);

        void stop();

        MarketDataSnapshot getSnapshot() const;

    private:
        MarketDataSnapshot snapshot;
        std::map<trtdata::Exchange, std::unique_ptr<TickerStream>> streams;

        void initStream(trtdata::Exchange exchange);

        void updateTicker(trtdata::Exchange exchange, trtdata::TickerData ticker);

        void updateConnection(trtdata::Exchange exchange, const QString& error);

        void updateError(const trtdata::InstIdentifier& identifier, const QString& error);
    };
}

#endif //NTRT_MARKETDATA_H
