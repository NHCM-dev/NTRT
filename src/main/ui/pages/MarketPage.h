//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_MARKETPAGE_H
#define NTRT_MARKETPAGE_H
#include <QTableWidget>

#include "main/ui/AbstractPage.h"

namespace trtui
{
    class MarketPage final : public AbstractPage
    {
        QTableWidget* table = nullptr;
    public:

        explicit MarketPage(QWidget* parent = nullptr)
        {
            initialize(parent);
        }

        QWidget* generateWidget(QWidget* parent) override;

        void update() override;
    };
} // trtui

#endif //NTRT_MARKETPAGE_H
