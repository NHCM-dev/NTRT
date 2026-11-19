#ifndef NTRT_TRADINGCONFIGDEF_H
#define NTRT_TRADINGCONFIGDEF_H

#include "main/data/core/TradingTimeDef.h"
#include <string>

namespace trtdata
{
    struct TradingConfig
    {
        TradingTimeZone timeZone = TradingTimeZone::UTC;
        std::string displayTimeZone = "UTC";
    };
}

#endif //NTRT_TRADINGCONFIGDEF_H
