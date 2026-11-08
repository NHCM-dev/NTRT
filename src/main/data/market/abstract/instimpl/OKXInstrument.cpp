//
// Created by NHCM on 2026/11/7.
//

#include "OKXInstrument.h"
#include "main/data/api/OKXApi.h"
#include "main/data/parser/OKXParser.h"

namespace trtdata::okx
{
    QFuture<TickerData> OKXInstrument::requestTicker()
    {
        return OKXApi::requestTicker(instId);
    }

    std::string OKXInstrument::getInstTypeStr() const
    {
        return std::string(OKXParser::getInstType(instType));
    }
} // trtdata
