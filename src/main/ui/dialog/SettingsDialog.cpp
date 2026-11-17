#include "SettingsDialog.h"
#include "../pages/SettingPage.h"
#include "main/util/QtUtil.h"
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QScreen>
#include <QTimer>

namespace trtui
{
    SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
    {
        setWindowTitle("Settings");
        initContent();
        resize(QSize(850, 620).boundedTo(screen()->availableGeometry().size()));
    }

    SettingsDialog::~SettingsDialog() = default;

    void SettingsDialog::showEvent(QShowEvent* event)
    {
        QDialog::showEvent(event);
        QTimer::singleShot(0, this, [this]
        {
            trtutils::fitWindowToScreen(this);
        });
    }

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
