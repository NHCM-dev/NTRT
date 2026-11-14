#ifndef NTRT_CANDLEDATA_H
#define NTRT_CANDLEDATA_H

#include <cstdint>

namespace trtdata
{
    struct CandleData
    {
        std::int64_t ts = 0;
        double open = 0;
        double high = 0;
        double low = 0;
        double close = 0;
        double volume = 0;
        double volumeCurrency = 0;
        double volumeQuote = 0;
        bool confirmed = false;
    };
}

#endif //NTRT_CANDLEDATA_H
