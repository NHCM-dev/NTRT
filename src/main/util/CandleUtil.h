#ifndef NTRT_CANDLEUTIL_H
#define NTRT_CANDLEUTIL_H

#include "main/trade/TradeDataDef.h"
#include "main/trade/TradeDef.h"
#include "TimeUtil.h"
#include <optional>
#include <span>
#include <vector>

namespace trtutils
{
    std::span<const trttrade::CandleData>::iterator candleLowerBound(std::span<const trttrade::CandleData> candles,
                                                                     std::int64_t timestamp);

    // Inputs are sorted by unique ascending timestamps; confirmed bars take precedence.
    void mergeCandles(std::vector<trttrade::CandleData>& stored, std::span<const trttrade::CandleData> candles);

    const trttrade::CandleData* dailyCandle(std::span<const trttrade::CandleData> candles, qint64 now,
                                            trtdata::TradingDataTimeZone zone);

    std::optional<double> dailyChangePercent(const trttrade::CandleData* candle, double latestPrice);

    double baseVolume(const trttrade::CandleData& candle, trttrade::InstType type);
}

#endif //NTRT_CANDLEUTIL_H
