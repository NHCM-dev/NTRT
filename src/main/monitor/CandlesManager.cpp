#include "CandlesManager.h"
#include "main/data/api/AbstractApi.h"
#include <QDateTime>
#include <algorithm>
#include <set>
#include <cmath>

namespace trtcore
{
    CandlesManager::CandlesManager()
    {
        requestTimer.setInterval(100);
        connect(&requestTimer, &QTimer::timeout, this, [this] { fetchNext(); });
    }

    void CandlesManager::setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments)
    {
        std::set<trtdata::InstIdentifier> desired;
        for (const std::shared_ptr<trtdata::Instrument>& instrument : instruments)
        {
            desired.insert(instrument->getUniqueIdentifier());
            subscriptions.try_emplace(instrument->getUniqueIdentifier());
        }
        std::erase_if(subscriptions, [this, &desired](std::pair<const trtdata::InstIdentifier, Subscription>& entry)
        {
            if (desired.contains(entry.first))
                return false;
            cancelRequest(entry.second);
            return true;
        });
        if (subscriptions.empty())
            requestTimer.stop();
        else
            requestTimer.start();
    }

    void CandlesManager::cancelRequest(Subscription& subscription)
    {
        if (!subscription.watcher)
            return;
        subscription.watcher->disconnect(this);
        subscription.watcher->cancel();
        subscription.watcher->deleteLater();
        subscription.watcher = nullptr;
    }

    void CandlesManager::stop()
    {
        requestTimer.stop();
        for (std::pair<const trtdata::InstIdentifier, Subscription>& entry : subscriptions)
        {
            cancelRequest(entry.second);
            entry.second.nextRequest = 0;
        }
    }

    void CandlesManager::fetchNext()
    {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        int pending = 0;
        std::map<trtdata::InstIdentifier, Subscription>::iterator next = subscriptions.end();
        for (std::map<trtdata::InstIdentifier, Subscription>::iterator entry = subscriptions.begin(); entry != subscriptions.end(); entry++)
        {
            if (entry->second.watcher)
                pending++;
            else if (entry->second.nextRequest <= now &&
                     (next == subscriptions.end() || entry->second.nextRequest < next->second.nextRequest))
                next = entry;
        }
        if (pending < 4 && next != subscriptions.end())
            fetchCandles(next->first, next->second);
    }

    void CandlesManager::fetchCandles(const trtdata::InstIdentifier& identifier, Subscription& subscription)
    {
        QFutureWatcher<std::vector<trtdata::CandleData>>* watcher = new QFutureWatcher<std::vector<trtdata::CandleData>>(this);
        subscription.watcher = watcher;
        connect(watcher, &QFutureWatcherBase::finished, this, [this, identifier, watcher]
        {
            const std::map<trtdata::InstIdentifier, Subscription>::iterator entry = subscriptions.find(identifier);
            if (entry == subscriptions.end() || entry->second.watcher != watcher)
                return;
            Subscription& current = entry->second;
            current.watcher = nullptr;
            try
            {
                const std::vector<trtdata::CandleData> candles = watcher->result();
                if (candles.empty())
                    throw std::runtime_error("OKX returned no candles");
                mergeCandles(current, candles);
                current.snapshot.error.clear();
                current.snapshot.updatedAt = QDateTime::currentMSecsSinceEpoch();
                current.nextRequest = current.snapshot.updatedAt + 10000;
            }
            catch (const std::exception& error)
            {
                current.snapshot.error = QString::fromUtf8(error.what());
                current.nextRequest = QDateTime::currentMSecsSinceEpoch() + 30000;
            }
            watcher->deleteLater();
        });
        const bool refill = subscription.snapshot.candles.empty() ||
                            QDateTime::currentMSecsSinceEpoch() - subscription.snapshot.updatedAt > 86400000;
        const int limit = refill ? 300 : 3;
        watcher->setFuture(exchangeApi(identifier.exchange).fetchDailyCandles(identifier, timeZone, limit));
    }

    void CandlesManager::mergeCandles(Subscription& subscription, const std::vector<trtdata::CandleData>& candles)
    {
        std::map<std::int64_t, trtdata::CandleData> merged;
        for (const trtdata::CandleData& candle : subscription.snapshot.candles)
            merged.emplace(candle.ts, candle);
        for (const trtdata::CandleData& candle : candles)
            merged.insert_or_assign(candle.ts, candle);
        while (merged.size() > 300)
            merged.erase(merged.begin());
        subscription.snapshot.candles.clear();
        for (const std::pair<const std::int64_t, trtdata::CandleData>& entry : merged)
            subscription.snapshot.candles.push_back(entry.second);
    }

    const CandleSnapshot* CandlesManager::getSnapshot(const trtdata::InstIdentifier& identifier) const
    {
        const std::map<trtdata::InstIdentifier, Subscription>::const_iterator entry = subscriptions.find(identifier);
        return entry == subscriptions.end() ? nullptr : &entry->second.snapshot;
    }

    const trtdata::CandleData* CandlesManager::getDailyCandle(const trtdata::InstIdentifier& identifier) const
    {
        const CandleSnapshot* snapshot = getSnapshot(identifier);
        if (!snapshot || snapshot->candles.empty())
            return nullptr;
        const qint64 dayMs = 86400000;
        const qint64 offsetMs = trtdata::tradingDataTimeZoneOffsetMs(timeZone);
        const qint64 today = (QDateTime::currentMSecsSinceEpoch() + offsetMs) / dayMs * dayMs - offsetMs;
        const trtdata::CandleData& candle = snapshot->candles.back();
        return candle.ts == today ? &candle : nullptr;
    }

    void CandlesManager::setTimeZone(trtdata::TradingDataTimeZone value)
    {
        if (timeZone == value)
            return;
        timeZone = value;
        for (std::pair<const trtdata::InstIdentifier, Subscription>& entry : subscriptions)
        {
            cancelRequest(entry.second);
            entry.second.snapshot = {};
            entry.second.nextRequest = 0;
        }
    }

    trtdata::TradingDataTimeZone CandlesManager::getTimeZone() const
    {
        return timeZone;
    }

    std::optional<double> CandlesManager::getDailyChangePercent(const trtdata::InstIdentifier& identifier, double latestPrice) const
    {
        const trtdata::CandleData* candle = getDailyCandle(identifier);
        if (!candle || candle->open <= 0 || !std::isfinite(latestPrice) || latestPrice <= 0)
            return std::nullopt;
        return (latestPrice - candle->open) / candle->open * 100.0;
    }
}
