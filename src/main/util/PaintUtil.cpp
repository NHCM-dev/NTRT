#include "PaintUtil.h"
#include <algorithm>

namespace trtutils
{
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
