#ifndef NTRT_SETTINGSDIALOG_H
#define NTRT_SETTINGSDIALOG_H
#include <QDialog>
#include <QShowEvent>
#include <memory>
#include "main/ui/pages/SettingPage.h"

namespace trtui
{
    class SettingsDialog final : public QDialog
    {
        std::unique_ptr<SettingPage> page;

        void initContent();

        void showEvent(QShowEvent* event) override;

    public:
        explicit SettingsDialog(QWidget* parent = nullptr);

        ~SettingsDialog() override;
    };
}
#endif //NTRT_SETTINGSDIALOG_H
