#ifndef NTRT_OKXUTIL_H
#define NTRT_OKXUTIL_H

#include "main/trade/CandleTimeframe.h"
#include "main/data/TradingDataConfig.h"
#include <QString>

namespace trtdata::okx
{
    QString candleBar(trttrade::CandleTimeframe timeframe, TradingDataTimeZone timeZone);
}

#endif //NTRT_OKXUTIL_H
