#include "TimeUtil.h"
#include <QDateTime>
#include <QTimeZone>

namespace trtutils
{
    qint64 startOfTradingDay(qint64 timestamp, trtdata::TradingDataTimeZone zone)
    {
        const qint64 dayMs = 86400000;
        const qint64 offsetMs = trtdata::tradingDataTimeZoneOffsetMs(zone);
        return (timestamp + offsetMs) / dayMs * dayMs - offsetMs;
    }

    QString formatTradingTime(qint64 timestamp, trtdata::TradingDataTimeZone zone, const QString& format)
    {
        return QDateTime::fromMSecsSinceEpoch(timestamp, QTimeZone::UTC).addMSecs(
            trtdata::tradingDataTimeZoneOffsetMs(zone)).toString(format);
    }
}
