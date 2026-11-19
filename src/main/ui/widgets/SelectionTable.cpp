#include "SelectionTable.h"
#include <QHeaderView>
#include <QSignalBlocker>

namespace trtui
{
    SelectionTable::RowDelegate::RowDelegate(SelectionTable* table) : QStyledItemDelegate(table), table(table) {}

    void SelectionTable::RowDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->state.setFlag(QStyle::State_MouseOver, index.row() == table->hoveredRow);
    }

    SelectionTable::SelectionTable(const QStringList& columns, bool multiple, QWidget* parent)
        : QTableWidget(parent), multiple(multiple)
    {
        setObjectName("selectionTable");
        QStringList headers{"Selected"};
        headers.append(columns);
        setColumnCount(headers.size());
        setHorizontalHeaderLabels(headers);
        verticalHeader()->hide();
        verticalHeader()->setDefaultSectionSize(32);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setAlternatingRowColors(true);
        setShowGrid(false);
        setMouseTracking(true);
        setItemDelegate(new RowDelegate(this));
        horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        horizontalHeader()->setStretchLastSection(true);
        horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        setColumnWidth(0, 76);
        connect(this, &QTableWidget::cellClicked, this, [this](int row, int) { toggleRow(row); });
    }

    void SelectionTable::updateHoveredRow(int row)
    {
        if (hoveredRow == row) return;
        const int previousRow = hoveredRow;
        hoveredRow = row;
        if (previousRow >= 0) viewport()->update(0, rowViewportPosition(previousRow), viewport()->width(), rowHeight(previousRow));
        if (hoveredRow >= 0) viewport()->update(0, rowViewportPosition(hoveredRow), viewport()->width(), rowHeight(hoveredRow));
    }

    void SelectionTable::mouseMoveEvent(QMouseEvent* event)
    {
        updateHoveredRow(rowAt(event->position().toPoint().y()));
        QTableWidget::mouseMoveEvent(event);
    }

    bool SelectionTable::viewportEvent(QEvent* event)
    {
        if (event->type() == QEvent::Leave) updateHoveredRow(-1);
        return QTableWidget::viewportEvent(event);
    }

    void SelectionTable::appendRow(const QString& id, const QStringList& values, bool checked)
    {
        const QSignalBlocker blocker(this);
        const int row = rowCount();
        insertRow(row);
        QTableWidgetItem* marker = new QTableWidgetItem;
        marker->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        marker->setTextAlignment(Qt::AlignCenter);
        marker->setData(IdRole, id);
        setItem(row, 0, marker);
        for (int column = 1; column < columnCount(); column++)
            setItem(row, column, new QTableWidgetItem(values.value(column - 1)));
        setRowChecked(row, checked);
    }

    bool SelectionTable::isRowChecked(int row) const
    {
        return row >= 0 && row < rowCount() && item(row, 0)->data(Qt::UserRole).toBool();
    }

    void SelectionTable::setRowChecked(int row, bool checked)
    {
        if (row < 0 || row >= rowCount()) return;
        if (checked && !multiple)
            for (int index = 0; index < rowCount(); index++)
                if (index != row && isRowChecked(index)) setRowChecked(index, false);
        QTableWidgetItem* marker = item(row, 0);
        marker->setData(Qt::UserRole, checked);
        marker->setText(checked ? QString(QChar(0x2713)) : QString());
        marker->setData(Qt::AccessibleTextRole, checked ? "Selected" : "Not selected");
        emit checkedRowsChanged();
    }

    void SelectionTable::toggleRow(int row) { setRowChecked(row, !isRowChecked(row)); }

    void SelectionTable::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Space)
        {
            toggleRow(currentRow());
            event->accept();
            return;
        }
        QTableWidget::keyPressEvent(event);
    }

    QStringList SelectionTable::getCheckedIds() const
    {
        QStringList ids;
        for (int row = 0; row < rowCount(); row++)
            if (isRowChecked(row)) ids.append(item(row, 0)->data(IdRole).toString());
        return ids;
    }

    void SelectionTable::filterRows(const QString& query)
    {
        for (int row = 0; row < rowCount(); row++)
        {
            QStringList values;
            for (int column = 1; column < columnCount(); column++) values.append(item(row, column)->text());
            setRowHidden(row, !values.join(' ').contains(query.trimmed(), Qt::CaseInsensitive));
        }
    }

    void SelectionTable::selectVisible(bool checked)
    {
        for (int row = 0; row < rowCount(); row++)
            if (!isRowHidden(row))
            {
                setRowChecked(row, checked);
                if (checked && !multiple) break;
            }
    }

    bool SelectionTable::isMultiple() const { return multiple; }
}
