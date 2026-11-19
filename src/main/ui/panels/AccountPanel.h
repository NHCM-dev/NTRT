#ifndef NTRT_ACCOUNTPANEL_H
#define NTRT_ACCOUNTPANEL_H
#include <QFrame>
#include <QStringList>
#include <vector>
#include <QLabel>
#include <QEvent>
#include <QTableWidget>
#include <QDateEdit>
#include <QTimer>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHash>
#include <QVariantAnimation>
#include <QPropertyAnimation>
#include <QTableView>
#include "main/trade/AccountDef.h"
#include "main/data/core/WorkspaceDef.h"
namespace trtui
{
    class AccountPanel final : public QFrame
    {
        trtdata::WorkspaceWindowType type;
        QLabel* status = nullptr;
        QTableWidget* table = nullptr;
        qint64 displayedTimestamp = -1;
        QString historyId;
        QDateEdit* beginDate = nullptr;
        QDateEdit* endDate = nullptr;
        QComboBox* instrumentType = nullptr;
        QComboBox* historySource = nullptr;
        QLineEdit* instrument = nullptr;
        QTimer historyInputDelay;
        QHash<QWidget*, QVariantAnimation*> historyHover;
        struct DayHover
        {
            QTableView* days;
            QFrame* highlight;
            QPropertyAnimation* animation;
        };
        QHash<QWidget*, DayHover> historyDayHovers;
        void initHistoryUi(QVBoxLayout* layout);
        void initHistoryDates(QVBoxLayout* layout);
        void initHistoryHover(QWidget* control);
        void initHistoryDayHover(QTableView* days);
        void updateHistoryDayHover(const DayHover& hover, const QRect& cell);
        void scheduleHistoryQuery();
        void queryHistory(bool refresh);
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
