//
// Created by NHCM on 2026/11/8.
//

#include "MarketData.h"
#include "main/api/AbstractApi.h"

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
                snapshot.failures.erase(entry.first);
            entry.second->setInstruments(identifiers);
        }

        for (std::pair<const trttrade::InstType, Identifier2SnapshotMap>& group : snapshot.tickersData)
        {
            std::erase_if(group.second, [&desired](const Identifier2SnapshotMap::value_type& entry)
            {
                return !desired.at(entry.first.exchange).contains(entry.first);
            });
        }
    }

    void MarketData::setBookInstruments(const BookStream::Instruments& instruments)
    {
        for (trtdata::Exchange exchange : availableExchanges())
        {
            BookStream::Instruments desired;
            for (const trtdata::InstIdentifier& instrument : instruments)
            {
                if (instrument.exchange == exchange)
                    desired.insert(instrument);
            }
            if (!desired.empty() && !bookStreams.contains(exchange))
            {
                std::unique_ptr<BookStream> stream = exchangeApi(exchange).createBookStream();
                if (stream)
                {
                    connect(stream.get(), &BookStream::bookReceived, this,
                            [this](const trtdata::InstIdentifier& instrument, const trttrade::BookSnapshot& book)
                    {
                        snapshot.books.insert_or_assign(instrument, book);
                    });
                    bookStreams.emplace(exchange, std::move(stream));
                }
            }
            if (bookStreams.contains(exchange))
                bookStreams.at(exchange)->setInstruments(desired);
            else
            {
                for (const trtdata::InstIdentifier& instrument : desired)
                    snapshot.books[instrument].error = "Order book unavailable for this exchange";
            }
        }
        std::erase_if(snapshot.books, [&instruments](const std::pair<const trtdata::InstIdentifier, trttrade::BookSnapshot>& entry)
        {
            return !instruments.contains(entry.first);
        });
    }

    void MarketData::stop()
    {
        for (std::pair<const trtdata::Exchange, std::unique_ptr<TickerStream>>& entry : streams)
            entry.second->stop();
        for (std::pair<const trtdata::Exchange, std::unique_ptr<BookStream>>& entry : bookStreams)
            entry.second->stop();
        snapshot.books.clear();
        snapshot.failures.clear();
    }

    void MarketData::updateTicker(trtdata::Exchange exchange, trtdata::TickerData ticker)
    {
        const trtdata::InstIdentifier identifier{exchange, ticker.instType, ticker.instId};
        MarketSnapshot& snapshot = this->snapshot.tickersData[ticker.instType].try_emplace(
            identifier, MarketSnapshot{exchange}).first->second;
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
                } else
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
            snapshot.failures.erase(exchange);
        else
            snapshot.failures.insert_or_assign(exchange, MarketSnapshot{exchange, std::nullopt, error});

        for (std::pair<const trttrade::InstType, Identifier2SnapshotMap>& group : snapshot.tickersData)
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
        MarketSnapshot& snapshot = this->snapshot.tickersData[identifier.type]
                                   .try_emplace(identifier, MarketSnapshot{identifier.exchange}).first->second;
        snapshot.error = error;
    }

    MarketDataSnapshot MarketData::getSnapshot() const
    {
        return snapshot;
    }

}
