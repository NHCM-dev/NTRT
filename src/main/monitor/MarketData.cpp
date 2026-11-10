//
// Created by NHCM on 2026/11/8.
//

#include "MarketData.h"
#include "main/data/api/AbstractApi.h"

namespace trtcore
{
    MarketData::MarketData()
    {
        for (trtdata::Exchange exchange : availableExchanges())
            initStream(exchange);
    }

    void MarketData::initStream(trtdata::Exchange exchange)
    {
        std::unique_ptr<TickerStream> stream = exchangeApi(exchange).createTickerStream();
        connect(stream.get(), &TickerStream::tickerReceived, this, [this, exchange](const trtdata::TickerData& ticker)
        {
            updateTicker(exchange, ticker);
        });
        connect(stream.get(), &TickerStream::connectionChanged, this, [this, exchange](const QString& error)
        {
            updateConnection(exchange, error);
        });
        connect(stream.get(), &TickerStream::subscriptionFailed, this,
                [this](const trtdata::InstIdentifier& identifier, const QString& error)
        {
            updateError(identifier, error);
        });
        streams.emplace(exchange, std::move(stream));
    }

    void MarketData::setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments)
    {
        std::map<trtdata::Exchange, TickerStream::Instruments> desired;
        for (const std::shared_ptr<trtdata::Instrument>& instrument : instruments)
            desired[instrument->exchange].insert(instrument->getUniqueIdentifier());

        for (std::pair<const trtdata::Exchange, std::unique_ptr<TickerStream>>& entry : streams)
        {
            const TickerStream::Instruments& identifiers = desired[entry.first];
            if (identifiers.empty())
                failures.erase(entry.first);
            entry.second->setInstruments(identifiers);
        }

        for (std::pair<const trtdata::InstType, Identifier2SnapshotMap>& group : tickersData)
        {
            std::erase_if(group.second, [&desired](const Identifier2SnapshotMap::value_type& entry)
            {
                return !desired.at(entry.first.exchange).contains(entry.first);
            });
        }
    }

    void MarketData::stop()
    {
        for (std::pair<const trtdata::Exchange, std::unique_ptr<TickerStream>>& entry : streams)
            entry.second->stop();
        failures.clear();
    }

    void MarketData::updateTicker(trtdata::Exchange exchange, trtdata::TickerData ticker)
    {
        const trtdata::InstIdentifier identifier{exchange, ticker.instType, ticker.instId};
        MarketSnapshot& snapshot = tickersData[ticker.instType].try_emplace(identifier, MarketSnapshot{exchange}).first->second;
        if (snapshot.ticker && ticker.ts < snapshot.ticker->ts)
            return;

        if (!snapshot.ticker || ticker.ts > snapshot.ticker->ts || ticker.last != snapshot.ticker->last)
        {
            if (snapshot.ticker)
            {
                if (ticker.last != snapshot.ticker->last)
                {
                    ticker.previousLast = snapshot.ticker->last;
                    snapshot.priceChangeAge.start();
                }
                else
                {
                    ticker.previousLast = snapshot.ticker->previousLast;
                }
            }
            snapshot.ticker = std::move(ticker);
        }
        snapshot.error.clear();
    }

    void MarketData::updateConnection(trtdata::Exchange exchange, const QString& error)
    {
        if (error.isEmpty())
            failures.erase(exchange);
        else
            failures.insert_or_assign(exchange, MarketSnapshot{exchange, std::nullopt, error});

        for (std::pair<const trtdata::InstType, Identifier2SnapshotMap>& group : tickersData)
        {
            for (Identifier2SnapshotMap::value_type& entry : group.second)
            {
                if (entry.first.exchange == exchange && (!error.isEmpty() || !entry.second.ticker))
                    entry.second.error = error;
            }
        }
    }

    void MarketData::updateError(const trtdata::InstIdentifier& identifier, const QString& error)
    {
        MarketSnapshot& snapshot = tickersData[identifier.type]
            .try_emplace(identifier, MarketSnapshot{identifier.exchange}).first->second;
        snapshot.error = error;
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

        const std::map<trtdata::Exchange, MarketSnapshot>::const_iterator failure = failures.find(identifier.exchange);
        return failure == failures.end() ? nullptr : &failure->second;
    }

    const MarketData::Identifier2SnapshotMap* MarketData::getSnapshots(trtdata::InstType instType) const
    {
        const std::map<trtdata::InstType, Identifier2SnapshotMap>::const_iterator entry = tickersData.find(instType);
        return entry == tickersData.end() ? nullptr : &entry->second;
    }
}
