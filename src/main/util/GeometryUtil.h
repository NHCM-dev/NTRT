#ifndef NTRT_GEOMETRYUTIL_H
#define NTRT_GEOMETRYUTIL_H

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

#endif //NTRT_GEOMETRYUTIL_H
