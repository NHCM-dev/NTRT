//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_INSTRUMENTLISTPANEL_H
#define NTRT_INSTRUMENTLISTPANEL_H
#include <QTableWidget>

#include "main/ui/AbstractPage.h"

class QComboBox;
class QLineEdit;
class QVBoxLayout;

namespace trtui
{
    class InstrumentListPanel final : public AbstractPage
    {
        QTableWidget* table = nullptr;
        QLineEdit* search = nullptr;
        QComboBox* typeFilter = nullptr;

        void initFilters(QFrame* page, QVBoxLayout* layout);
        void initTable(QFrame* page, QVBoxLayout* layout);
        void initActions(QFrame* page, QVBoxLayout* layout);
        void updateColumnVisibility();
        QTableWidgetItem* updateItem(int row, int column, const QString& text);

    public:

        explicit InstrumentListPanel(QWidget* parent = nullptr)
        {
            initialize(parent);
        }

        QFrame* generateWidget(QWidget* parent) override;

        void update() override;
    };
} // trtui

#endif //NTRT_INSTRUMENTLISTPANEL_H
