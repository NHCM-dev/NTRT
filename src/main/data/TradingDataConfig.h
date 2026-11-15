#ifndef NTRT_TRADINGDATACONFIG_H
#define NTRT_TRADINGDATACONFIG_H

#include <string>
#include <cstdint>
#include <string_view>

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

#endif //NTRT_TRADINGDATACONFIG_H
