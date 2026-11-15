#ifndef NTRT_CHARTLAYER_H
#define NTRT_CHARTLAYER_H

#include "ChartViewport.h"
#include "main/util/PaintUtil.h"
#include <QColor>
#include <vector>

namespace trtui
{
    struct ChartLayerContext
    {
        const ChartViewport& viewport;
        std::span<const trttrade::CandleData> candles;
        QColor rising;
        QColor falling;
        double pixelSize;
    };

    // Layers only emit geometry in logical pixels. Indicators and drawing tools can use the same transforms.
    class ChartLayer
    {
    public:
        virtual ~ChartLayer() = default;

        virtual void build(const ChartLayerContext& context, std::vector<ChartVertex>& vertices) const = 0;
    };

    class CandlestickLayer final : public ChartLayer
    {
    public:
        void build(const ChartLayerContext& context, std::vector<ChartVertex>& vertices) const override;
    };

    class VolumeLayer final : public ChartLayer
    {
    public:
        void build(const ChartLayerContext& context, std::vector<ChartVertex>& vertices) const override;
    };
}
#endif
