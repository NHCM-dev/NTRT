#ifndef NTRT_CHARTVIEW_H
#define NTRT_CHARTVIEW_H

#include "ChartRenderer.h"
#include "main/market/abstract/Instrument.h"
#include "main/data/TradingDataConfig.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class QPainter;

namespace trtui
{
    class ChartView final : public QOpenGLWidget, protected QOpenGLFunctions
    {
        Q_OBJECT

        ChartViewport viewport;
        ChartRenderer renderer;
        std::vector<std::unique_ptr<ChartLayer>> layers;
        std::vector<trttrade::CandleData> candles;
        std::vector<ChartVertex> vertices;
        trtdata::TradingDataTimeZone timeZone = trtdata::TradingDataTimeZone::UTC;
        QMetaObject::Connection contextCleanup;
        QPointF cursorPosition;
        QPointF dragPosition;
        bool cursorVisible = false;
        bool dragging = false;
        bool geometryDirty = true;
        bool rendererReady = false;
        trttrade::InstType instrumentType = trttrade::InstType::Spot;
        QString baseCurrency;
        QString quoteCurrency;
        QString message = "Choose an instrument to begin";
        QString rendererError;
        bool historyLoading = false;
        bool historyExhausted = false;

        void initSurface();

        void initInteraction();

        void cleanup();

        void invalidateGeometry();

        void buildGeometry();

        void drawGrid(QPainter& painter);

        void drawAxes(QPainter& painter);

        void drawCrosshair(QPainter& painter);

        void drawReadout(QPainter& painter);

        QString formatCrosshairPrice(double value) const;

        QString formatTime(qint64 timestamp, const QString& format) const;

        qint64 getCursorIndex() const;

        QColor getRisingColor() const;

        QColor getFallingColor() const;

    protected:
        void initializeGL() override;

        void resizeGL(int width, int height) override;

        void paintGL() override;

        void mousePressEvent(QMouseEvent* event) override;

        void mouseMoveEvent(QMouseEvent* event) override;

        void mouseReleaseEvent(QMouseEvent* event) override;

        void mouseDoubleClickEvent(QMouseEvent* event) override;

        void wheelEvent(QWheelEvent* event) override;

        void keyPressEvent(QKeyEvent* event) override;

        void leaveEvent(QEvent* event) override;

        void changeEvent(QEvent* event) override;

        void showEvent(QShowEvent* event) override;

    public:
        explicit ChartView(QWidget* parent = nullptr);

        ~ChartView() override;

        void setInstrument(const trtdata::InstIdentifier& instrument);

        void setCandles(std::span<const trttrade::CandleData> data);

        void setTimeZone(trtdata::TradingDataTimeZone value);

        void setHistoryState(bool loading, bool exhausted);

        void setMessage(const QString& text);

        void setVolumeVisible(bool visible);

        void resetView();

        void followLatest();

        void addLayer(std::unique_ptr<ChartLayer> layer);

    signals:
        void historyRequested();

        void followChanged(bool following);

        void renderingFailed(const QString& error);
    };
}
#endif
