#include "CandleUtil.h"
#include <algorithm>
#include <cmath>

namespace trtutils
{
    std::span<const trttrade::CandleData>::iterator candleLowerBound(std::span<const trttrade::CandleData> candles,
                                                                     std::int64_t timestamp)
    {
        return std::lower_bound(candles.begin(), candles.end(), timestamp,
                                [](const trttrade::CandleData& candle, std::int64_t value)
                                {
                                    return candle.timestamp < value;
                                });
    }

    void mergeCandles(std::vector<trttrade::CandleData>& stored, std::span<const trttrade::CandleData> candles)
    {
        // The parser supplies ascending, unique timestamps. Merge without tree allocation per bar.
        if (candles.empty())
            return;
        const std::vector<trttrade::CandleData>::iterator first = std::lower_bound(
            stored.begin(), stored.end(), candles.front().timestamp,
            [](const trttrade::CandleData& candle, std::int64_t value)
            {
                return candle.timestamp < value;
            });
        const std::size_t prefixSize = static_cast<std::size_t>(first - stored.begin());
        std::vector<trttrade::CandleData> merged;
        merged.reserve(stored.size() - prefixSize + candles.size());
        std::size_t oldIndex = prefixSize;
        std::size_t newIndex = 0;
        while (oldIndex < stored.size() || newIndex < candles.size())
        {
            if (newIndex == candles.size() || (oldIndex < stored.size() && stored[oldIndex].timestamp < candles[
                newIndex].timestamp))
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
                } else
                    merged.push_back(candles[newIndex++]);
            }
        }
        stored.resize(prefixSize);
        stored.insert(stored.end(), merged.begin(), merged.end());
    }

    const trttrade::CandleData* dailyCandle(std::span<const trttrade::CandleData> candles, qint64 now,
                                            trtdata::TradingDataTimeZone zone)
    {
        if (candles.empty() || candles.back().timestamp != startOfTradingDay(now, zone))
            return nullptr;
        return &candles.back();
    }

    std::optional<double> dailyChangePercent(const trttrade::CandleData* candle, double latestPrice)
    {
        if (!candle || candle->open <= 0 || !std::isfinite(latestPrice) || latestPrice <= 0)
            return std::nullopt;
        return (latestPrice - candle->open) / candle->open * 100.0;
    }

    double baseVolume(const trttrade::CandleData& candle, trttrade::InstType type)
    {
        switch (type)
        {
            case trttrade::InstType::Spot:
            case trttrade::InstType::Margin:
                return candle.volume;
            case trttrade::InstType::Swap:
            case trttrade::InstType::Futures:
            case trttrade::InstType::Option:
            case trttrade::InstType::Events:
                return candle.volumeCcy;
        }
        return 0;
    }
}
