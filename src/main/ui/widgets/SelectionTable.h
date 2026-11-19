#ifndef NTRT_SELECTIONTABLE_H
#define NTRT_SELECTIONTABLE_H
#include <QTableWidget>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QEvent>
#include <QStringList>

namespace trtui
{
    class SelectionTable final : public QTableWidget
    {
        Q_OBJECT
        class RowDelegate final : public QStyledItemDelegate
        {
            SelectionTable* table;
            void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
        public:
            explicit RowDelegate(SelectionTable* table);
        };
        bool multiple;
        int hoveredRow = -1;
        void updateHoveredRow(int row);
        void mouseMoveEvent(QMouseEvent* event) override;
        bool viewportEvent(QEvent* event) override;
        void toggleRow(int row);
        void keyPressEvent(QKeyEvent* event) override;
    public:
        static constexpr int IdRole = Qt::UserRole + 1;
        SelectionTable(const QStringList& columns, bool multiple, QWidget* parent = nullptr);
        void appendRow(const QString& id, const QStringList& values, bool checked = false);
        bool isRowChecked(int row) const;
        void setRowChecked(int row, bool checked);
        QStringList getCheckedIds() const;
        void filterRows(const QString& query);
        void selectVisible(bool checked);
        bool isMultiple() const;
    signals:
        void checkedRowsChanged();
    };
}
#endif
