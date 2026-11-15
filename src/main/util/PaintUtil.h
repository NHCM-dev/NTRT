#ifndef NTRT_PAINTUTIL_H
#define NTRT_PAINTUTIL_H

#include "main/ui/chart/ChartDef.h"
#include <QIcon>
#include <QColor>
#include <QRectF>
#include <vector>
#include <QPainter>
#include <QRegion>

class QWidget;

namespace trtutils
{
    void clipToParentRect(QPainter& painter, const QWidget* widget);

    QRegion frameRegion(const QRect& rect, int thickness);

    void drawBadge(QPainter& painter, const QRectF& area, const QString& text, const QColor& background);

    QIcon createLetterIcon(const QString& symbol);

    void drawHighlightedText(QPainter& painter, const QRect& area, const QString& value, const QFont& font,
                             const QColor& foreground, Qt::Alignment alignment, qsizetype start,
                             qsizetype length, const QColor& highlight);

    void drawValueBar(QPainter& painter, const QRectF& area, double fraction, const QColor& color);

    void appendClippedRect(std::vector<trtui::ChartVertex>& vertices, const QRectF& rect, const QColor& color,
                           const QRectF& clip);
}

#endif //NTRT_PAINTUTIL_H
