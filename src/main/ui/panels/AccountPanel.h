#ifndef NTRT_ACCOUNTPANEL_H
#define NTRT_ACCOUNTPANEL_H
#include <QFrame>
#include <QStringList>
#include <vector>
#include <QLabel>
#include <QEvent>
#include <QTableWidget>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "main/trade/AccountDef.h"
#include "main/data/core/Workspace.h"
namespace trtui
{
    class AccountPanel final : public QFrame
    {
        trtdata::WorkspaceWindowType type;
        QLabel* status = nullptr;
        QTableWidget* table = nullptr;
        qint64 displayedTimestamp = -1;
        QString historyId;
        QDateTimeEdit* beginTime = nullptr;
        QDateTimeEdit* endTime = nullptr;
        QComboBox* instrumentType = nullptr;
        QComboBox* historySource = nullptr;
        QLineEdit* instrument = nullptr;
        QPushButton* older = nullptr;
        trttrade::AccountHistoryQuery historyQuery;
        void initHistoryUi(QVBoxLayout* layout);
        void queryHistory(bool append);
        void initUi();
        void updateColumnWidths();
        void updateColors();
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void displayRows(const std::vector<QStringList>& rows);
    public:
        explicit AccountPanel(trtdata::WorkspaceWindowType type, QWidget* parent = nullptr);
        ~AccountPanel() override;
        trtdata::WorkspaceWindowType getType() const;
        void updateData();
        QSize sizeHint() const override;
    };
}
#endif
