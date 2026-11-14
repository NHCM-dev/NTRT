#ifndef NTRT_TRADINGDATACONFIG_H
#define NTRT_TRADINGDATACONFIG_H

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
    };

    constexpr std::string_view tradingDataTimeZoneName(TradingDataTimeZone timeZone)
    {
        switch (timeZone)
        {
        case TradingDataTimeZone::UTC: return "UTC";
        case TradingDataTimeZone::UTC8: return "UTC+8";
        }
        return {};
    }

    constexpr std::int64_t tradingDataTimeZoneOffsetMs(TradingDataTimeZone timeZone)
    {
        switch (timeZone)
        {
        case TradingDataTimeZone::UTC: return 0;
        case TradingDataTimeZone::UTC8: return 8 * 3600000;
        }
        return 0;
    }
}

#endif //NTRT_TRADINGDATACONFIG_H
