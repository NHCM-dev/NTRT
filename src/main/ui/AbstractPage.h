//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_ABSTRACTPAGE_H
#define NTRT_ABSTRACTPAGE_H

#include <QFrame>

namespace trtui
{
    class AbstractPage
    {
    public:
        QFrame* widget = nullptr;

        virtual ~AbstractPage() = default;

        void initialize(QWidget* parent = nullptr)
        {
            widget = generateWidget(parent);
        }

        virtual QFrame* generateWidget(QWidget* parent) = 0;
        virtual void update() = 0;
    };
} // trtui

#endif //NTRT_ABSTRACTPAGE_H
