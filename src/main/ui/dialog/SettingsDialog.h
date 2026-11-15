#ifndef NTRT_SETTINGSDIALOG_H
#define NTRT_SETTINGSDIALOG_H
#include <QDialog>
#include <memory>

namespace trtui
{
    class SettingPage;

    class SettingsDialog final : public QDialog
    {
        std::unique_ptr<SettingPage> page;

        void initContent();

    public:
        explicit SettingsDialog(QWidget* parent = nullptr);

        ~SettingsDialog() override;
    };
}
#endif //NTRT_SETTINGSDIALOG_H
