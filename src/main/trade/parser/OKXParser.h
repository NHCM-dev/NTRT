//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_OKXPARSER_H
#define NTRT_OKXPARSER_H

#include "main/market/abstract/Instrument.h"
#include "main/trade/TradeDataDef.h"
#include <QByteArray>
#include "main/trade/CandleTimeframe.h"
#include "main/data/TradingDataConfig.h"

#include "main/util/JsonUtil.h"

namespace trtdata::OKXParser
{
    QString candleBar(trttrade::CandleTimeframe timeframe, TradingDataTimeZone timeZone);

    std::string_view getInstType(trttrade::InstType type);

    trttrade::InstType readType(std::string_view type);

    void ensureSuccess(const elementResult& json);

    jsonArray getDataField(const elementResult& json);

    const TickerData parseTickerFromElement(const jsonElement& json);

    const TickerData parseTicker(const QByteArray& json);

    const std::shared_ptr<std::vector<TickerData>> parseTickers(const QByteArray& json);

    std::vector<trttrade::CandleData> parseCandles(const QByteArray& json);
} // trtdata

#endif //NTRT_OKXPARSER_H
