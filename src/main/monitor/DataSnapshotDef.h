#ifndef NTRT_DATASNAPSHOTDEF_H
#define NTRT_DATASNAPSHOTDEF_H

#include "main/trade/TradeSnapshotDef.h"

namespace trtcore
{
    struct DataSnapshot
    {
        MarketDataSnapshot marketData;
        CandlesSnapshot candles;
    };
}

#endif //NTRT_DATASNAPSHOTDEF_H
