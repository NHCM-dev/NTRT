#ifndef NTRT_CHARTVIEW_H
#define NTRT_CHARTVIEW_H

#include <QTimeZone>
#include <optional>
#include "ChartRenderer.h"
#include "main/trade/TradeDataDef.h"
#include "main/data/core/TradingTimeDef.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QToolButton>

namespace trtui
{
    class ChartView final : public QOpenGLWidget, protected QOpenGLFunctions
    {
        Q_OBJECT

        enum class DragMode { None, Plot, Time, PriceScale };

        ChartViewport viewport;
        ChartRenderer renderer;
        QToolButton* autoScaleButton = nullptr;
        std::vector<std::unique_ptr<ChartLayer>> layers;
        std::vector<trttrade::CandleData> candles;
        std::optional<double> latestPrice;
        std::vector<ChartVertex> vertices;
        QTimeZone timeZone = QTimeZone::utc();
        QMetaObject::Connection contextCleanup;
        QPointF cursorPosition;
        QPointF dragPosition;
        bool cursorVisible = false;
        DragMode dragMode = DragMode::None;
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

        void initControls();

        void positionControls();

        void setAutoScale(bool enabled);

        bool isPriceAxis(const QPointF& position) const;

        void updateCursor(const QPointF& position);

        void zoomAt(double factor, const QPointF& position);

        void cleanup();

        void invalidateGeometry();

        void buildGeometry();

        void drawGrid(QPainter& painter);

        void drawAxes(QPainter& painter);

        void drawLatestPrice(QPainter& painter);

        void drawCrosshair(QPainter& painter);

        void drawReadout(QPainter& painter);

        QString formatCrosshairPrice(double value) const;

        QString formatTime(qint64 timestamp, const QString& format) const;

        qint64 getCursorIndex() const;

        QColor getRisingColor() const;

        QColor getFallingColor() const;

    protected:
        bool event(QEvent* event) override;

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

        void setLatestPrice(std::optional<double> value);

        void setTimeZone(const QTimeZone& value);

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
