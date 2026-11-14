#include "ChartRenderer.h"
#include "main/util/FileUtil.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QVector2D>
#include <cstddef>

namespace trtui
{
    QString ChartRenderer::initialize()
    {
        program = std::make_unique<QOpenGLShaderProgram>();
        if (!program->addShaderFromSourceFile(QOpenGLShader::Vertex, trtutils::relativePath("res/shaders/chart.vert")) ||
            !program->addShaderFromSourceFile(QOpenGLShader::Fragment, trtutils::relativePath("res/shaders/chart.frag")) || !program->link())
        {
            const QString error = program->log();
            return error.isEmpty() ? "Could not load or compile the chart shaders in res/shaders." : error;
        }
        if (!buffer.create() || !vertexArray.create())
            return "Could not allocate OpenGL chart buffers.";
        buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
        return {};
    }

    void ChartRenderer::release()
    {
        vertexArray.destroy();
        buffer.destroy();
        program.reset();
        vertexCount = 0;
    }

    void ChartRenderer::draw(const std::vector<ChartVertex>& vertices, const QSize& size, bool upload)
    {
        QOpenGLFunctions* functions = QOpenGLContext::currentContext()->functions();
        functions->glDisable(GL_DEPTH_TEST);
        functions->glDisable(GL_CULL_FACE);
        functions->glEnable(GL_BLEND);
        functions->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        program->bind();
        program->setUniformValue("viewportSize", QVector2D(static_cast<float>(size.width()), static_cast<float>(size.height())));
        vertexArray.bind();
        buffer.bind();
        if (upload)
        {
            buffer.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(ChartVertex)));
            vertexCount = static_cast<int>(vertices.size());
        }
        program->enableAttributeArray(0);
        program->enableAttributeArray(1);
        program->setAttributeBuffer(0, GL_FLOAT, offsetof(ChartVertex, x), 2, sizeof(ChartVertex));
        program->setAttributeBuffer(1, GL_FLOAT, offsetof(ChartVertex, red), 4, sizeof(ChartVertex));
        functions->glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        buffer.release();
        vertexArray.release();
        program->release();
    }
}
