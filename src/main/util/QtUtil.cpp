//
// Created by NHCM on 2026/11/8.
//

#include "QtUtil.h"

#include <QHeaderView>
#include <QScreen>
#include <QProxyStyle>
#include <QStyleOption>
#include <QPainter>
#include <algorithm>

namespace trtutils
{
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
        QRect geometry = widget->frameGeometry();
        geometry.moveCenter(widget->screen()->availableGeometry().center());
        widget->move(geometry.topLeft());
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
