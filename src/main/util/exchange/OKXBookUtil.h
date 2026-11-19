#ifndef NTRT_OKXBOOKUTIL_H
#define NTRT_OKXBOOKUTIL_H

#include "main/util/JsonUtil.h"
#include "main/trade/TradeSnapshotDef.h"

namespace trtdata::okx
{
    // Parse into temporary state. Sequence gaps and malformed rows leave the old book untouched.
    bool applyBookMessage(const jsonElement& row, bool snapshot, trttrade::BookSnapshot& book);
}
#endif
