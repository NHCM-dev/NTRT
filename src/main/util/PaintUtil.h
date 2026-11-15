#ifndef NTRT_PAINTUTIL_H
#define NTRT_PAINTUTIL_H

#include <QIcon>
#include <QPainter>
#include <QRegion>

class QWidget;

namespace trtutils
{
    void clipToParentRect(QPainter& painter, const QWidget* widget);

    QRegion frameRegion(const QRect& rect, int thickness);

    void drawBadge(QPainter& painter, const QRectF& area, const QString& text, const QColor& background);

    QIcon createLetterIcon(const QString& symbol);
}


#include <QColor>
#include <QRectF>
#include <vector>

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
}

namespace trtutils
{
    void appendClippedRect(std::vector<trtui::ChartVertex>& vertices, const QRectF& rect, const QColor& color,
                           const QRectF& clip);
}


#endif //NTRT_PAINTUTIL_H
