#include "SettingPage.h"
#include "main/NTRT.h"
#include "main/Ver.h"
#include "main/data/api/AbstractApi.h"
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace trtui
{
    SettingPage::SettingPage(QWidget* parent)
    {
        initialize(parent);
    }

    QFrame* SettingPage::generateWidget(QWidget* parent)
    {
        QFrame* page = new QFrame(parent);
        page->setBackgroundRole(QPalette::Base);
        page->setAutoFillBackground(true);
        QHBoxLayout* layout = new QHBoxLayout(page);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        initSidebar(page, layout);
        sections = new QStackedWidget(page);
        layout->addWidget(sections, 1);
        initGeneralSection();
        initAboutSection();
        initSearch();
        return page;
    }

    void SettingPage::initSidebar(QFrame* page, QHBoxLayout* layout)
    {
        QFrame* sidebar = new QFrame(page);
        sidebar->setFixedWidth(172);
        sidebar->setBackgroundRole(QPalette::Base);
        sidebar->setAutoFillBackground(true);
        QVBoxLayout* navigation = new QVBoxLayout(sidebar);
        navigation->setContentsMargins(12, 16, 12, 12);
        QLabel* title = new QLabel("Settings", sidebar);
        QFont titleFont = title->font();
        titleFont.setBold(true);
        title->setFont(titleFont);
        navigation->addWidget(title);
        search = new QLineEdit(sidebar);
        search->setPlaceholderText("Search settings");
        navigation->addWidget(search);
        categories = new QListWidget(sidebar);
        categories->setFrameShape(QFrame::NoFrame);
        categories->setSpacing(2);
        QPalette colors = categories->palette();
        const QColor base = colors.color(QPalette::Base);
        const QColor text = colors.color(QPalette::Text);
        const QColor selection = QColor::fromRgbF(
            base.redF() * 0.88 + text.redF() * 0.12,
            base.greenF() * 0.88 + text.greenF() * 0.12,
            base.blueF() * 0.88 + text.blueF() * 0.12);
        colors.setColor(QPalette::Highlight, selection);
        colors.setColor(QPalette::HighlightedText, text);
        categories->setPalette(colors);
        navigation->addWidget(categories, 1);
        layout->addWidget(sidebar);
        QFrame* sidebarDivider = new QFrame(page);
        sidebarDivider->setFrameShape(QFrame::VLine);
        sidebarDivider->setFrameShadow(QFrame::Sunken);
        layout->addWidget(sidebarDivider);
    }

    void SettingPage::initGeneralSection()
    {
        QVBoxLayout* rows = addSection("General", "Trading preferences", "default exchange trading pairs");
        trtdata::ConfigManager& config = *ntrt::configManager;
        const std::vector<trtdata::Exchange> exchanges = trtcore::availableExchanges();
        QStringList names;
        int selected = -1;
        for (trtdata::Exchange exchange : exchanges)
        {
            if (exchange == config.getExchange())
                selected = static_cast<int>(names.size());
            names.append(QString::fromUtf8(trtdata::getExchangeStr(exchange)));
        }

        addDropdown(rows, "Default exchange", "Preselected when you open Add Trading Pairs.", names, selected,
            [exchanges, &config](int index) -> bool
            {
                const trtdata::Exchange previous = config.getExchange();
                config.setExchange(exchanges[index]);
                if (config.saveExchangeConfig())
                    return true;

                config.setExchange(previous);
                return false;
            });
    }

    void SettingPage::initAboutSection()
    {
        QVBoxLayout* rows = addSection("About", "Application", "app version");
        addRow(rows, QString::fromStdString(NTRT_NAME), "Installed version", new QLabel(QString::fromStdString(NTRT_FULL_VER)));
    }

    void SettingPage::initSearch()
    {
        QWidget* empty = new QWidget(sections);
        QVBoxLayout* emptyLayout = new QVBoxLayout(empty);
        emptyLayout->setSpacing(6);
        emptyLayout->addStretch();
        QLabel* emptyTitle = new QLabel("No results", empty);
        QFont emptyFont = emptyTitle->font();
        emptyFont.setPointSize(14);
        emptyFont.setBold(true);
        emptyTitle->setFont(emptyFont);
        emptyLayout->addWidget(emptyTitle, 0, Qt::AlignHCenter);
        QLabel* emptyHint = new QLabel("Try a different search term.", empty);
        emptyHint->setForegroundRole(QPalette::PlaceholderText);
        emptyLayout->addWidget(emptyHint, 0, Qt::AlignHCenter);
        emptyLayout->addStretch();
        const int emptyPage = sections->addWidget(empty);
        QObject::connect(categories, &QListWidget::currentRowChanged, sections, [this, emptyPage](int row)
        {
            sections->setCurrentIndex(row >= 0 ? row : emptyPage);
        });
        QObject::connect(search, &QLineEdit::textChanged, sections, [this, emptyPage](const QString& text)
        {
            int firstVisible = -1;
            for (int row = 0; row < categories->count(); row++)
            {
                QListWidgetItem* item = categories->item(row);
                const QString keywords = item->text() + " " + item->data(Qt::UserRole).toString();
                const bool visible = keywords.contains(text.trimmed(), Qt::CaseInsensitive);
                item->setHidden(!visible);
                if (visible && firstVisible < 0)
                    firstVisible = row;
            }
            if (!categories->currentItem() || categories->currentItem()->isHidden())
                categories->setCurrentRow(firstVisible);
            if (firstVisible < 0)
                sections->setCurrentIndex(emptyPage);
        });
        categories->setCurrentRow(0);
    }

    QVBoxLayout* SettingPage::addSection(const QString& title, const QString& group, const QString& keywords)
    {
        QListWidgetItem* category = new QListWidgetItem(title, categories);
        category->setData(Qt::UserRole, group + " " + keywords);
        category->setSizeHint(QSize(0, 32));
        QScrollArea* scroll = new QScrollArea(this->sections);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidgetResizable(true);
        QWidget* content = new QWidget;
        content->setBackgroundRole(QPalette::Base);
        content->setAutoFillBackground(true);
        QVBoxLayout* outer = new QVBoxLayout(content);
        outer->setContentsMargins(24, 28, 24, 28);

        QWidget* column = new QWidget(content);
        column->setMaximumWidth(720);
        QVBoxLayout* sections = new QVBoxLayout(column);
        sections->setContentsMargins(0, 0, 0, 0);
        sections->setSpacing(14);
        QHBoxLayout* centered = new QHBoxLayout;
        centered->addStretch();
        centered->addWidget(column, 8);
        centered->addStretch();
        outer->addLayout(centered);
        outer->addStretch();

        QLabel* heading = new QLabel(title, column);
        QFont font = heading->font();
        font.setPointSize(14);
        font.setBold(true);
        heading->setFont(font);
        sections->addWidget(heading);
        sections->addWidget(new QLabel(group, column));

        QFrame* card = new QFrame(column);
        card->setFrameShape(QFrame::NoFrame);
        QVBoxLayout* rows = new QVBoxLayout(card);
        rows->setContentsMargins(0, 0, 0, 0);
        rows->setSpacing(8);
        sections->addWidget(card);
        scroll->setWidget(content);
        this->sections->addWidget(scroll);
        return rows;
    }

    void SettingPage::addRow(QVBoxLayout* rows, const QString& name, const QString& description, QWidget* control)
    {
        QFrame* panel = new QFrame(rows->parentWidget());
        panel->setBackgroundRole(QPalette::Window);
        panel->setAutoFillBackground(true);
        QHBoxLayout* row = new QHBoxLayout(panel);
        row->setContentsMargins(14, 10, 14, 10);
        QVBoxLayout* text = new QVBoxLayout;
        text->setSpacing(4);
        QLabel* title = new QLabel(name);
        QFont font = title->font();
        font.setBold(true);
        title->setFont(font);
        text->addWidget(title);
        QLabel* detail = new QLabel(description);
        detail->setWordWrap(true);
        detail->setForegroundRole(QPalette::PlaceholderText);
        text->addWidget(detail);
        row->addLayout(text, 1);
        row->addWidget(control);
        rows->addWidget(panel);
    }

    void SettingPage::addDropdown(QVBoxLayout* rows, const QString& name, const QString& description, const QStringList& options,
        int selected, std::function<bool(int)> onChange)
    {
        QComboBox* control = new QComboBox(rows->parentWidget());
        control->addItems(options);
        control->setCurrentIndex(selected);
        addRow(rows, name, description, control);

        QLabel* status = new QLabel(rows->parentWidget());
        status->setWordWrap(true);
        status->hide();
        rows->addWidget(status);
        QObject::connect(control, &QComboBox::currentIndexChanged, control,
            [control, status, selected, onChange = std::move(onChange)](int index) mutable
            {
                if (index < 0)
                    return;
                if (onChange(index))
                {
                    selected = index;
                    status->hide();
                    return;
                }

                const QSignalBlocker blocker(control);
                control->setCurrentIndex(selected);
                status->setText("Could not save settings.");
                status->show();
            });
    }

    void SettingPage::addToggle(QVBoxLayout* rows, const QString& name, const QString& description, bool checked, std::function<void(bool)> onChange)
    {
        QCheckBox* control = new QCheckBox(rows->parentWidget());
        control->setChecked(checked);
        addRow(rows, name, description, control);
        QObject::connect(control, &QCheckBox::toggled, control, std::move(onChange));
    }

    void SettingPage::addText(QVBoxLayout* rows, const QString& name, const QString& description, const QString& value,
        std::function<void(const QString&)> onEdit)
    {
        QLineEdit* control = new QLineEdit(value, rows->parentWidget());
        addRow(rows, name, description, control);
        QObject::connect(control, &QLineEdit::editingFinished, control, [control, onEdit = std::move(onEdit)]
        {
            onEdit(control->text());
        });
    }

    void SettingPage::addButton(QVBoxLayout* rows, const QString& name, const QString& description, const QString& caption,
        std::function<void()> onClick)
    {
        QPushButton* control = new QPushButton(caption, rows->parentWidget());
        addRow(rows, name, description, control);
        QObject::connect(control, &QPushButton::clicked, control, std::move(onClick));
    }
}
