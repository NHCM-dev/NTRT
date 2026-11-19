//
// Created by NHCM on 2026/11/8.
//

#include "QtUtil.h"
#include "main/ui/dialog/ListManagerDialog.h"

#include <QHeaderView>
#include <QScreen>
#include <QProxyStyle>
#include <QStyleOption>
#include <QPainter>
#include <QPointingDevice>
#include <algorithm>
#include <cmath>

namespace trtutils
{
    std::optional<QStringList> selectList(QWidget* parent, const QString& title, const QStringList& names, const QString& actionText, bool multiple)
    {
        std::optional<QStringList> selected;
        trtui::ListManagerDialog dialog(title, [&names]
        {
            trtui::ListManagerDialog::Items items;
            for (int index = 0; index < names.size(); index++) items.append({QString::number(index), names[index]});
            return items;
        }, parent, multiple);
        dialog.addAction(actionText, [&dialog, &selected, &names](const QStringList& ids)
        {
            QStringList values;
            for (const QString& id : ids) values.append(names[id.toInt()]);
            selected = values;
            dialog.accept();
        }, 1, multiple ? -1 : 1);
        if (dialog.exec() != QDialog::Accepted) return std::nullopt;
        return selected;
    }

    std::optional<QString> selectListSingle(QWidget* parent, const QString& title, const QStringList& names, const QString& actionText)
    {
        const std::optional<QStringList> selected = selectList(parent, title, names, actionText, false);
        if (!selected || selected->isEmpty()) return std::nullopt;
        return selected->first();
    }

    std::optional<QStringList> selectListMulti(QWidget* parent, const QString& title, const QStringList& names, const QString& actionText)
    {
        return selectList(parent, title, names, actionText, true);
    }

    double wheelZoomFactor(const QWheelEvent& event)
    {
        const bool pixels = !event.pixelDelta().isNull();
        const QPoint delta = pixels ? event.pixelDelta() : event.angleDelta();
        return std::pow(1.15, delta.y() / (pixels ? 40.0 : 120.0));
    }

    bool isWheelPan(const QWheelEvent& event)
    {
        // Windows precision-touchpad pinch is delivered as Ctrl+wheel. Native
        // pinch is handled separately; ordinary two-finger scrolling must not zoom.
        return !event.modifiers().testFlag(Qt::ControlModifier) &&
            (!event.pixelDelta().isNull() || event.phase() != Qt::NoScrollPhase || event.angleDelta().x() != 0 ||
                event.modifiers().testFlag(Qt::ShiftModifier) || event.pointingDevice()->type() == QInputDevice::DeviceType::TouchPad);
    }

    double wheelPanPixels(const QWheelEvent& event)
    {
        if (!event.pixelDelta().isNull())
            return event.pixelDelta().x();
        const QPoint delta = event.angleDelta();
        const int horizontal = delta.x() == 0 && event.modifiers().testFlag(Qt::ShiftModifier) ? delta.y() : delta.x();
        return horizontal / 120.0 * 40.0;
    }

    QRect boundedWidgetGeometry(const QRect& geometry, const QRect& bounds)
    {
        if (bounds.isEmpty())
            return geometry;
        const QSize size = geometry.size().boundedTo(bounds.size());
        const int x = std::clamp(geometry.x(), bounds.left(), bounds.right() - size.width() + 1);
        const int y = std::clamp(geometry.y(), bounds.top(), bounds.bottom() - size.height() + 1);
        return QRect(QPoint(x, y), size);
    }

    QRect resizedWidgetGeometry(const QRect& geometry, const QPoint& delta, Qt::Edges edges,
                                const QRect& bounds, const QSize& minimum)
    {
        QRect resized = geometry;
        if (edges.testFlag(Qt::LeftEdge))
            resized.setLeft(std::clamp(geometry.left() + delta.x(), bounds.left(),
                std::max(bounds.left(), geometry.right() - minimum.width() + 1)));
        if (edges.testFlag(Qt::RightEdge))
            resized.setRight(std::clamp(geometry.right() + delta.x(),
                std::min(bounds.right(), geometry.left() + minimum.width() - 1), bounds.right()));
        if (edges.testFlag(Qt::TopEdge))
            resized.setTop(std::clamp(geometry.top() + delta.y(), bounds.top(),
                std::max(bounds.top(), geometry.bottom() - minimum.height() + 1)));
        if (edges.testFlag(Qt::BottomEdge))
            resized.setBottom(std::clamp(geometry.bottom() + delta.y(),
                std::min(bounds.bottom(), geometry.top() + minimum.height() - 1), bounds.bottom()));
        return resized;
    }

