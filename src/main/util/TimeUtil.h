#ifndef NTRT_TIMEUTIL_H
#define NTRT_TIMEUTIL_H

#include "main/data/TradingDataDef.h"
#include <QString>

namespace trtdata
{
    constexpr std::string_view tradingDataTimeZoneName(TradingDataTimeZone timeZone)
    {
        switch (timeZone)
        {
            case TradingDataTimeZone::UTC:
                return "UTC";
            case TradingDataTimeZone::UTC8:
                return "UTC+8";
        }
        return {};
    }

    constexpr std::int64_t tradingDataTimeZoneOffsetMs(TradingDataTimeZone timeZone)
    {
        switch (timeZone)
        {
            case TradingDataTimeZone::UTC:
                return 0;
            case TradingDataTimeZone::UTC8:
                return 8 * 3600000;
        }
        return 0;
    }
}

namespace trtutils
{
    qint64 startOfTradingDay(qint64 timestamp, trtdata::TradingDataTimeZone zone);

    QString formatTradingTime(qint64 timestamp, trtdata::TradingDataTimeZone zone, const QString& format);
}

#endif //NTRT_TIMEUTIL_H
