#ifndef NTRT_SETTINGPAGE_H
#define NTRT_SETTINGPAGE_H

#include "main/ui/AbstractPage.h"
#include <QStringList>
#include <functional>

class QHBoxLayout;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class QVBoxLayout;

namespace trtdata
{
    enum class EnumTheme;
}

namespace trtui
{
    class SettingPage final : public AbstractPage
    {
    public:
        explicit SettingPage(QWidget* parent = nullptr);

        QFrame* generateWidget(QWidget* parent) override;
        void update() override {}

    private:
        QLineEdit* search = nullptr;
        QListWidget* categories = nullptr;
        QStackedWidget* sections = nullptr;

        void initSidebar(QFrame* page, QHBoxLayout* layout);
        void initGeneralSection();
        void initAppearanceSection();
        void initTradingSection();
        void initAboutSection();
        void initSearch();
        void updateCategoryPalette();
        bool saveAppearance(trtdata::EnumTheme theme);

        QVBoxLayout* addSection(const QString& title, const QString& group, const QString& keywords = {});
        void addRow(QVBoxLayout* rows, const QString& name, const QString& description, QWidget* control);
        void addDropdown(QVBoxLayout* rows, const QString& name, const QString& description, const QStringList& options,
            int selected, std::function<bool(int)> onChange);
        void addToggle(QVBoxLayout* rows, const QString& name, const QString& description, bool checked, std::function<bool(bool)> onChange);
        void addText(QVBoxLayout* rows, const QString& name, const QString& description, const QString& value,
            std::function<void(const QString&)> onEdit);
        void addButton(QVBoxLayout* rows, const QString& name, const QString& description, const QString& caption,
            std::function<void()> onClick);
    };
}

#endif //NTRT_SETTINGPAGE_H
