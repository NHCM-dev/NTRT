//
// Created by NHCM on 2026/11/14.
//

#ifndef NTRT_TRADEDATADEF_H
#define NTRT_TRADEDATADEF_H
#include "TradeDef.h"
#include <cstdint>
#include <optional>

namespace trttrade
{
    struct BookLevel
    {
        double price = 0;
        double size = 0;
    };

    struct CandleData
    {
        std::int64_t timestamp = 0;
        double open = 0;
        double high = 0;
        double low = 0;
        double close = 0;
        double volume = 0;
        double volumeCcy = 0;
        double volumeQuote = 0;
        bool confirmed = false;
    };
}

namespace trtdata
{
    struct TickerData
    {
        trttrade::InstType instType;
        std::string instId;
        double last;
        double open24h;
        double high24h;
        double low24h;
        double volCcy24h;
        double vol24h;
        std::int64_t ts;
        std::optional<double> previousLast{};
    };
}

#endif //NTRT_TRADEDATADEF_H
