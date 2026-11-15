#ifndef NTRT_PAINTUTIL_H
#define NTRT_PAINTUTIL_H

#include <QIcon>
#include <QPainter>

namespace trtutils
{
    void drawBadge(QPainter& painter, const QRectF& area, const QString& text, const QColor& background);

    QIcon createLetterIcon(const QString& symbol);
}

#endif //NTRT_PAINTUTIL_H
