#ifndef NTRT_TRADINGDATADEF_H
#define NTRT_TRADINGDATADEF_H

#include <string>

namespace trtdata
{
    enum class TradingDataTimeZone
    {
        UTC,
        UTC8
    };

    struct TradingDataConfig
    {
        TradingDataTimeZone timeZone = TradingDataTimeZone::UTC;
        std::string displayTimeZone = "UTC";
    };
}

#endif //NTRT_TRADINGDATADEF_H
