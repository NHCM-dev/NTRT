//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_MARKETDATA_H
#define NTRT_MARKETDATA_H

#include "main/data/market/abstract/Instrument.h"
#include <QElapsedTimer>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace trtcore
{
    struct MarketSnapshot
    {
        trtdata::Exchange exchange;
        std::optional<trtdata::TickerData> ticker;
        QString error;
    };

    class MarketData final : public QObject
    {
    public:
        using Identifier2SnapshotMap = std::map<trtdata::InstIdentifier, MarketSnapshot>;
        static constexpr int requestInterval = 200;

        MarketData();
        void updateNext();
        const MarketSnapshot* getSnapshot(const trtdata::InstIdentifier& identifier) const;
        const Identifier2SnapshotMap* getSnapshots(trtdata::InstType instType) const;

    private:
        using Market = std::pair<trtdata::Exchange, trtdata::InstType>;

        struct RequestState
        {
            QElapsedTimer lastRequest;
            int delay = requestInterval;
            bool pending = false;
        };

        std::vector<Market> markets;
        std::map<trtdata::Exchange, RequestState> requests;
        std::map<trtdata::InstType, Identifier2SnapshotMap> tickersData;
        std::map<Market, MarketSnapshot> failures;
        std::size_t nextMarket = 0;

        void requestMarket(const Market& market);
    };
}

#endif //NTRT_MARKETDATA_H
