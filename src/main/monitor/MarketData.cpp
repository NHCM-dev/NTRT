//
// Created by NHCM on 2026/11/8.
//

#include "MarketData.h"
#include "main/data/api/AbstractApi.h"
#include <QFutureWatcher>
#include <algorithm>
#include <exception>

namespace trtcore
{
    MarketData::MarketData()
    {
        for (trtdata::Exchange exchange : availableExchanges())
        {
            for (trtdata::InstType type : exchangeApi(exchange).instrumentTypes())
                markets.emplace_back(exchange, type);
        }
    }

    void MarketData::updateNext()
    {
        for (std::size_t index = 0; index < markets.size(); index++)
        {
            const Market& market = markets[nextMarket];
            nextMarket = (nextMarket + 1) % markets.size();
            RequestState& state = requests[market.first];
            if (state.pending || (state.lastRequest.isValid() && state.lastRequest.elapsed() < state.delay))
                continue;

            requestMarket(market);
            return;
        }
    }

    void MarketData::requestMarket(const Market& market)
    {
        using Batch = std::shared_ptr<std::vector<trtdata::TickerData>>;
        const AbstractApi& api = exchangeApi(market.first);
        const int interval = std::max(requestInterval, api.getRequestInterval());
        RequestState& state = requests[market.first];
        state.pending = true;
        state.delay = interval;
        state.lastRequest.start();

        QFutureWatcher<Batch>* watcher = new QFutureWatcher<Batch>(this);
        connect(watcher, &QFutureWatcher<Batch>::finished, this, [this, watcher, market]
        {
            RequestState& state = requests.at(market.first);
            state.pending = false;
            watcher->deleteLater();

            try
            {
                const Batch batch = watcher->result();
                for (trtdata::TickerData& ticker : *batch)
                {
                    const trtdata::InstIdentifier identifier{market.first, ticker.instType, ticker.instId};
                    MarketSnapshot& snapshot = tickersData[ticker.instType][identifier];
                    snapshot.exchange = market.first;
                    if (!snapshot.ticker || ticker.ts > snapshot.ticker->ts ||
                        (ticker.ts == snapshot.ticker->ts && ticker.last != snapshot.ticker->last))
                    {
                        if (snapshot.ticker)
                            ticker.previousLast = snapshot.ticker->last;
                        snapshot.ticker = std::move(ticker);
                        snapshot.quoteAge.start();
                    }
                    snapshot.error.clear();
                }
                failures.erase(market);
                const std::map<trtdata::InstType, Identifier2SnapshotMap>::iterator group = tickersData.find(market.second);
                if (group != tickersData.end())
                {
                    for (std::pair<const trtdata::InstIdentifier, MarketSnapshot>& entry : group->second)
                    {
                        if (entry.first.exchange == market.first)
                            entry.second.error.clear();
                    }
                }
            }
            catch (const std::exception& error)
            {
                const QString message = QString::fromUtf8(error.what());
                failures.insert_or_assign(market, MarketSnapshot{market.first, std::nullopt, message});
                const std::map<trtdata::InstType, Identifier2SnapshotMap>::iterator group = tickersData.find(market.second);
                if (group != tickersData.end())
                {
                    for (std::pair<const trtdata::InstIdentifier, MarketSnapshot>& entry : group->second)
                    {
                        if (entry.first.exchange == market.first)
                            entry.second.error = message;
                    }
                }
                state.delay = std::max(state.delay, 2000);
                state.lastRequest.restart();
            }
        });
        watcher->setFuture(api.fetchTickers(market.second));
    }

    const MarketSnapshot* MarketData::getSnapshot(const trtdata::InstIdentifier& identifier) const
    {
        const Identifier2SnapshotMap* snapshots = getSnapshots(identifier.type);
        if (snapshots)
        {
            const Identifier2SnapshotMap::const_iterator entry = snapshots->find(identifier);
            if (entry != snapshots->end())
                return &entry->second;
        }

        const std::map<Market, MarketSnapshot>::const_iterator failure = failures.find({identifier.exchange, identifier.type});
        return failure == failures.end() ? nullptr : &failure->second;
    }

    const MarketData::Identifier2SnapshotMap* MarketData::getSnapshots(trtdata::InstType instType) const
    {
        const std::map<trtdata::InstType, Identifier2SnapshotMap>::const_iterator entry = tickersData.find(instType);
        return entry == tickersData.end() ? nullptr : &entry->second;
    }
}