    void updateWidgetTreeRegion(QWidget* widget, const QRegion& region)
    {
        if (!widget->isVisible() || !widget->updatesEnabled())
            return;
        const QRegion damage = region.intersected(widget->rect());
        if (damage.isEmpty())
            return;
        widget->update(damage);
        // An ancestor update alone skips opaque descendants in Qt's backing store.
        const QObjectList& children = widget->children();
        for (QObject* object : children)
        {
            QWidget* child = qobject_cast<QWidget*>(object);
            if (child && !child->isWindow())
                updateWidgetTreeRegion(child, damage.translated(-child->pos()));
        }
    }

    class TableStyle final : public QProxyStyle
    {
    public:
        TableStyle() : QProxyStyle("Fusion")
        {}

        void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                           const QWidget* widget) const override
        {
            // Use the standard checkbox frame instead of the flat item-view indicator.
            if (element == PE_IndicatorItemViewItemCheck)
                element = PE_IndicatorCheckBox;
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        }

        int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget,
                      QStyleHintReturn* data) const override
        {
            if (hint == SH_Table_GridLineColor && option && option->palette.base().color().lightness() < 128)
                return static_cast<int>(option->palette.midlight().color().rgb());
            return QProxyStyle::styleHint(hint, option, widget, data);
        }
    };

    class HeaderTable final : public QTableWidget
    {
    public:
        explicit HeaderTable(QWidget* parent) : QTableWidget(parent)
        {}

    protected:
        void paintEvent(QPaintEvent* event) override
        {
            QTableWidget::paintEvent(event);
            if (!showGrid() || rowCount() == 0 || columnCount() == 0)
                return;

            QStyleOptionViewItem option;
            option.initFrom(this);
            const int gridColor = style()->styleHint(QStyle::SH_Table_GridLineColor, &option, this);
            QPainter painter(viewport());
            painter.setPen(QColor::fromRgb(static_cast<QRgb>(gridColor)));
            const int bottom = std::min(viewport()->height() - 1,
                                        rowViewportPosition(rowCount() - 1) + rowHeight(rowCount() - 1) - 1);
            painter.drawLine(0, 0, 0, bottom);
        }
    };

    QTableWidgetItem* updateTableItem(QTableWidget* table, int row, int column, const QString& text)
    {
        QTableWidgetItem* item = table->item(row, column);
        if (!item)
        {
            item = new QTableWidgetItem;
            table->setItem(row, column, item);
        }
        if (item->text() != text)
            item->setText(text);
        return item;
    }

    void swapTableRows(QTableWidget* table, int first, int second)
    {
        for (int column = 0; column < table->columnCount(); column++)
        {
            QTableWidgetItem* source = table->takeItem(first, column);
            QTableWidgetItem* destination = table->takeItem(second, column);
            table->setItem(first, column, destination);
            table->setItem(second, column, source);
        }
    }

    void centerOnScreen(QWidget* widget)
    {
        fitWindowToScreen(widget);
        QRect geometry = widget->frameGeometry();
        geometry.moveCenter(widget->screen()->availableGeometry().center());
        widget->move(geometry.topLeft());
    }

    void fitWindowToScreen(QWidget* widget)
    {
        const QRect available = widget->screen()->availableGeometry();
        const QSize frame = widget->frameGeometry().size() - widget->size();
        widget->resize(widget->size().boundedTo((available.size() - frame).expandedTo(QSize(1, 1))));
        widget->move(boundedWidgetGeometry(widget->frameGeometry(), available).topLeft());
    }

    QTableWidget* createHeaderTable(QWidget* parent, QStringList headers)
    {
        QTableWidget* table = new HeaderTable(parent);
        TableStyle* style = new TableStyle;
        style->setParent(table);
        table->setStyle(style);

        table->setFrameShape(QFrame::NoFrame);

        table->setColumnCount(headers.count());
        table->setHorizontalHeaderLabels(headers);

        // Auto adjust column size
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        return table;
    }
} // trtutils
