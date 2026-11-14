#include "ChartViewport.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace trtui
{
    void ChartViewport::setSize(const QSize& size)
    {
        const double width = std::max(1, size.width() - 104);
        const double height = std::max(1, size.height() - 62);
        const double volumeHeight = volumeVisible ? std::max(70.0, height * 0.25) : 0;
        priceRect = QRectF(10, 32, width, std::max(1.0, height - volumeHeight - (volumeVisible ? 12 : 0)));
        volumeRect = QRectF(10, priceRect.bottom() + 12, width, volumeHeight);
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
        setSize(size);
    }

    void ChartViewport::constrain()
    {
        const double capacity = priceRect.width() / barWidth;
        const double latest = std::max(-0.5, static_cast<double>(seriesSize) - capacity + 5);
        if (following)
            firstBar = latest;
        else
            firstBar = std::clamp(firstBar, -std::min(40.0, capacity * 0.2) - 0.5, latest);
    }

    void ChartViewport::updateRange(std::span<const trttrade::CandleData> candles)
    {
        priceMin = std::numeric_limits<double>::max();
        priceMax = std::numeric_limits<double>::lowest();
        volumeMax = 0;
        const std::size_t end = std::min(candles.size(), getLastVisible());
        for (std::size_t index = getFirstVisible(); index < end; index++)
        {
            priceMin = std::min(priceMin, candles[index].low);
            priceMax = std::max(priceMax, candles[index].high);
            volumeMax = std::max(volumeMax, candles[index].volume);
        }
        if (priceMin > priceMax)
        {
            priceMin = 0;
            priceMax = 1;
        }
        const double padding = std::max((priceMax - priceMin) * 0.08, std::max(std::abs(priceMax) * 0.0001, 1e-12));
        priceMin -= padding;
        priceMax += padding;
        volumeMax = std::max(volumeMax * 1.15, 1e-12);
    }

    void ChartViewport::pan(double pixels)
    {
        following = false;
        firstBar -= pixels / barWidth;
        constrain();
    }

    void ChartViewport::zoom(double factor, double anchorX)
    {
        const double anchor = indexFromX(anchorX);
        following = false;
        barWidth = std::clamp(barWidth * factor, 2.0, 48.0);
        firstBar = anchor - (anchorX - priceRect.left()) / barWidth + 0.5;
        constrain();
    }

    void ChartViewport::reset()
    {
        barWidth = 8;
        followLatest();
    }

    void ChartViewport::followLatest()
    {
        following = true;
        constrain();
    }

    bool ChartViewport::isFollowing() const { return following; }
    bool ChartViewport::isVolumeVisible() const { return volumeVisible; }
    const QRectF& ChartViewport::getPriceRect() const { return priceRect; }
    const QRectF& ChartViewport::getVolumeRect() const { return volumeRect; }
    double ChartViewport::getBarWidth() const { return barWidth; }
    double ChartViewport::getPriceMin() const { return priceMin; }
    double ChartViewport::getPriceMax() const { return priceMax; }
    double ChartViewport::getPriceStep() const
    {
        const double raw = (priceMax - priceMin) / 5;
        const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
        const double fraction = raw / magnitude;
        for (double step : {1.0, 2.0, 2.5, 5.0, 10.0})
        {
            if (fraction <= step)
                return step * magnitude;
        }
        return 10 * magnitude;
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

    double ChartViewport::getVolumeMax() const { return volumeMax; }
    double ChartViewport::xFromIndex(double index) const { return priceRect.left() + (index - firstBar + 0.5) * barWidth; }
    double ChartViewport::indexFromX(double x) const { return firstBar + (x - priceRect.left()) / barWidth - 0.5; }
    double ChartViewport::yFromPrice(double price) const { return priceRect.bottom() - (price - priceMin) / (priceMax - priceMin) * priceRect.height(); }
    double ChartViewport::priceFromY(double y) const { return priceMin + (priceRect.bottom() - y) / priceRect.height() * (priceMax - priceMin); }
    double ChartViewport::yFromVolume(double volume) const { return volumeRect.bottom() - volume / volumeMax * std::max(1.0, volumeRect.height() - 40); }
    std::size_t ChartViewport::getFirstVisible() const { return static_cast<std::size_t>(std::max(0.0, std::floor(firstBar))); }
    std::size_t ChartViewport::getLastVisible() const
    {
        return std::min(seriesSize, static_cast<std::size_t>(std::max(0.0, std::ceil(firstBar + priceRect.width() / barWidth))));
    }
}
