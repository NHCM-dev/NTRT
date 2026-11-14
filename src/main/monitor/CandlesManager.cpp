#include "CandlesManager.h"
#include "main/api/AbstractApi.h"
#include <QDateTime>
#include <algorithm>
#include <cmath>
#include <limits>

namespace trtcore
{
    CandlesManager::CandlesManager()
    {
        requestTimer.setInterval(100);
        connect(&requestTimer, &QTimer::timeout, this, [this] { fetchNext(); });
    }

    void CandlesManager::setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments)
    {
        for (std::pair<const SeriesKey, Subscription>& entry : subscriptions)
            entry.second.tracked = false;
        for (const std::shared_ptr<trtdata::Instrument>& instrument : instruments)
        {
            subscriptions[SeriesKey{instrument->getUniqueIdentifier()}].tracked = true;
        }
        std::erase_if(subscriptions, [this](std::pair<const SeriesKey, Subscription>& entry)
        {
            if (entry.second.tracked || entry.second.readers != 0)
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
        if (subscription.stream)
        {
            subscription.stream->stop();
            subscription.stream.reset();
        }
        if (subscription.historyWatcher)
        {
            subscription.historyWatcher->disconnect(this);
            subscription.historyWatcher->cancel();
            subscription.historyWatcher->deleteLater();
            subscription.historyWatcher = nullptr;
        }
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
        for (std::pair<const SeriesKey, Subscription>& entry : subscriptions)
        {
            cancelRequest(entry.second);
            entry.second.nextRequest = 0;
        }
    }

    void CandlesManager::ensureStream(const SeriesKey& key, Subscription& subscription)
    {
        if (subscription.stream)
            return;
        subscription.stream = exchangeApi(key.instrument.exchange).createCandleStream();
        if (!subscription.stream)
            return;
        connect(subscription.stream.get(), &CandleStream::candlesReceived, this, [this, key](const std::vector<trttrade::CandleData>& candles)
        {
            Subscription& current = subscriptions.at(key);
            mergeCandles(current, candles);
            current.snapshot.updatedAt = QDateTime::currentMSecsSinceEpoch();
        });
        connect(subscription.stream.get(), &CandleStream::connectionChanged, this, [this, key](const QString& error)
        {
            Subscription& current = subscriptions.at(key);
            current.snapshot.error = error;
            if (error.isEmpty())
                current.nextRequest = 0; // Backfill bars missed during a disconnect.
        });
        subscription.stream->start(key.instrument, key.timeframe, timeZone);
    }

    void CandlesManager::requestHistory(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        const std::map<SeriesKey, Subscription>::iterator entry = subscriptions.find({instrument, timeframe});
        if (entry == subscriptions.end() || entry->second.historyExhausted || entry->second.snapshot.candles.empty())
            return;
        entry->second.historyTarget = entry->second.snapshot.candles.front().timestamp;
        entry->second.snapshot.historyLoading = true;
    }

    void CandlesManager::fetchHistory(const SeriesKey& key, Subscription& subscription)
    {
        if (subscription.snapshot.candles.empty())
            return;
        const qint64 cursor = subscription.snapshot.candles.front().timestamp;
        QFutureWatcher<std::vector<trttrade::CandleData>>* watcher = new QFutureWatcher<std::vector<trttrade::CandleData>>(this);
        subscription.historyWatcher = watcher;
        connect(watcher, &QFutureWatcherBase::finished, this, [this, key, watcher, cursor]
        {
            Subscription& current = subscriptions.at(key);
            current.historyWatcher = nullptr;
            try
            {
                const std::vector<trttrade::CandleData> page = watcher->result();
                if (page.empty())
                    current.historyExhausted = true;
                else if (page.front().timestamp >= cursor)
                    throw std::runtime_error("History endpoint did not advance the pagination cursor");
                else
                    mergeCandles(current, page);
                current.historyTarget = 0;
                current.snapshot.historyLoading = false;
                current.snapshot.historyExhausted = current.historyExhausted;
                current.snapshot.error.clear();
                current.snapshot.updatedAt = QDateTime::currentMSecsSinceEpoch();
            }
            catch (const std::exception& error)
            {
                current.snapshot.error = QString::fromUtf8(error.what());
                current.historyRetry = QDateTime::currentMSecsSinceEpoch() + 5000;
            }
            watcher->deleteLater();
        });
        watcher->setFuture(exchangeApi(key.instrument.exchange).fetchCandleHistory(key.instrument, key.timeframe, timeZone, cursor));
    }

    void CandlesManager::fetchNext()
    {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        int pending = 0;
        std::map<SeriesKey, Subscription>::iterator next = subscriptions.end();
        for (std::map<SeriesKey, Subscription>::iterator entry = subscriptions.begin(); entry != subscriptions.end(); entry++)
        {
            if (!entry->second.stream && nextStreamStart <= now)
            {
                ensureStream(entry->first, entry->second);
                if (entry->second.stream)
                    nextStreamStart = now + 500;
            }
            if (entry->second.historyTarget != 0 && !entry->second.historyWatcher && entry->second.historyRetry <= now)
            {
                fetchHistory(entry->first, entry->second);
                return;
            }
            if (entry->second.watcher)
                pending++;
            else if (entry->second.nextRequest <= now &&
                     (next == subscriptions.end() || entry->second.nextRequest < next->second.nextRequest))
                next = entry;
        }
        if (pending < 4 && next != subscriptions.end())
            fetchCandles(next->first, next->second);
    }

    void CandlesManager::fetchCandles(const SeriesKey& key, Subscription& subscription)
    {
        QFutureWatcher<std::vector<trttrade::CandleData>>* watcher = new QFutureWatcher<std::vector<trttrade::CandleData>>(this);
        subscription.watcher = watcher;
        connect(watcher, &QFutureWatcherBase::finished, this, [this, key, watcher]
        {
            const std::map<SeriesKey, Subscription>::iterator entry = subscriptions.find(key);
            if (entry == subscriptions.end() || entry->second.watcher != watcher)
                return;
            Subscription& current = entry->second;
            current.watcher = nullptr;
            try
            {
                const std::vector<trttrade::CandleData> candles = watcher->result();
                if (candles.empty())
                    throw std::runtime_error("OKX returned no candles");
                mergeCandles(current, candles);
                current.snapshot.error.clear();
                current.snapshot.updatedAt = QDateTime::currentMSecsSinceEpoch();
                current.nextRequest = current.stream ? std::numeric_limits<qint64>::max() : current.snapshot.updatedAt + 10000;
            }
            catch (const std::exception& error)
            {
                current.snapshot.error = QString::fromUtf8(error.what());
                current.nextRequest = QDateTime::currentMSecsSinceEpoch() + 30000;
            }
            watcher->deleteLater();
        });
        const bool refill = subscription.snapshot.candles.empty() ||
                            QDateTime::currentMSecsSinceEpoch() - subscription.snapshot.updatedAt > 120000;
        const int limit = refill ? 300 : 3;
        watcher->setFuture(exchangeApi(key.instrument.exchange).fetchCandles(key.instrument, key.timeframe, timeZone, limit));
    }

    void CandlesManager::mergeCandles(Subscription& subscription, const std::vector<trttrade::CandleData>& candles)
    {
        // The parser supplies ascending, unique timestamps. Merge without tree allocation per bar.
        std::vector<trttrade::CandleData>& stored = subscription.snapshot.candles;
        if (candles.empty())
            return;
        const std::vector<trttrade::CandleData>::iterator first = std::lower_bound(stored.begin(), stored.end(), candles.front().timestamp,
            [](const trttrade::CandleData& candle, std::int64_t value) { return candle.timestamp < value; });
        const std::size_t prefixSize = static_cast<std::size_t>(first - stored.begin());
        std::vector<trttrade::CandleData> merged;
        merged.reserve(stored.size() - prefixSize + candles.size());
        std::size_t oldIndex = prefixSize;
        std::size_t newIndex = 0;
        while (oldIndex < stored.size() || newIndex < candles.size())
        {
            if (newIndex == candles.size() || (oldIndex < stored.size() && stored[oldIndex].timestamp < candles[newIndex].timestamp))
                merged.push_back(stored[oldIndex++]);
            else
            {
                if (oldIndex < stored.size() && stored[oldIndex].timestamp == candles[newIndex].timestamp)
                {
                    // A delayed incomplete bar must not replace an already confirmed bar.
                    if (stored[oldIndex].confirmed && !candles[newIndex].confirmed)
                        merged.push_back(stored[oldIndex]);
                    else
                        merged.push_back(candles[newIndex]);
                    oldIndex++;
                    newIndex++;
                }
                else
                    merged.push_back(candles[newIndex++]);
            }
        }
        stored.resize(prefixSize);
        stored.insert(stored.end(), merged.begin(), merged.end());
        if (subscription.readers == 0 && stored.size() > 300)
            stored.erase(stored.begin(), stored.end() - 300);
    }

    const CandleSnapshot* CandlesManager::getSnapshot(const trtdata::InstIdentifier& identifier, trttrade::CandleTimeframe timeframe) const
    {
        const std::map<SeriesKey, Subscription>::const_iterator entry = subscriptions.find(SeriesKey{identifier, timeframe});
        return entry == subscriptions.end() ? nullptr : &entry->second.snapshot;
    }

    const trttrade::CandleData* CandlesManager::getDailyCandle(const trtdata::InstIdentifier& identifier) const
    {
        const CandleSnapshot* snapshot = getSnapshot(identifier);
        if (!snapshot || snapshot->candles.empty())
            return nullptr;
        const qint64 dayMs = 86400000;
        const qint64 offsetMs = trtdata::tradingDataTimeZoneOffsetMs(timeZone);
        const qint64 today = (QDateTime::currentMSecsSinceEpoch() + offsetMs) / dayMs * dayMs - offsetMs;
        const trttrade::CandleData& candle = snapshot->candles.back();
        return candle.timestamp == today ? &candle : nullptr;
    }

    void CandlesManager::setTimeZone(trtdata::TradingDataTimeZone value)
    {
        if (timeZone == value)
            return;
        timeZone = value;
        for (std::pair<const SeriesKey, Subscription>& entry : subscriptions)
        {
            cancelRequest(entry.second);
            entry.second.snapshot = {};
            entry.second.historyTarget = 0;
            entry.second.historyExhausted = false;
            entry.second.nextRequest = 0;
        }
    }

    trtdata::TradingDataTimeZone CandlesManager::getTimeZone() const
    {
        return timeZone;
    }

    std::optional<double> CandlesManager::getDailyChangePercent(const trtdata::InstIdentifier& identifier, double latestPrice) const
    {
        const trttrade::CandleData* candle = getDailyCandle(identifier);
        if (!candle || candle->open <= 0 || !std::isfinite(latestPrice) || latestPrice <= 0)
            return std::nullopt;
        return (latestPrice - candle->open) / candle->open * 100.0;
    }
    void CandlesManager::subscribe(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        Subscription& subscription = subscriptions[SeriesKey{instrument, timeframe}];
        subscription.readers++;
        subscription.nextRequest = 0;
        requestTimer.start();
    }

    void CandlesManager::unsubscribe(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        const std::map<SeriesKey, Subscription>::iterator entry = subscriptions.find(SeriesKey{instrument, timeframe});
        if (entry == subscriptions.end())
            return;
        if (entry->second.readers != 0)
            entry->second.readers--;
        if (entry->second.tracked || entry->second.readers != 0)
            return;
        cancelRequest(entry->second);
        subscriptions.erase(entry);
        if (subscriptions.empty())
            requestTimer.stop();
    }

    std::span<const trttrade::CandleData> CandlesManager::getCandles(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe) const
    {
        const CandleSnapshot* snapshot = getSnapshot(instrument, timeframe);
        return snapshot ? std::span<const trttrade::CandleData>(snapshot->candles) : std::span<const trttrade::CandleData>();
    }

    const trttrade::CandleData* CandlesManager::getCandle(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe, std::int64_t timestamp) const
    {
        const std::span<const trttrade::CandleData> candles = getCandles(instrument, timeframe);
        const std::span<const trttrade::CandleData>::iterator entry = std::lower_bound(candles.begin(), candles.end(), timestamp,
            [](const trttrade::CandleData& candle, std::int64_t value) { return candle.timestamp < value; });
        return entry != candles.end() && entry->timestamp == timestamp ? &*entry : nullptr;
    }

    std::span<const trttrade::CandleData> CandlesManager::getCandles(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe, std::int64_t from, std::int64_t to) const
    {
        if (from >= to)
            return {};
        const std::span<const trttrade::CandleData> candles = getCandles(instrument, timeframe);
        const std::span<const trttrade::CandleData>::iterator begin = std::lower_bound(candles.begin(), candles.end(), from,
            [](const trttrade::CandleData& candle, std::int64_t value) { return candle.timestamp < value; });
        const std::span<const trttrade::CandleData>::iterator end = std::lower_bound(begin, candles.end(), to,
            [](const trttrade::CandleData& candle, std::int64_t value) { return candle.timestamp < value; });
        return {begin, end};
    }

    std::span<const trttrade::CandleData> CandlesManager::getLatestCandles(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe, std::size_t count) const
    {
        const std::span<const trttrade::CandleData> candles = getCandles(instrument, timeframe);
        return candles.last(std::min(count, candles.size()));
    }

    CandlesManager::CandleMap CandlesManager::getCandleMap(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe) const
    {
        CandleMap result;
        for (const trttrade::CandleData& candle : getCandles(instrument, timeframe))
            result.emplace_hint(result.end(), candle.timestamp, candle);
        return result;
    }

}
