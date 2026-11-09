//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_MARKETPAGE_H
#define NTRT_MARKETPAGE_H
#include <QTableWidget>

#include "main/ui/AbstractPage.h"

class QComboBox;
class QLineEdit;

namespace trtui
{
    class MarketPage final : public AbstractPage
    {
        QTableWidget* table = nullptr;
        QLineEdit* search = nullptr;
        QComboBox* typeFilter = nullptr;


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
