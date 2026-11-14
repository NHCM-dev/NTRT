//
// Created by NHCM on 2026/11/14.
//

#ifndef NTRT_TRADEDATADEF_H
#define NTRT_TRADEDATADEF_H
#include <cstdint>

namespace trttrade
{
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

#endif //NTRT_TRADEDATADEF_H
