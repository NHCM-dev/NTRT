//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_MARKETPAGE_H
#define NTRT_MARKETPAGE_H
#include <QTableWidget>

#include "main/ui/AbstractPage.h"

class QComboBox;
class QLineEdit;
class QVBoxLayout;

namespace trtui
{
    class MarketPage final : public AbstractPage
    {
        QTableWidget* table = nullptr;
        QLineEdit* search = nullptr;
        QComboBox* typeFilter = nullptr;

        void initFilters(QFrame* page, QVBoxLayout* layout);
        void initTable(QFrame* page, QVBoxLayout* layout);
        void initActions(QFrame* page, QVBoxLayout* layout);
        void initChartPlaceholder(QFrame* page);
        void updateColumnVisibility();

    public:

        explicit MarketPage(QWidget* parent = nullptr)
        {
            initialize(parent);
        }

        QFrame* generateWidget(QWidget* parent) override;

        void update() override;
    };
} // trtui

#endif //NTRT_MARKETPAGE_H
