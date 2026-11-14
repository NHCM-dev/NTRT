#include "SettingsDialog.h"
#include "../pages/SettingPage.h"
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace trtui
{
    SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
    {
        setWindowTitle("Settings");
        resize(850, 620);
        initContent();
    }

    SettingsDialog::~SettingsDialog() = default;

    void SettingsDialog::initContent()
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 8);
        page = std::make_unique<SettingPage>(this);
        layout->addWidget(page->widget, 1);
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }
}
