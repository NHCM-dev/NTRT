//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_SETTINGWINDOW_H
#define NTRT_SETTINGWINDOW_H
#include "dialog/InstrumentDialog.h"

namespace trtui
{
    class SettingWindow final : public QDialog
    {
    public:
        explicit SettingWindow(QWidget* parent = nullptr);
    };
} // trtui

#endif //NTRT_SETTINGWINDOW_H
