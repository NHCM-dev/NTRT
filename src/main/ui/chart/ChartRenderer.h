#ifndef NTRT_CHARTRENDERER_H
#define NTRT_CHARTRENDERER_H

#include "ChartLayer.h"
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <memory>

namespace trtui
{
    class ChartRenderer final
    {
        std::unique_ptr<QOpenGLShaderProgram> program;
        QOpenGLBuffer buffer{QOpenGLBuffer::VertexBuffer};
        QOpenGLVertexArrayObject vertexArray;
        int vertexCount = 0;

    public:
        QString initialize();

        void release();

        void draw(const std::vector<ChartVertex>& vertices, const QSize& size, bool upload);
    };
}
#endif
