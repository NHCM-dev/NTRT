#include "ChartViewport.h"
#include "main/util/NumberUtil.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace trtui
{
    void ChartViewport::setSize(const QSize& size, int headerHeight)
    {
        readoutHeight = std::max(1, headerHeight);
        const double width = std::max(1, size.width() - 104);
        readoutRect = QRectF(10, 2, width, readoutHeight);
        const double top = readoutRect.bottom() + 4;
        const double available = std::max(1.0, size.height() - 30.0 - top);
        const double separator = volumeVisible ? readoutHeight + 12.0 : 0;
        const double plotHeight = std::max(2.0, available - separator);
        const double volumeHeight = volumeVisible ? plotHeight * 0.25 : 0;
        priceRect = QRectF(10, top, width, std::max(1.0, plotHeight - volumeHeight));
        volumeReadoutRect = QRectF(10, priceRect.bottom() + 8, width, readoutHeight);
        volumeRect = QRectF(10, volumeReadoutRect.bottom() + 4, width, volumeHeight);
        constrain();
    }

    void ChartViewport::setSeriesSize(std::size_t count, std::size_t removedPrefix)
    {
        seriesSize = count;
        firstBar -= static_cast<double>(removedPrefix);
        constrain();
    }

    void ChartViewport::setVolumeVisible(bool visible, const QSize& size)
    {
        volumeVisible = visible;
        setSize(size, readoutHeight);
    }

    void ChartViewport::constrain()
    {
        const double capacity = priceRect.width() / barWidth;
        if (seriesSize == 0)
        {
            firstBar = 0;
            return;
        }
        const double retained = std::min({2.0, static_cast<double>(seriesSize), capacity});
        const double earliest = retained - capacity;
        const double latest = static_cast<double>(seriesSize) - retained;
        if (following)
            firstBar = std::clamp(std::max(-0.5, static_cast<double>(seriesSize) - capacity + 5), earliest, latest);
        else
            firstBar = std::clamp(firstBar, earliest, latest);
    }

    void ChartViewport::updateRange(std::span<const trttrade::CandleData> candles)
    {
        double minimum = std::numeric_limits<double>::max();
        double maximum = std::numeric_limits<double>::lowest();
        volumeMax = 0;
        const std::size_t end = std::min(candles.size(), getLastVisible());
        for (std::size_t index = getFirstVisible(); index < end; index++)
        {
            minimum = std::min(minimum, candles[index].low);
            maximum = std::max(maximum, candles[index].high);
            volumeMax = std::max(volumeMax, candles[index].volume);
        }
        if ((autoScale || !priceRangeInitialized) && minimum <= maximum)
        {
            const double padding = std::max((maximum - minimum) * 0.08, std::max(std::abs(maximum) * 0.0001, 1e-12));
            priceMin = minimum - padding;
            priceMax = maximum + padding;
            priceRangeInitialized = true;
        }
        volumeMax = std::max(volumeMax * 1.15, 1e-12);
    }

    void ChartViewport::setAutoScale(bool enabled)
    {
        autoScale = enabled;
    }

    void ChartViewport::pan(double pixels)
    {
        following = false;
        firstBar -= pixels / barWidth;
        constrain();
    }

    void ChartViewport::panPrice(double pixels)
    {
        if (autoScale || !std::isfinite(pixels) || pixels == 0)
            return;
        const double shift = pixels / priceRect.height() * (priceMax - priceMin);
        const double minimum = priceMin + shift;
        const double maximum = priceMax + shift;
        if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum >= maximum)
            return;
        priceMin = minimum;
        priceMax = maximum;
    }

    void ChartViewport::zoom(double factor, double anchorX)
    {
        const double anchor = indexFromX(anchorX);
        following = false;
        barWidth = std::clamp(barWidth * factor, 2.0, 48.0);
        firstBar = anchor - (anchorX - priceRect.left()) / barWidth + 0.5;
        constrain();
    }

    void ChartViewport::scalePrice(double factor)
    {
        if (!std::isfinite(factor) || factor <= 0 || factor == 1)
            return;
        const double range = priceMax - priceMin;
        const double center = priceMin + range * 0.5;
        const double minimum = std::max(std::abs(center) * 1e-10, 1e-12);
        const double maximum = std::max(std::abs(center) * 1e6, 1e6);
        const double halfRange = std::clamp(range / factor, minimum, maximum) * 0.5;
        priceMin = center - halfRange;
        priceMax = center + halfRange;
        autoScale = false;
        priceRangeInitialized = true;
    }

    void ChartViewport::reset()
    {
        barWidth = 8;
        autoScale = true;
        priceRangeInitialized = false;
        priceMin = 0;
        priceMax = 1;
        followLatest();
    }

    void ChartViewport::followLatest()
    {
        following = true;
        constrain();
    }

    bool ChartViewport::isFollowing() const
    {
        return following;
    }

    bool ChartViewport::isVolumeVisible() const
    {
        return volumeVisible;
    }

    bool ChartViewport::isAutoScale() const
    {
        return autoScale;
    }

    const QRectF& ChartViewport::getReadoutRect() const { return readoutRect; }
    const QRectF& ChartViewport::getVolumeReadoutRect() const { return volumeReadoutRect; }

    const QRectF& ChartViewport::getPriceRect() const
    {
        return priceRect;
    }

    const QRectF& ChartViewport::getVolumeRect() const
    {
        return volumeRect;
    }

    double ChartViewport::getBarWidth() const
    {
        return barWidth;
    }

    double ChartViewport::getPriceMin() const
    {
        return priceMin;
    }

    double ChartViewport::getPriceMax() const
    {
        return priceMax;
    }

    double ChartViewport::getPriceStep() const
    {
        return trtutils::niceAxisStep(priceMin, priceMax, priceRect.height());
    }

    std::vector<double> ChartViewport::getPriceTicks() const
    {
        const double step = getPriceStep();
        const double first = std::ceil(priceMin / step);
        const int count = std::max(0, static_cast<int>(std::floor(priceMax / step) - first) + 1);
        std::vector<double> result;
        for (int index = 0; index < count; index++)
            result.push_back((first + index) * step);
        return result;
    }

    double ChartViewport::getVolumeMax() const
    {
        return volumeMax;
    }

    double ChartViewport::xFromIndex(double index) const
    {
        return priceRect.left() + (index - firstBar + 0.5) * barWidth;
    }

    double ChartViewport::indexFromX(double x) const
    {
        return firstBar + (x - priceRect.left()) / barWidth - 0.5;
    }

    double ChartViewport::yFromPrice(double price) const
    {
        return priceRect.bottom() - (price - priceMin) / (priceMax - priceMin) * priceRect.height();
    }

    double ChartViewport::priceFromY(double y) const
    {
        return priceMin + (priceRect.bottom() - y) / priceRect.height() * (priceMax - priceMin);
    }

    double ChartViewport::yFromVolume(double volume) const
    {
        return volumeRect.bottom() - volume / volumeMax * std::max(1.0, volumeRect.height());
    }

    std::size_t ChartViewport::getFirstVisible() const
    {
        return static_cast<std::size_t>(std::max(0.0, std::floor(firstBar)));
    }

    std::size_t ChartViewport::getLastVisible() const
    {
        return std::min(
            seriesSize, static_cast<std::size_t>(std::max(0.0, std::ceil(firstBar + priceRect.width() / barWidth))));
    }
}
