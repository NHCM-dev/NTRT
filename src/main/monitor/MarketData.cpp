//
// Created by NHCM on 2026/11/8.
//

#include "MarketData.h"
#include <QFutureWatcher>
#include <algorithm>
#include <exception>

namespace trtcore
{
    void MarketData::setInstruments(std::span<const InstrumentPtr> collection)
    {
        if (std::ranges::equal(instruments, collection))
            return;

        instruments.assign(collection.begin(), collection.end());
        std::erase_if(snapshots, [this](const std::pair<const InstrumentPtr, MarketSnapshot>& entry)
        {
            return std::ranges::find(instruments, entry.first) == instruments.end();
        });
        nextInstrument = 0;
    }

    void MarketData::updateNext()
    {
        // OKX permits 20 ticker requests per 2 seconds per IP.
        if (pending || instruments.empty() || (lastRequest.isValid() && lastRequest.elapsed() < requestInterval))
            return;

        const InstrumentPtr instrument = instruments[nextInstrument];
        nextInstrument = (nextInstrument + 1) % instruments.size();
        snapshots.try_emplace(instrument);
        QFutureWatcher<trtdata::TickerData>* watcher = new QFutureWatcher<trtdata::TickerData>(this);
        pending = true;
        lastRequest.start();

        connect(watcher, &QFutureWatcher<trtdata::TickerData>::finished, this, [this, watcher, instrument]
        {
            pending = false;
            watcher->deleteLater();

            const std::map<InstrumentPtr, MarketSnapshot>::iterator entry = snapshots.find(instrument);
            if (entry == snapshots.end())
                return;
            MarketSnapshot& current = entry->second;
            try
            {
                trtdata::TickerData ticker = watcher->result();
                if (!current.ticker || ticker.ts >= current.ticker->ts)
                    current.ticker = std::move(ticker);
                current.error.clear();
                requestInterval = 200;
            }
            catch (const std::exception& error)
            {
                current.error = QString::fromUtf8(error.what());
                requestInterval = 2000;
                lastRequest.restart();
            }
        });
        watcher->setFuture(instrument->requestTicker());
    }

    const MarketSnapshot* MarketData::getSnapshot(const InstrumentPtr& instrument) const
    {
        const std::map<InstrumentPtr, MarketSnapshot>::const_iterator entry = snapshots.find(instrument);
        return entry == snapshots.end() ? nullptr : &entry->second;
    }
}
