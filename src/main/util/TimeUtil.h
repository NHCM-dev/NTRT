#ifndef NTRT_TIMEUTIL_H
#define NTRT_TIMEUTIL_H

#include "../core/TradingTimeDef.h"
#include <QString>

namespace trtdata
{
    constexpr std::string_view tradingDataTimeZoneName(TradingTimeZone timeZone)
    {
        switch (timeZone)
        {
            case TradingTimeZone::UTC:
                return "UTC";
            case TradingTimeZone::UTC8:
                return "UTC+8";
        }
        return {};
    }

    constexpr std::int64_t tradingDataTimeZoneOffsetMs(TradingTimeZone timeZone)
    {
        switch (timeZone)
        {
            case TradingTimeZone::UTC:
                return 0;
            case TradingTimeZone::UTC8:
                return 8 * 3600000;
        }
        return 0;
    }
}

namespace trtutils
{
    qint64 startOfTradingDay(qint64 timestamp, trtdata::TradingTimeZone zone);

    QString formatTradingTime(qint64 timestamp, trtdata::TradingTimeZone zone, const QString& format);
}

#endif //NTRT_TIMEUTIL_H
