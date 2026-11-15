#include "ChartView.h"
#include "main/util/CandleUtil.h"
#include "main/util/NumberUtil.h"
#include "main/util/PaletteUtil.h"
#include "main/util/PaintUtil.h"
#include "main/util/TimeUtil.h"
#include <QDateTime>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QTimeZone>
#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace trtui
{
    ChartView::ChartView(QWidget* parent) : QOpenGLWidget(parent)
    {
        initSurface();
        initInteraction();
        layers.push_back(std::make_unique<VolumeLayer>());
        layers.push_back(std::make_unique<CandlestickLayer>());
    }

    void ChartView::initSurface()
    {
        QSurfaceFormat format;
        format.setRenderableType(QSurfaceFormat::OpenGL);
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setDepthBufferSize(0);
        format.setStencilBufferSize(0);
        format.setSamples(0);
        setFormat(format);
    }

    void ChartView::initInteraction()
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        setMinimumSize(240, 180);
        setAccessibleName("Candlestick chart");
    }

    ChartView::~ChartView()
    {
        cleanup();
    }

    void ChartView::initializeGL()
    {
        initializeOpenGLFunctions();
        rendererError = renderer.initialize();
        rendererReady = rendererError.isEmpty();
        geometryDirty = true;
        contextCleanup = connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, [this]
        {
            cleanup();
        }, Qt::DirectConnection);
        if (!rendererReady)
            emit renderingFailed(rendererError);
    }

    void ChartView::cleanup()
    {
        disconnect(contextCleanup);
        if (context() && context()->isValid())
        {
            makeCurrent();
            renderer.release();
            doneCurrent();
        }
        rendererReady = false;
    }

    void ChartView::resizeGL(int, int)
    {
        viewport.setSize(size());
        geometryDirty = true;
    }

    void ChartView::invalidateGeometry()
    {
        geometryDirty = true;
        if (!candles.empty() && viewport.getFirstVisible() < 10)
            emit historyRequested();
        update();
        emit followChanged(viewport.isFollowing());
    }

    void ChartView::setInstrument(const trtdata::InstIdentifier& instrument)
    {
        instrumentType = instrument.type;
        const QString symbol = QString::fromStdString(instrument.id);
        baseCurrency = symbol.section('-', 0, 0);
        quoteCurrency = symbol.section('-', 1, 1);
    }

    void ChartView::setCandles(std::span<const trttrade::CandleData> data)
    {
        std::size_t removedPrefix = 0;
        if (!candles.empty() && !data.empty())
        {
            const std::span<const trttrade::CandleData> previous(candles);
            removedPrefix = static_cast<std::size_t>(trtutils::candleLowerBound(previous, data.front().timestamp) -
                previous.begin());
        }
        if (!candles.empty() && !data.empty() && data.front().timestamp < candles.front().timestamp && !viewport.
            isFollowing())
        {
            const std::span<const trttrade::CandleData>::iterator previousFirst = trtutils::candleLowerBound(
                data, candles.front().timestamp);
            viewport.setSeriesSize(data.size());
            viewport.pan(-static_cast<double>(previousFirst - data.begin()) * viewport.getBarWidth());
        }
        candles.assign(data.begin(), data.end());
        viewport.setSeriesSize(candles.size(), removedPrefix);
        invalidateGeometry();
    }

    void ChartView::setTimeZone(trtdata::TradingDataTimeZone value)
    {
        if (timeZone == value)
            return;
        timeZone = value;
        update();
    }

    void ChartView::setHistoryState(bool loading, bool exhausted)
    {
        if (historyLoading == loading && historyExhausted == exhausted)
            return;
        historyLoading = loading;
        historyExhausted = exhausted;
        update();
    }

    void ChartView::setMessage(const QString& text)
    {
        if (message == text)
            return;
        message = text;
        update();
    }

    void ChartView::setVolumeVisible(bool visible)
    {
        viewport.setVolumeVisible(visible, size());
        invalidateGeometry();
    }

    void ChartView::resetView()
    {
        viewport.reset();
        invalidateGeometry();
    }

    void ChartView::followLatest()
    {
        viewport.followLatest();
        invalidateGeometry();
    }

    void ChartView::addLayer(std::unique_ptr<ChartLayer> layer)
    {
        layers.push_back(std::move(layer));
        invalidateGeometry();
    }

    QColor ChartView::getRisingColor() const
    {
        return trtutils::risingColor(palette());
    }

    QColor ChartView::getFallingColor() const
    {
        return trtutils::fallingColor(palette());
    }

    void ChartView::buildGeometry()
    {
        viewport.updateRange(candles);
        vertices.clear();
        const ChartLayerContext context{
            viewport,
            candles,
            getRisingColor(),
            getFallingColor(),
            1.0 / devicePixelRatioF()
        };
        for (const std::unique_ptr<ChartLayer>& layer : layers)
            layer->build(context, vertices);
    }

    void ChartView::paintGL()
    {
        const QColor background = palette().color(QPalette::Base);
        glClearColor(background.redF(), background.greenF(), background.blueF(), 1);
        glClear(GL_COLOR_BUFFER_BIT);
        if (geometryDirty)
            buildGeometry();
        QPainter painter(this);
        painter.setFont(font());
        drawGrid(painter);
        if (rendererReady && !candles.empty())
        {
            painter.beginNativePainting();
            renderer.draw(vertices, size(), geometryDirty);
            painter.endNativePainting();
        }
        geometryDirty = false;
        drawAxes(painter);
        drawReadout(painter);
        drawCrosshair(painter);
        if (!candles.empty() && viewport.indexFromX(viewport.getPriceRect().left()) < 0 && (historyLoading ||
            historyExhausted))
        {
            const QRectF area = viewport.getPriceRect();
            const double blankWidth = std::max(100.0, viewport.xFromIndex(0) - area.left());
            painter.setPen(palette().color(QPalette::PlaceholderText));
            painter.drawText(QRectF(area.left(), area.center().y() - 15, std::min(blankWidth, area.width()), 30),
                             Qt::AlignCenter,
                             historyExhausted ? "No earlier data" : "Loading…");
        }
        if (candles.empty() || !rendererReady)
        {
            painter.setPen(palette().color(QPalette::PlaceholderText));
            const QString text = rendererReady ? message : "OpenGL chart unavailable\n" + rendererError;
            painter.drawText(viewport.getPriceRect().adjusted(16, 16, -16, -16), Qt::AlignCenter | Qt::TextWordWrap,
                             text);
        }
    }

    void ChartView::drawGrid(QPainter& painter)
    {
        const QRectF& area = viewport.getPriceRect();
        QColor color = palette().color(QPalette::Mid);
        color.setAlphaF(0.18);
        painter.setPen(QPen(color, 1, Qt::DotLine));
        for (double tick : viewport.getPriceTicks())
        {
            const double y = viewport.yFromPrice(tick);
            painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        }
        const int columns = std::max(1, static_cast<int>(area.width() / 120));
        for (int column = 0; column <= columns; column++)
        {
            const double x = area.left() + area.width() * column / columns;
            painter.drawLine(QPointF(x, area.top()),
                             QPointF(x, viewport.isVolumeVisible()
                                            ? viewport.getVolumeRect().bottom()
                                            : area.bottom()));
        }
        painter.setPen(palette().color(QPalette::Mid));
        painter.drawLine(area.topRight(), QPointF(area.right(), height() - 26));
        if (viewport.isVolumeVisible())
            painter.drawLine(viewport.getVolumeRect().topLeft(), viewport.getVolumeRect().topRight());
    }

    QString ChartView::formatCrosshairPrice(double value) const
    {
        const double perPixel = (viewport.getPriceMax() - viewport.getPriceMin()) / viewport.getPriceRect().height();
        return trtutils::formatAxisPrice(value, perPixel);
    }

    QString ChartView::formatTime(qint64 timestamp, const QString& format) const
    {
        return trtutils::formatTradingTime(timestamp, timeZone, format);
    }

    void ChartView::drawAxes(QPainter& painter)
    {
        const QRectF& area = viewport.getPriceRect();
        painter.setPen(palette().color(QPalette::PlaceholderText));
        if (candles.empty())
            return;
        for (double tick : viewport.getPriceTicks())
        {
            const double y = viewport.yFromPrice(tick);
            painter.drawText(QRectF(area.right() + 6, y - 10, 86, 20), Qt::AlignLeft | Qt::AlignVCenter,
                             trtutils::formatPrice(tick));
        }
        const int columns = std::max(1, static_cast<int>(area.width() / 120));
        for (int column = 0; column <= columns; column++)
        {
            const double x = area.left() + area.width() * column / columns;
            const qint64 index = std::llround(viewport.indexFromX(x));
            if (index < 0 || index >= static_cast<qint64>(candles.size()))
                continue;
            const trttrade::CandleData& candle = candles[static_cast<std::size_t>(index)];
            painter.drawText(QRectF(std::clamp(x - 54, 0.0, std::max(0.0, area.right() - 108)), height() - 24, 108, 20),
                             Qt::AlignCenter, formatTime(candle.timestamp, "MM-dd HH:mm"));
        }
        if (viewport.isVolumeVisible())
        {

            painter.drawText(QRectF(area.right() + 6, viewport.getVolumeRect().top(), 86, 20), Qt::AlignLeft,
                             trtutils::formatVolume(viewport.getVolumeMax()));
        }
        const trttrade::CandleData& latest = candles.back();
        const double y = viewport.yFromPrice(latest.close);
        if (y >= area.top() && y <= area.bottom())
        {
            const QColor color = latest.close >= latest.open ? getRisingColor() : getFallingColor();
            QColor lineColor = color;
            lineColor.setAlphaF(0.4);
            painter.setPen(QPen(lineColor, 1, Qt::DashLine));
            painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
            trtutils::drawBadge(painter, QRectF(area.right() + 1, y - 10, 91, 20), trtutils::formatPrice(latest.close),
                                color);
        }
    }

    qint64 ChartView::getCursorIndex() const
    {
        return std::llround(viewport.indexFromX(cursorPosition.x()));
    }

    void ChartView::drawReadout(QPainter& painter)
    {
        if (candles.empty())
            return;
        const qint64 index = getCursorIndex();
        const bool hasCandle = cursorVisible && index >= 0 && index < static_cast<qint64>(candles.size());
        const trttrade::CandleData& candle = hasCandle ? candles[static_cast<std::size_t>(index)] : candles.back();
        const QString text = QString("O %1   H %2   L %3   C %4  %5").arg(
            trtutils::formatPrice(candle.open), trtutils::formatPrice(candle.high),
            trtutils::formatPrice(candle.low), trtutils::formatPrice(candle.close),
            candle.confirmed ? "" : "· Forming");
        painter.setPen(candle.close >= candle.open ? getRisingColor() : getFallingColor());
        const QRectF readout = viewport.getPriceRect().adjusted(5, 3, -5, 0);
        painter.fillRect(QRectF(readout.topLeft(), QSizeF(readout.width(), 23)), palette().brush(QPalette::Base));
        painter.drawText(QRectF(readout.topLeft(), QSizeF(readout.width(), 23)), Qt::AlignVCenter,
                         painter.fontMetrics().elidedText(text, Qt::ElideRight, static_cast<int>(readout.width())));
        if (viewport.isVolumeVisible())
        {
            const double base = trtutils::baseVolume(candle, instrumentType);
            QString contracts = trtutils::formatVolume(candle.volume);
            switch (instrumentType)
            {
                case trttrade::InstType::Spot:
                case trttrade::InstType::Margin:
                    contracts = "—";
                    break;
                case trttrade::InstType::Swap:
                case trttrade::InstType::Futures:
                case trttrade::InstType::Option:
                case trttrade::InstType::Events:
                    break;
            }
            const QString volume = QString("Volume %1 %2  %3 %4  %5 Contracts").arg(
                trtutils::formatVolume(base), baseCurrency, trtutils::formatVolume(candle.volumeQuote), quoteCurrency,
                contracts);
            const QRectF volumeArea = viewport.getVolumeRect().adjusted(5, 2, -5, 0);
            painter.fillRect(QRectF(volumeArea.topLeft(), QSizeF(volumeArea.width(), 38)),
                             palette().brush(QPalette::Base));
            painter.drawText(volumeArea, Qt::AlignTop | Qt::AlignLeft, volume);
        }
    }

    void ChartView::drawCrosshair(QPainter& painter)
    {
        if (!cursorVisible || candles.empty())
            return;
        const QRectF& area = viewport.getPriceRect();
        const qint64 index = getCursorIndex();
        const bool hasCandle = index >= 0 && index < static_cast<qint64>(candles.size());
        const double x = hasCandle ? viewport.xFromIndex(static_cast<double>(index)) : cursorPosition.x();
        const double bottom = viewport.isVolumeVisible() ? viewport.getVolumeRect().bottom() : area.bottom();
        QColor crosshairColor = palette().color(QPalette::Text);
        crosshairColor.setAlphaF(0.35);
        painter.setPen(QPen(crosshairColor, 1, Qt::DashLine));
        painter.drawLine(QPointF(x, area.top()), QPointF(x, bottom));
        const QColor badge = palette().color(QPalette::Highlight);
        if (area.contains(cursorPosition))
        {
            painter.drawLine(QPointF(area.left(), cursorPosition.y()), QPointF(area.right(), cursorPosition.y()));
            trtutils::drawBadge(painter, QRectF(area.right() + 1, cursorPosition.y() - 10, 91, 20),
                                formatCrosshairPrice(viewport.priceFromY(cursorPosition.y())), badge);
        }
        if (!hasCandle)
            return;
        const QString text = formatTime(candles[static_cast<std::size_t>(index)].timestamp, "yyyy-MM-dd HH:mm");
        const double badgeWidth = painter.fontMetrics().horizontalAdvance(text) + 12;
        trtutils::drawBadge(
            painter, QRectF(std::clamp(x - badgeWidth / 2, 0.0, std::max(0.0, area.right() - badgeWidth)),
                            height() - 25, badgeWidth, 23), text, badge);
    }

    void ChartView::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            dragging = true;
            dragPosition = event->position();
            setCursor(Qt::ClosedHandCursor);
            setFocus(Qt::MouseFocusReason);
            event->accept();
            return;
        }
        QOpenGLWidget::mousePressEvent(event);
    }

    void ChartView::mouseMoveEvent(QMouseEvent* event)
    {
        cursorPosition = event->position();
        cursorVisible = viewport.getPriceRect().contains(cursorPosition) ||
            (viewport.isVolumeVisible() && viewport.getVolumeRect().contains(cursorPosition));
        if (dragging)
        {
            viewport.pan(cursorPosition.x() - dragPosition.x());
            dragPosition = cursorPosition;
            invalidateGeometry();
        } else
            update();
    }

    void ChartView::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            dragging = false;
            unsetCursor();
            event->accept();
            return;
        }
        QOpenGLWidget::mouseReleaseEvent(event);
    }

    void ChartView::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            dragging = false;
            unsetCursor();
            resetView();
            event->accept();
            return;
        }
        QOpenGLWidget::mouseDoubleClickEvent(event);
    }

    void ChartView::wheelEvent(QWheelEvent* event)
    {
        viewport.zoom(std::pow(1.15, event->angleDelta().y() / 120.0), event->position().x());
        invalidateGeometry();
        event->accept();
    }

    void ChartView::keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
            case Qt::Key_Left:
                viewport.pan(viewport.getBarWidth() * 10);
                break;
            case Qt::Key_Right:
                viewport.pan(-viewport.getBarWidth() * 10);
                break;
            case Qt::Key_Plus:
            case Qt::Key_Equal:
                viewport.zoom(1.25, viewport.getPriceRect().center().x());
                break;
            case Qt::Key_Minus:
                viewport.zoom(0.8, viewport.getPriceRect().center().x());
                break;
            case Qt::Key_End:
                viewport.followLatest();
                break;
            case Qt::Key_Home:
                viewport.reset();
                break;
            default:
                QOpenGLWidget::keyPressEvent(event);
                return;
        }
        invalidateGeometry();
        event->accept();
    }

    void ChartView::leaveEvent(QEvent* event)
    {
        cursorVisible = false;
        update();
        QOpenGLWidget::leaveEvent(event);
    }

    void ChartView::changeEvent(QEvent* event)
    {
        QOpenGLWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::FontChange)
            invalidateGeometry();
    }

    void ChartView::showEvent(QShowEvent* event)
    {
        QOpenGLWidget::showEvent(event);
        QTimer::singleShot(0, this, [this]
        {
            if (!isValid())
                emit renderingFailed("An OpenGL 3.3 context could not be created. Check the graphics driver.");
        });
    }
}
