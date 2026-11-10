//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_MARKETDATA_H
#define NTRT_MARKETDATA_H

#include "main/data/api/TickerStream.h"
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

    class MarketData final : public QObject
    {
    public:
        using Identifier2SnapshotMap = std::map<trtdata::InstIdentifier, MarketSnapshot>;
        MarketData();
        void setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments);
        void stop();
        const MarketSnapshot* getSnapshot(const trtdata::InstIdentifier& identifier) const;
        const Identifier2SnapshotMap* getSnapshots(trtdata::InstType instType) const;

    private:
        std::map<trtdata::InstType, Identifier2SnapshotMap> tickersData;
        std::map<trtdata::Exchange, MarketSnapshot> failures;
        std::map<trtdata::Exchange, std::unique_ptr<TickerStream>> streams;

        void initStream(trtdata::Exchange exchange);
        void updateTicker(trtdata::Exchange exchange, trtdata::TickerData ticker);
        void updateConnection(trtdata::Exchange exchange, const QString& error);
        void updateError(const trtdata::InstIdentifier& identifier, const QString& error);
    };
}

#endif //NTRT_MARKETDATA_H
