#ifndef NTRT_DATASNAPSHOTDEF_H
#define NTRT_DATASNAPSHOTDEF_H

#include "main/trade/TradeSnapshotDef.h"
#include "main/trade/AccountSnapshotDef.h"

namespace trtcore
{
    struct DataSnapshot
    {
        MarketDataSnapshot marketData;
        CandlesSnapshot candles;
        AccountSnapshot account;
    };
}

#endif //NTRT_DATASNAPSHOTDEF_H
