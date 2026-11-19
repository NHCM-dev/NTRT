#ifndef NTRT_LISTMANAGERDIALOG_H
#define NTRT_LISTMANAGERDIALOG_H
#include <QDialog>
#include "main/ui/widgets/SelectionTable.h"
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPair>
#include <QStringList>
#include <functional>
#include <vector>

namespace trtui
{
    class ListManagerDialog final : public QDialog
    {
    public:
        using Items = QList<QPair<QString, QString>>;
    private:
        struct Action
        {
            QPushButton* button;
            int minimum;
            int maximum;
        };
        SelectionTable* list = nullptr;
        QLineEdit* search = nullptr;
        QLabel* count = nullptr;
        QHBoxLayout* actions = nullptr;
        std::function<Items()> provider;
        std::vector<Action> buttons;
        bool multiple;
        void initUi();
        void initSearch(QVBoxLayout* layout);
        void initFooter(QVBoxLayout* layout);
        void updateActions();
        void refresh();
    public:
        ListManagerDialog(const QString& title, std::function<Items()> provider, QWidget* parent, bool multiple = true);
        QStringList getSelectedIds() const;
        void reload();
        void addAction(const QString& text, std::function<void(const QStringList&)> callback, int minimum = 0, int maximum = -1);
    };
}
#endif
