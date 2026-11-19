#ifndef NTRT_ACCOUNTPANEL_H
#define NTRT_ACCOUNTPANEL_H
#include <QFrame>
#include <QStringList>
#include <vector>
#include <QLabel>
#include <QEvent>
#include <QTableWidget>
#include "main/data/core/Workspace.h"
namespace trtui
{
    class AccountPanel final : public QFrame
    {
        trtdata::WorkspaceWindowType type;
        QLabel* status = nullptr;
        QTableWidget* table = nullptr;
        qint64 displayedTimestamp = -1;
        void initUi();
        void updateColumnWidths();
        void updateColors();
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void displayRows(const std::vector<QStringList>& rows);
    public:
        explicit AccountPanel(trtdata::WorkspaceWindowType type, QWidget* parent = nullptr);
        trtdata::WorkspaceWindowType getType() const;
        void updateData();
        QSize sizeHint() const override;
    };
}
#endif
