//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_OKXPARSER_H
#define NTRT_OKXPARSER_H

#include "main/data/market/abstract/Instrument.h"
#include <QByteArray>

#include "main/util/JsonUtil.h"

namespace trtdata::OKXParser
{
    std::string_view getInstType(InstType type);

    InstType readType(std::string_view type);

    void ensureSuccess(const elementResult& json);
    jsonArray getDataField(const elementResult& json);

    const TickerData parseTickerFromElement(const jsonElement& json);
    const TickerData parseTicker(const QByteArray& json);
    const std::shared_ptr<std::vector<TickerData>> parseTickers(const QByteArray& json);
} // trtdata

#endif //NTRT_OKXPARSER_H
