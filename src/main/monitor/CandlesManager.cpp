#include "CandlesManager.h"
#include "main/util/TimeUtil.h"
#include "main/util/CandleUtil.h"
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
        connect(&requestTimer, &QTimer::timeout, this, [this]
        {
            fetchNext();
        });
    }

    void CandlesManager::setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments)
    {
        for (std::pair<const CandleSeriesKey, Subscription>& entry : subscriptions)
            entry.second.tracked = false;
        for (const std::shared_ptr<trtdata::Instrument>& instrument : instruments)
        {
            subscriptions[CandleSeriesKey{instrument->getUniqueIdentifier()}].tracked = true;
        }
        std::erase_if(subscriptions, [this](std::pair<const CandleSeriesKey, Subscription>& entry)
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
        subscription.streamReady = false;
        subscription.requestLiveCandles.clear();
        subscription.historyLiveCandles.clear();
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
        for (std::pair<const CandleSeriesKey, Subscription>& entry : subscriptions)
        {
            cancelRequest(entry.second);
            entry.second.nextRequest = 0;
        }
    }

    void CandlesManager::ensureStream(const CandleSeriesKey& key, Subscription& subscription)
    {
        if (subscription.stream)
            return;
        subscription.stream = exchangeApi(key.instrument.exchange).createCandleStream();
        if (!subscription.stream)
            return;
        connect(subscription.stream.get(), &CandleStream::candlesReceived, this,
                [this, key](const std::vector<trttrade::CandleData>& candles)
                {
                    Subscription& current = subscriptions.at(key);
                    current.publishedSnapshot.reset();
                    if (current.watcher)
                        trtutils::mergeCandles(current.requestLiveCandles, candles);
                    if (current.historyWatcher)
                        trtutils::mergeCandles(current.historyLiveCandles, candles);
                    mergeCandles(current, candles);
                    current.snapshot.updatedAt = QDateTime::currentMSecsSinceEpoch();
                });
        connect(subscription.stream.get(), &CandleStream::connectionChanged, this, [this, key](const QString& error)
        {
            Subscription& current = subscriptions.at(key);
            current.publishedSnapshot.reset();
            current.streamReady = error.isEmpty();
            current.streamError = error;
            current.snapshot.error = error;
            beginRecovery(current);
        });
        subscription.stream->start(key.instrument, key.timeframe, timeZone);
    }

    void CandlesManager::beginRecovery(Subscription& subscription)
    {
        if (!subscription.recoveryFrom && !subscription.snapshot.candles.empty())
            subscription.recoveryFrom = subscription.snapshot.candles.back().timestamp;
        subscription.recoveryCursor.reset();
        subscription.recoveryGeneration++;
        subscription.refill = true;
        subscription.nextRequest = 0;
    }

    void CandlesManager::requestHistory(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        const std::map<CandleSeriesKey, Subscription>::iterator entry = subscriptions.find({instrument, timeframe});
        if (entry == subscriptions.end() || entry->second.historyExhausted || entry->second.snapshot.candles.empty())
            return;
        entry->second.historyTarget = entry->second.snapshot.candles.front().timestamp;
        entry->second.snapshot.historyLoading = true;
        entry->second.publishedSnapshot.reset();
    }

    void CandlesManager::fetchHistory(const CandleSeriesKey& key, Subscription& subscription)
    {
        if (subscription.snapshot.candles.empty())
            return;
        const qint64 cursor = subscription.snapshot.candles.front().timestamp;
        QFutureWatcher<std::vector<trttrade::CandleData>>* watcher = new QFutureWatcher<std::vector<
            trttrade::CandleData>>(this);
        subscription.historyWatcher = watcher;
        subscription.historyLiveCandles.clear();
        connect(watcher, &QFutureWatcherBase::finished, this, [this, key, watcher, cursor]
        {
            Subscription& current = subscriptions.at(key);
            current.publishedSnapshot.reset();
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
                mergeCandles(current, current.historyLiveCandles);
                current.historyTarget = 0;
                current.snapshot.historyLoading = false;
                current.snapshot.historyExhausted = current.historyExhausted;
                current.snapshot.error = current.streamError;
                current.snapshot.updatedAt = QDateTime::currentMSecsSinceEpoch();
            } catch (const std::exception& error)
            {
                current.snapshot.error = QString::fromUtf8(error.what());
                current.historyRetry = QDateTime::currentMSecsSinceEpoch() + 5000;
            }
            current.historyLiveCandles.clear();
            watcher->deleteLater();
        });
        watcher->setFuture(
            exchangeApi(key.instrument.exchange).fetchCandleHistory(key.instrument, key.timeframe, timeZone, cursor));
    }

    void CandlesManager::fetchNext()
    {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        int pending = 0;
        std::map<CandleSeriesKey, Subscription>::iterator next = subscriptions.end();
        for (std::map<CandleSeriesKey, Subscription>::iterator entry = subscriptions.begin(); entry != subscriptions.end();
             entry++)
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

    void CandlesManager::fetchCandles(const CandleSeriesKey& key, Subscription& subscription)
    {
        const std::optional<qint64> cursor = subscription.recoveryCursor;
        const std::uint64_t generation = subscription.recoveryGeneration;
        QFutureWatcher<std::vector<trttrade::CandleData>>* watcher = new QFutureWatcher<std::vector<
            trttrade::CandleData>>(this);
        subscription.watcher = watcher;
        subscription.requestLiveCandles.clear();
        connect(watcher, &QFutureWatcherBase::finished, this, [this, key, watcher, cursor, generation]
        {
            const std::map<CandleSeriesKey, Subscription>::iterator entry = subscriptions.find(key);
            if (entry == subscriptions.end() || entry->second.watcher != watcher)
                return;
            Subscription& current = entry->second;
            current.publishedSnapshot.reset();
            current.watcher = nullptr;
            try
            {
                const std::vector<trttrade::CandleData> candles = watcher->result();
                if (candles.empty())
                    throw std::runtime_error(cursor ? "Recovery endpoint returned no candles before the gap was filled" : "OKX returned no candles");
                if (cursor && !candles.empty() && candles.front().timestamp >= *cursor)
                    throw std::runtime_error("Recovery endpoint did not advance the pagination cursor");
                mergeCandles(current, candles);
                // Replay updates received after this request began, including across bar boundaries.
                mergeCandles(current, current.requestLiveCandles);
                current.snapshot.error = current.streamError;
                current.snapshot.updatedAt = QDateTime::currentMSecsSinceEpoch();
                if (generation == current.recoveryGeneration)
                {
                    current.refill = false;
                    if (current.recoveryFrom && !candles.empty() && candles.front().timestamp > *current.recoveryFrom)
                    {
                        current.recoveryCursor = candles.front().timestamp;
                        current.nextRequest = 0;
                    } else
                    {
                        current.recoveryFrom.reset();
                        current.recoveryCursor.reset();
                        current.nextRequest = current.streamReady ? std::numeric_limits<qint64>::max() : current.snapshot.updatedAt + 10000;
                    }
                }
            } catch (const std::exception& error)
            {
                current.snapshot.error = QString::fromUtf8(error.what());
                if (generation == current.recoveryGeneration)
                    current.nextRequest = QDateTime::currentMSecsSinceEpoch() + 30000;
            }
            current.requestLiveCandles.clear();
            watcher->deleteLater();
        });
        const AbstractApi& api = exchangeApi(key.instrument.exchange);
        if (cursor)
            watcher->setFuture(api.fetchCandleHistory(key.instrument, key.timeframe, timeZone, *cursor));
        else
            watcher->setFuture(api.fetchCandles(key.instrument, key.timeframe, timeZone, subscription.refill ? 300 : 3));
    }

    void CandlesManager::mergeCandles(Subscription& subscription, const std::vector<trttrade::CandleData>& candles)
    {
        subscription.publishedSnapshot.reset();
        if (candles.empty())
            return;
        trtutils::mergeCandles(subscription.snapshot.candles, candles);
        if (subscription.readers == 0 && subscription.snapshot.candles.size() > 300)
            subscription.snapshot.candles.erase(subscription.snapshot.candles.begin(),
                                                subscription.snapshot.candles.end() - 300);
    }

    const CandleSnapshot* CandlesManager::getSnapshot(const trtdata::InstIdentifier& identifier,
                                                      trttrade::CandleTimeframe timeframe) const
    {
        const std::map<CandleSeriesKey, Subscription>::const_iterator entry = subscriptions.find(CandleSeriesKey{
            identifier,
            timeframe
        });
        return entry == subscriptions.end() ? nullptr : &entry->second.snapshot;
    }

    const trttrade::CandleData* CandlesManager::getDailyCandle(const trtdata::InstIdentifier& identifier) const
    {
        const CandleSnapshot* snapshot = getSnapshot(identifier);
        if (!snapshot)
            return nullptr;
        return trtutils::dailyCandle(snapshot->candles, QDateTime::currentMSecsSinceEpoch(), timeZone);
    }

    void CandlesManager::setTimeZone(trtdata::TradingTimeZone value)
    {
        if (timeZone == value)
            return;
        timeZone = value;
        for (std::pair<const CandleSeriesKey, Subscription>& entry : subscriptions)
        {
            cancelRequest(entry.second);
            entry.second.snapshot = {};
            entry.second.publishedSnapshot.reset();
            entry.second.historyTarget = 0;
            entry.second.historyExhausted = false;
            entry.second.historyRetry = 0;
            entry.second.streamError.clear();
            entry.second.recoveryFrom.reset();
            beginRecovery(entry.second);
        }
    }

    trtdata::TradingTimeZone CandlesManager::getTimeZone() const
    {
        return timeZone;
    }

    std::optional<double> CandlesManager::getDailyChangePercent(const trtdata::InstIdentifier& identifier,
                                                                double latestPrice) const
    {
        const trttrade::CandleData* candle = getDailyCandle(identifier);
        return trtutils::dailyChangePercent(candle, latestPrice);
    }

    void CandlesManager::subscribe(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        Subscription& subscription = subscriptions[CandleSeriesKey{instrument, timeframe}];
        subscription.readers++;
        subscription.nextRequest = 0;
        requestTimer.start();
    }

    void CandlesManager::unsubscribe(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe)
    {
        const std::map<CandleSeriesKey, Subscription>::iterator entry = subscriptions.find(CandleSeriesKey{instrument, timeframe});
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

    std::span<const trttrade::CandleData> CandlesManager::getCandles(const trtdata::InstIdentifier& instrument,
                                                                     trttrade::CandleTimeframe timeframe) const
    {
        const CandleSnapshot* snapshot = getSnapshot(instrument, timeframe);
        return snapshot
                   ? std::span<const trttrade::CandleData>(snapshot->candles)
                   : std::span<const trttrade::CandleData>();
    }

    const trttrade::CandleData* CandlesManager::getCandle(const trtdata::InstIdentifier& instrument,
                                                          trttrade::CandleTimeframe timeframe,
                                                          std::int64_t timestamp) const
    {
        const std::span<const trttrade::CandleData> candles = getCandles(instrument, timeframe);
        const std::span<const trttrade::CandleData>::iterator entry = trtutils::candleLowerBound(candles, timestamp);
        return entry != candles.end() && entry->timestamp == timestamp ? &*entry : nullptr;
    }

    std::span<const trttrade::CandleData> CandlesManager::getCandles(const trtdata::InstIdentifier& instrument,
                                                                     trttrade::CandleTimeframe timeframe,
                                                                     std::int64_t from, std::int64_t to) const
    {
        if (from >= to)
            return {};
        const std::span<const trttrade::CandleData> candles = getCandles(instrument, timeframe);
        const std::span<const trttrade::CandleData>::iterator begin = trtutils::candleLowerBound(candles, from);
        const std::span<const trttrade::CandleData>::iterator end = trtutils::candleLowerBound(
            {begin, candles.end()}, to);
        return {begin, end};
    }

    std::span<const trttrade::CandleData> CandlesManager::getLatestCandles(
        const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe, std::size_t count) const
    {
        const std::span<const trttrade::CandleData> candles = getCandles(instrument, timeframe);
        return candles.last(std::min(count, candles.size()));
    }

    CandlesManager::CandleMap CandlesManager::getCandleMap(const trtdata::InstIdentifier& instrument,
                                                           trttrade::CandleTimeframe timeframe) const
    {
        CandleMap result;
        for (const trttrade::CandleData& candle : getCandles(instrument, timeframe))
            result.emplace_hint(result.end(), candle.timestamp, candle);
        return result;
    }

    CandlesSnapshot CandlesManager::getReadSnapshot()
    {
        CandlesSnapshot result;
        result.timeZone = timeZone;
        for (std::pair<const CandleSeriesKey, Subscription>& entry : subscriptions)
        {
            if (!entry.second.publishedSnapshot)
                entry.second.publishedSnapshot = std::make_shared<const CandleSnapshot>(entry.second.snapshot);
            result.series.emplace(entry.first, entry.second.publishedSnapshot);
        }
        return result;
    }

}
