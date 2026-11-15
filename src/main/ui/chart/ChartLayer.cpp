#include "ChartLayer.h"
#include "main/util/PaintUtil.h"
#include <algorithm>

namespace trtui
{
    void CandlestickLayer::build(const ChartLayerContext& context, std::vector<ChartVertex>& vertices) const
    {
        const ChartViewport& viewport = context.viewport;
        const double bodyWidth = std::max(context.pixelSize, viewport.getBarWidth() * 0.72);
        const std::size_t end = std::min(context.candles.size(), viewport.getLastVisible());
        for (std::size_t index = viewport.getFirstVisible(); index < end; index++)
        {
            const trttrade::CandleData& candle = context.candles[index];
            const QColor& color = candle.close >= candle.open ? context.rising : context.falling;
            const double x = viewport.xFromIndex(static_cast<double>(index));
            const double high = viewport.yFromPrice(candle.high);
            const double low = viewport.yFromPrice(candle.low);
            const double open = viewport.yFromPrice(candle.open);
            const double close = viewport.yFromPrice(candle.close);
            trtutils::appendClippedRect(
                vertices, QRectF(x - context.pixelSize / 2, high, context.pixelSize,
                                 std::max(context.pixelSize, low - high)), color, viewport.getPriceRect());
            trtutils::appendClippedRect(
                vertices, QRectF(x - bodyWidth / 2, std::min(open, close), bodyWidth,
                                 std::max(context.pixelSize, std::abs(open - close))), color, viewport.getPriceRect());
        }
    }

    void VolumeLayer::build(const ChartLayerContext& context, std::vector<ChartVertex>& vertices) const
    {
        const ChartViewport& viewport = context.viewport;
        if (!viewport.isVolumeVisible())
            return;
        const double bodyWidth = std::max(context.pixelSize, viewport.getBarWidth() * 0.72);
        const std::size_t end = std::min(context.candles.size(), viewport.getLastVisible());
        for (std::size_t index = viewport.getFirstVisible(); index < end; index++)
        {
            const trttrade::CandleData& candle = context.candles[index];
            QColor color = candle.close >= candle.open ? context.rising : context.falling;
            color.setAlphaF(0.5);
            const double x = viewport.xFromIndex(static_cast<double>(index));
            const double y = viewport.yFromVolume(candle.volume);
            trtutils::appendClippedRect(
                vertices, QRectF(x - bodyWidth / 2, y, bodyWidth, viewport.getVolumeRect().bottom() - y), color,
                viewport.getVolumeRect());
        }
    }
}
