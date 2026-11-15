#ifndef NTRT_CHARTVIEWPORT_H
#define NTRT_CHARTVIEWPORT_H

#include "main/trade/TradeDataDef.h"
#include <QRectF>
#include <QSize>
#include <span>
#include <vector>

namespace trtui
{
    // Owns the shared time transform and pane transforms, independently of the renderer.
    class ChartViewport final
    {
        QRectF readoutRect;
        QRectF volumeReadoutRect;
        int readoutHeight = 23;
        QRectF priceRect;
        QRectF volumeRect;
        std::size_t seriesSize = 0;
        double firstBar = 0;
        double barWidth = 8;
        double priceMin = 0;
        double priceMax = 1;
        double volumeMax = 1;
        bool following = true;
        bool volumeVisible = true;

        void constrain();

    public:
        void setSize(const QSize& size, int readoutHeight = 23);

        void setSeriesSize(std::size_t count, std::size_t removedPrefix = 0);

        void setVolumeVisible(bool visible, const QSize& size);

        void updateRange(std::span<const trttrade::CandleData> candles);

        void pan(double pixels);

        void zoom(double factor, double anchorX);

        void reset();

        void followLatest();

        bool isFollowing() const;

        bool isVolumeVisible() const;

        const QRectF& getReadoutRect() const;
        const QRectF& getVolumeReadoutRect() const;
        const QRectF& getPriceRect() const;

        const QRectF& getVolumeRect() const;

        double getBarWidth() const;

        double getPriceMin() const;

        double getPriceMax() const;

        double getPriceStep() const;

        std::vector<double> getPriceTicks() const;

        double getVolumeMax() const;

        double xFromIndex(double index) const;

        double indexFromX(double x) const;

        double yFromPrice(double price) const;

        double priceFromY(double y) const;

        double yFromVolume(double volume) const;

        std::size_t getFirstVisible() const;

        std::size_t getLastVisible() const;
    };
}
#endif
