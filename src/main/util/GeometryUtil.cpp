#include "GeometryUtil.h"

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
