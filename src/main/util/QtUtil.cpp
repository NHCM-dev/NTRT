//
// Created by NHCM on 2026/11/8.
//

#include "QtUtil.h"

#include <QHeaderView>
#include <QProxyStyle>
#include <QStyleOption>
#include <QPainter>
#include <algorithm>

namespace trtutils
{
    class TableStyle final : public QProxyStyle
    {
    public:
        TableStyle() : QProxyStyle("Fusion") {}

        int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* data) const override
        {
            if (hint == SH_Table_GridLineColor && option && option->palette.base().color().lightness() < 128)
                return static_cast<int>(option->palette.midlight().color().rgb());
            return QProxyStyle::styleHint(hint, option, widget, data);
        }
    };

    class HeaderTable final : public QTableWidget
    {
    public:
        explicit HeaderTable(QWidget* parent) : QTableWidget(parent) {}

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
            const int bottom = std::min(viewport()->height() - 1, rowViewportPosition(rowCount() - 1) + rowHeight(rowCount() - 1) - 1);
            painter.drawLine(0, 0, 0, bottom);
        }
    };

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
