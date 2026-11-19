#include "HeaderTable.h"
#include <QProxyStyle>
#include <QStyleOptionViewItem>
#include <QPainter>
#include <QHeaderView>
#include <algorithm>
namespace trtui
{
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

    HeaderTable::HeaderTable(QWidget* parent, const QStringList& headers) : QTableWidget(parent)
    {
        TableStyle* tableStyle = new TableStyle;
        tableStyle->setParent(this);
        setStyle(tableStyle);
        setFrameShape(QFrame::NoFrame);
        setColumnCount(headers.size());
        setHorizontalHeaderLabels(headers);
        horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }
    void HeaderTable::paintEvent(QPaintEvent* event)
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
}
