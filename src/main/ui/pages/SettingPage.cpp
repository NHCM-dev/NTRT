#include <QTimeZone>
#include "SettingPage.h"
#include "main/util/TradeUtil.h"
#include "main/util/PaletteUtil.h"
#include "main/NTRT.h"
#include "main/Ver.h"
#include "main/api/AbstractApi.h"
#include <QInputDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QScreen>
#include <QBoxLayout>
#include <algorithm>

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
        initAppearanceSection();
        initTradingSection();
        initApiSection();
        initAboutSection();
        initSearch();
        return page;
    }

    void SettingPage::initSidebar(QFrame* page, QHBoxLayout* layout)
    {
        QFrame* sidebar = new QFrame(page);
        sidebar->setFixedWidth(std::clamp(page->screen()->availableGeometry().width() / 5, 100, 172));
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
        categories->setObjectName("settingsCategories");
        categories->setFrameShape(QFrame::NoFrame);
        categories->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
        categories->setSpacing(2);

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

    void SettingPage::initAppearanceSection()
    {
        QVBoxLayout* rows = addSection("Appearance", "Display", "light dark display scale scaling zoom size custom restart");
        const trtdata::AppearanceManager* appearance = ntrt::configManager->getAppearanceManager();
        const int selected = appearance->getCurrentTheme() == trtdata::Theme::Dark ? 1 : 0;
        addDropdown(rows, "Theme", "Choose a light or dark interface.", {"Light", "Dark"}, selected, [this](int index)
        {
            return saveAppearance(index == 0 ? trtdata::Theme::Light : trtdata::Theme::Dark);
        });
        initDisplayScaleRows(rows);
    }

    void SettingPage::initDisplayScaleRows(QVBoxLayout* rows)
    {
        trtdata::AppearanceManager* appearance = ntrt::configManager->getAppearanceManager();
        QWidget* controls = new QWidget(rows->parentWidget());
        const bool compact = controls->screen()->availableGeometry().width() < 700;
        QBoxLayout* layout = new QBoxLayout(compact ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight, controls);
        layout->setContentsMargins(0, 0, 0, 0);
        QComboBox* presets = new QComboBox(controls);
        presets->setObjectName("displayScalePresets");
        presets->setAccessibleName("Display scale");
        for (int percent = trtdata::MIN_DISPLAY_SCALE_PERCENT; percent <= trtdata::MAX_DISPLAY_SCALE_PERCENT; percent += 25)
            presets->addItem(QString::number(percent) + "%", percent);
        presets->addItem("Custom…", 0);
        const int savedIndex = presets->findData(appearance->getDisplayScalePercent());
        presets->setCurrentIndex(savedIndex >= 0 ? savedIndex : presets->count() - 1);
        QSpinBox* custom = new QSpinBox(controls);
        custom->setObjectName("displayScaleCustom");
        custom->setAccessibleName("Custom display scale");
        custom->setRange(trtdata::MIN_DISPLAY_SCALE_PERCENT, trtdata::MAX_DISPLAY_SCALE_PERCENT);
        custom->setSuffix("%");
        custom->setKeyboardTracking(false);
        custom->setValue(appearance->getDisplayScalePercent());
        custom->setVisible(savedIndex < 0);
        QPushButton* apply = new QPushButton("Apply", controls);
        apply->setObjectName("displayScaleApply");
        layout->addWidget(presets);
        layout->addWidget(custom);
        layout->addWidget(apply);
        addRow(rows, "Display scale", "Scale the entire interface relative to your system display setting. "
               "Choose 25–400%, or a custom whole percentage. Restart to apply.", controls);

        QLabel* status = new QLabel(rows->parentWidget());
        status->setObjectName("displayScaleStatus");
        status->setWordWrap(true);
        rows->addWidget(status);
        QPushButton* restart = new QPushButton("Restart now", rows->parentWidget());
        restart->setObjectName("displayScaleRestart");
        restart->setEnabled(appearance->isDisplayScaleRestartRequired());
        addRow(rows, "Apply after restart", "Restart saves your workspace layout and settings before reopening NTRT.", restart);
        if (appearance->isDisplayScaleRestartRequired())
            status->setText(QString("Saved: %1%. Restart to apply.").arg(appearance->getDisplayScalePercent()));
        QObject::connect(presets, &QComboBox::currentIndexChanged, controls, [presets, custom](int)
        {
            const int percent = presets->currentData().toInt();
            custom->setVisible(percent == 0);
            if (percent != 0)
                custom->setValue(percent);
        });
        QObject::connect(apply, &QPushButton::clicked, controls, [appearance, presets, custom, status, restart]
        {
            custom->interpretText();
            const int preset = presets->currentData().toInt();
            const int percent = preset == 0 ? custom->value() : preset;
            if (!appearance->setDisplayScalePercent(percent))
            {
                status->setText("Could not save display scale. The saved setting has not changed.");
                return;
            }
            const bool pending = appearance->isDisplayScaleRestartRequired();
            restart->setEnabled(pending);
            status->setText(pending ? QString("Saved: %1%. Restart to apply.").arg(percent) : "This display scale is already active.");
        });
        QObject::connect(restart, &QPushButton::clicked, restart, [status]
        {
            if (!ntrt::restartApp())
                status->setText("Could not save settings or workspace layout, or restart NTRT. The application is still open.");
        });
    }

    bool SettingPage::saveAppearance(trtdata::Theme theme)
    {
        trtdata::AppearanceManager* appearance = ntrt::configManager->getAppearanceManager();
        const trtdata::Theme previous = appearance->getCurrentTheme();
        appearance->setTheme(theme);
        if (!appearance->saveConfig())
        {
            appearance->setTheme(previous);
            return false;
        }

        trtutils::applyPalette(theme);

        return true;
    }

    void SettingPage::initTradingSection()
    {
        QVBoxLayout* rows = addSection("Trading", "Market",
                                       "price flash color exchange type volume columns change amount daily OHLC data time zone timezone UTC");
        trtdata::ConfigManager& config = *ntrt::configManager;
        QStringList displayZones;
        for (const QByteArray& id : QTimeZone::availableTimeZoneIds())
            displayZones.append(QString::fromUtf8(id));
        if (!displayZones.contains("UTC"))
            displayZones.prepend("UTC");
        const QString currentZone = QString::fromStdString(config.getDisplayTimeZone());
        if (!displayZones.contains(currentZone))
            displayZones.prepend(currentZone);
        addDropdown(rows, "Display time zone", "Chart timestamps and the clock use this time zone, including daylight saving time.",
                    displayZones, displayZones.indexOf(currentZone), [&config, displayZones](int index)
                    {
                        return config.setDisplayTimeZone(displayZones.at(index).toStdString());
                    });
        int selectedTimeZone = 0;
        switch (config.getTradingDataTimeZone())
        {
            case trtdata::TradingTimeZone::UTC:
                selectedTimeZone = 0;
                break;
            case trtdata::TradingTimeZone::UTC8:
                selectedTimeZone = 1;
                break;
        }
        addDropdown(rows, "Trading time zone",
                    "Daily open and volume use UTC or the exchange trading day (OKX: UTC+8).",
                    {"UTC", "Exchange time zone (OKX: UTC+8)"}, selectedTimeZone, [&config](int index)
                    {
                        switch (index)
                        {
                            case 0:
                                return config.setTradingDataTimeZone(trtdata::TradingTimeZone::UTC);
                            case 1:
                                return config.setTradingDataTimeZone(trtdata::TradingTimeZone::UTC8);
                            default:
                                return false;
                        }
                    });
        trtdata::AppearanceManager* appearance = ntrt::configManager->getAppearanceManager();
        addToggle(rows, "Flash on price change", "Flash the latest price and highlight the changed digits.",
                  appearance->isPriceFlashEnabled(), [appearance](bool enabled)
                  {
                      return appearance->setPriceFlashEnabled(enabled);
                  });
        addToggle(rows, "Show Change", "Show the price change from the trading day's open in Instrument List.",
                  appearance->getMarketColumns().changeAmount, [appearance](bool enabled)
                  {
                      trtdata::MarketColumnVisibility columns = appearance->getMarketColumns();
                      columns.changeAmount = enabled;
                      return appearance->setMarketColumns(columns);
                  });
        addToggle(rows, "Show Icon", "Show the icon for each instrument.", appearance->getMarketColumns().icon,
                  [appearance](bool enabled)
                  {
                      trtdata::MarketColumnVisibility columns = appearance->getMarketColumns();
                      columns.icon = enabled;
                      return appearance->setMarketColumns(columns);
                  });
        addToggle(rows, "Show Exchange", "Show the exchange for each instrument.",
                  appearance->getMarketColumns().exchange,
                  [appearance](bool enabled)
                  {
                      trtdata::MarketColumnVisibility columns = appearance->getMarketColumns();
                      columns.exchange = enabled;
                      return appearance->setMarketColumns(columns);
                  });
        addToggle(rows, "Show Type", "Show the instrument type.", appearance->getMarketColumns().type,
                  [appearance](bool enabled)
                  {
                      trtdata::MarketColumnVisibility columns = appearance->getMarketColumns();
                      columns.type = enabled;
                      return appearance->setMarketColumns(columns);
                  });
        addToggle(rows, "Show Day Volume", "Show today's candle volume in the trading data time zone.",
                  appearance->getMarketColumns().volume24h,
                  [appearance](bool enabled)
                  {
                      trtdata::MarketColumnVisibility columns = appearance->getMarketColumns();
                      columns.volume24h = enabled;
                      return appearance->setMarketColumns(columns);
                  });
    }

    void SettingPage::initApiSection()
    {
        QVBoxLayout* rows = addSection("API", "OKX", "okx api key secret passphrase import account credentials");
        initOKXApiRows(rows);
    }

    void SettingPage::initOKXApiRows(QVBoxLayout* rows)
    {
        trtdata::OKXCredentials saved;
        bool found = false;
        const bool loaded = ntrt::configManager->getAccountManager()->loadOKXCredentials(saved, found);
        if (!loaded)
        {
            QLabel* status = new QLabel("Could not read saved credentials.", rows->parentWidget());
            status->setWordWrap(true);
            rows->addWidget(status);
        }
        addOKXCredentialRow(rows, "API Key", "Your OKX API key.", trtdata::OKXCredentialField::ApiKey, !saved.apiKey.isEmpty());
        addOKXCredentialRow(rows, "Secret Key", "The secret issued with your API key.", trtdata::OKXCredentialField::SecretKey, !saved.secretKey.isEmpty());
        addOKXCredentialRow(rows, "Passphrase", "The passphrase chosen when creating the API key.", trtdata::OKXCredentialField::Passphrase, !saved.passphrase.isEmpty());
    }

    void SettingPage::addOKXCredentialRow(QVBoxLayout* rows, const QString& name, const QString& description,
                                            trtdata::OKXCredentialField field, bool configured)
    {
        QPushButton* control = new QPushButton(configured ? "Change…" : "Set…", rows->parentWidget());
        addRow(rows, name, description, control);
        QObject::connect(control, &QPushButton::clicked, control, [control, name, field]
        {
            bool accepted = false;
            const QString value = QInputDialog::getText(control, "OKX · " + name, "Enter " + name + ":",
                QLineEdit::Password, {}, &accepted);
            if (!accepted)
                return;
            if (value.trimmed().isEmpty())
            {
                QMessageBox::warning(control, "OKX", name + " cannot be empty.");
                return;
            }
            if (!ntrt::configManager->getAccountManager()->setOKXCredential(field, value))
            {
                QMessageBox::warning(control, "OKX", "Could not save " + name + ". Existing credentials were retained.");
                return;
            }
            control->setText("Change…");
        });
    }

    void SettingPage::initAboutSection()
    {
        QVBoxLayout* rows = addSection("About", "Application", "app version");
        addButton(rows, QString::fromStdString("About " + NTRT_NAME),
                  QString::fromStdString(NTRT_NAME + " " + NTRT_FULL_VER), "About", [this]
                  {
                      ntrt::displayAbout(this->widget);
                  });
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
        const bool compact = panel->screen()->availableGeometry().width() < 700;
        QBoxLayout* row = new QBoxLayout(compact ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight, panel);
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

    void SettingPage::addDropdown(QVBoxLayout* rows, const QString& name, const QString& description,
                                  const QStringList& options,
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

    void SettingPage::addToggle(QVBoxLayout* rows, const QString& name, const QString& description, bool checked,
                                std::function<bool(bool)> onChange)
    {
        QCheckBox* control = new QCheckBox(rows->parentWidget());
        control->setChecked(checked);
        addRow(rows, name, description, control);
        QObject::connect(control, &QCheckBox::toggled, control, [control, onChange = std::move(onChange)](bool enabled)
        {
            if (onChange(enabled))
                return;
            const QSignalBlocker blocker(control);
            control->setChecked(!enabled);
            QMessageBox::warning(control, "Settings", "Could not save settings.");
        });
    }

    void SettingPage::addText(QVBoxLayout* rows, const QString& name, const QString& description, const QString& value,
                              std::function<void(const QString &)> onEdit)
    {
        QLineEdit* control = new QLineEdit(value, rows->parentWidget());
        addRow(rows, name, description, control);
        QObject::connect(control, &QLineEdit::editingFinished, control, [control, onEdit = std::move(onEdit)]
        {
            onEdit(control->text());
        });
    }

    void SettingPage::addButton(QVBoxLayout* rows, const QString& name, const QString& description,
                                const QString& caption,
                                std::function<void()> onClick)
    {
        QPushButton* control = new QPushButton(caption, rows->parentWidget());
        addRow(rows, name, description, control);
        QObject::connect(control, &QPushButton::clicked, control, std::move(onClick));
    }
}
