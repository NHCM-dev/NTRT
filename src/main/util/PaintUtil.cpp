#include "PaintUtil.h"
#include <algorithm>
#include <QWidget>
#include <cmath>

namespace trtutils
{
    void clipToParentRect(QPainter& painter, const QWidget* widget)
    {
        if (!widget || !widget->parentWidget())
            return;
        const QRectF parentRect = QRectF(widget->parentWidget()->rect()).translated(-widget->pos());
        const QTransform transform = painter.deviceTransform();
        const QRectF pixels = transform.mapRect(parentRect);
        const QRectF inside(QPointF(std::ceil(pixels.left()), std::ceil(pixels.top())),
                            QPointF(std::floor(pixels.right()), std::floor(pixels.bottom())));
        // Round inward so a fractional viewport edge cannot paint the adjacent widget.
        painter.setClipRect(transform.inverted().mapRect(inside), Qt::IntersectClip);
    }

    QRegion frameRegion(const QRect& rect, int thickness)
    {
        return QRegion(rect).subtracted(QRegion(rect.adjusted(thickness, thickness, -thickness, -thickness)));
    }

    void drawBadge(QPainter& painter, const QRectF& area, const QString& text, const QColor& background)
    {
        painter.save();
        painter.setClipRect(area);
        painter.fillRect(area, background);
        painter.setPen(background.lightness() < 140 ? Qt::white : Qt::black);
        const double textWidth = painter.fontMetrics().horizontalAdvance(text);
        if (textWidth > area.width() - 6)
        {
            QFont labelFont = painter.font();
            labelFont.setPixelSize(
                std::max(8, static_cast<int>(painter.fontMetrics().height() * (area.width() - 6) / textWidth * 0.8)));
            painter.setFont(labelFont);
        }
        painter.drawText(area.adjusted(3, 0, -3, 0), Qt::AlignCenter, text);
        painter.restore();
    }

    QIcon createLetterIcon(const QString& symbol)
    {
        QPixmap image(40, 40);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0x53647c));
        painter.drawEllipse(0, 0, 40, 40);
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPixelSize(20);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(image.rect(), Qt::AlignCenter, symbol.left(1));
        return QIcon(image);
    }
}


namespace trtutils
{
    void appendClippedRect(std::vector<trtui::ChartVertex>& vertices, const QRectF& rect, const QColor& color,
                           const QRectF& clip)
    {
        const QRectF area = rect.intersected(clip);
        if (area.isEmpty())
            return;
        const float left = static_cast<float>(area.left());
        const float right = static_cast<float>(area.right());
        const float top = static_cast<float>(area.top());
        const float bottom = static_cast<float>(area.bottom());
        const float red = color.redF();
        const float green = color.greenF();
        const float blue = color.blueF();
        const float alpha = color.alphaF();
        vertices.insert(vertices.end(), {
                            {left, top, red, green, blue, alpha},
                            {right, top, red, green, blue, alpha},
                            {right, bottom, red, green, blue, alpha},
                            {left, top, red, green, blue, alpha},
                            {right, bottom, red, green, blue, alpha},
                            {left, bottom, red, green, blue, alpha}
                        });
    }
}
