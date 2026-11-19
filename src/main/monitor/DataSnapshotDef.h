#ifndef NTRT_DATASNAPSHOTDEF_H
#define NTRT_DATASNAPSHOTDEF_H

#include "main/trade/TradeSnapshotDef.h"
#include "main/trade/AccountSnapshotDef.h"
#include <QHash>
#include <QImage>
#include <QString>
#include <memory>

namespace trtcore
{
    struct DataSnapshot
    {
        std::shared_ptr<const MarketDataSnapshot> marketData = std::make_shared<const MarketDataSnapshot>();
        std::shared_ptr<const CandlesSnapshot> candles = std::make_shared<const CandlesSnapshot>();
        std::shared_ptr<const AccountSnapshot> account = std::make_shared<const AccountSnapshot>();
        std::shared_ptr<const QHash<QString, QImage>> icons = std::make_shared<const QHash<QString, QImage>>();
    };
}

#endif //NTRT_DATASNAPSHOTDEF_H
