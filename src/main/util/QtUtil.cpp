//
// Created by NHCM on 2026/11/8.
//

#include "QtUtil.h"

#include <QHeaderView>
#include <QProxyStyle>
#include <QStyleOption>

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

    QTableWidget* createHeaderTable(QWidget* parent, QStringList headers)
    {
        QTableWidget* table = new QTableWidget(parent);
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
