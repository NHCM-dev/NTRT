#ifndef NTRT_CHARTDEF_H
#define NTRT_CHARTDEF_H

#include "main/trade/TradeDataDef.h"
#include <QColor>
#include <span>
#include "main/ui/chart/ChartViewport.h"

namespace trtui
{
    struct ChartVertex
    {
        float x;
        float y;
        float red;
        float green;
        float blue;
        float alpha;
    };

    struct ChartLayerContext
    {
        const ChartViewport& viewport;
        std::span<const trttrade::CandleData> candles;
        QColor rising;
        QColor falling;
        double pixelSize;
    };
}

#endif //NTRT_CHARTDEF_H
