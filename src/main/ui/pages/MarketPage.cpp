//
// Created by NHCM on 2026/11/7.
//

#include "MarketPage.h"
#include "../dialog/InstrumentDialog.h"
#include <QTableWidget>
#include "main/NTRT.h"
#include <QHeaderView>
#include <QLocale>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <cmath>
#include <QPushButton>
#include <QObject>
#include <QComboBox>
#include <QLineEdit>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QVariant>
#include <QColor>
#include <QLabel>
#include <QSplitter>
#include <QMetaType>

#include "main/util/QtUtil.h"

namespace trtui
{
    inline const int fullFlashMs = 500;
    inline const int digitHighlightMs = 1000;

    class PriceDelegate final : public QStyledItemDelegate
    {
    public:
        explicit PriceDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

        static void setMovement(QTableWidgetItem* item, const trtcore::MarketSnapshot& snapshot, const QPalette& palette, bool flashEnabled)
        {
            if (!flashEnabled || !snapshot.priceChangeAge.isValid())
                return;

            const qint64 age = snapshot.priceChangeAge.elapsed();
            if (age >= fullFlashMs + digitHighlightMs)
                return;

            const trtdata::TickerData& ticker = *snapshot.ticker;
            if (!ticker.previousLast || ticker.last == *ticker.previousLast)
                return;

            const double difference = ticker.last - *ticker.previousLast;
            const bool dark = palette.color(QPalette::Base).lightness() < 128;
            const QColor color = difference > 0 ? QColor(dark ? 0x4ade80 : 0x15803d) : QColor(dark ? 0xfb7185 : 0xb91c1c);
            const QString price = item->text();
            const qsizetype start = age < fullFlashMs ? 0 : changedDigitOffset(price, std::abs(difference));
            if (start < price.size())
            {
                item->setData(Qt::UserRole, start);
                item->setData(Qt::UserRole + 1, color);
            }
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            const QVariant start = index.data(Qt::UserRole);
            QStyleOptionViewItem styled(option);
            initStyleOption(&styled, index);
            QStyle* style = styled.widget ? styled.widget->style() : QApplication::style();
            const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &styled, styled.widget) + 1;
            const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &styled, styled.widget).adjusted(margin, 0, -margin, 0);
            const QString price = styled.fontMetrics.elidedText(styled.text, styled.textElideMode, textRect.width());
            if (price.isEmpty())
            {
                QStyledItemDelegate::paint(painter, option, index);
                return;
            }
            styled.text.clear();
            style->drawControl(QStyle::CE_ItemViewItem, &styled, painter, styled.widget);

            QTextLayout text(price, styled.font);
            if (start.isValid() && start.toInt() < price.size())
            {
                QTextLayout::FormatRange suffix;
                suffix.start = start.toInt();
                suffix.length = static_cast<int>(price.size()) - suffix.start;
                suffix.format.setForeground(index.data(Qt::UserRole + 1).value<QColor>());
                text.setFormats({suffix});
            }
            text.beginLayout();
            QTextLine line = text.createLine();
            line.setLineWidth(textRect.width());
            text.endLayout();

            painter->save();
            painter->setClipRect(textRect, Qt::IntersectClip);
            painter->setPen(styled.palette.color(QPalette::Text));
            const QSize textSize(static_cast<int>(std::ceil(line.naturalTextWidth())), static_cast<int>(std::ceil(line.height())));
            const QRect aligned = QStyle::alignedRect(styled.direction, styled.displayAlignment, textSize, textRect);
            text.draw(painter, aligned.topLeft());
            painter->restore();
        }

    private:
        static qsizetype changedDigitOffset(const QString& price, double difference)
        {
            const int changedPower = static_cast<int>(std::floor(std::log10(difference) + 1e-9));
            const qsizetype exponentPosition = price.indexOf('e', 0, Qt::CaseInsensitive);
            const qsizetype end = exponentPosition < 0 ? price.size() : exponentPosition;
            const int exponent = exponentPosition < 0 ? 0 : price.mid(exponentPosition + 1).toInt();
            const qsizetype decimal = price.indexOf('.');
            const qsizetype integerEnd = decimal < 0 ? end : decimal;
            int power = exponent - 1;
            for (qsizetype index = 0; index < integerEnd; index++)
            {
                if (price[index].isDigit())
                    power++;
            }
            for (qsizetype index = 0; index < end; index++)
            {
                if (!price[index].isDigit())
                    continue;
                if (power <= changedPower)
                    return index;
                power--;
            }
            return price.size();
        }
    };

    QFrame* MarketPage::generateWidget(QWidget* parent)
    {
        QFrame* page = new QFrame(parent);

        page->setBackgroundRole(QPalette::Base);
        page->setAutoFillBackground(true);

        QHBoxLayout* layout = new QHBoxLayout(page);
        layout->setContentsMargins(0, 0, 0, 0);
        QSplitter* splitter = new QSplitter(Qt::Horizontal, page);
        splitter->setChildrenCollapsible(false);
        splitter->setObjectName("marketSplitter");
        splitter->setHandleWidth(12);
        layout->addWidget(splitter);

        QFrame* instruments = new QFrame(splitter);
        QVBoxLayout* instrumentLayout = new QVBoxLayout(instruments);
        initFilters(instruments, instrumentLayout);
        initTable(instruments, instrumentLayout);
        initActions(instruments, instrumentLayout);
        QFrame* chart = new QFrame(splitter);
        initChartPlaceholder(chart);
        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);
        splitter->setSizes({instruments->minimumSizeHint().width(), 10000});
        return page;
    }

    void MarketPage::initFilters(QFrame* page, QVBoxLayout* layout)
    {
        QHBoxLayout* filters = new QHBoxLayout;
        search = new QLineEdit(page);
        search->setPlaceholderText("Search instruments");
        typeFilter = new QComboBox(page);
        typeFilter->addItem("All types", -1);
        for (trttrade::InstType type : {trttrade::InstType::Spot, trttrade::InstType::Margin, trttrade::InstType::Swap,
                                     trttrade::InstType::Futures, trttrade::InstType::Option, trttrade::InstType::Events})
            typeFilter->addItem(QString::fromUtf8(instDisplayStr(type)), static_cast<int>(type));
        filters->addWidget(search, 1);
        filters->addWidget(typeFilter);
        layout->addLayout(filters);

        QObject::connect(search, &QLineEdit::textChanged, page, [this] { update(); });
        QObject::connect(typeFilter, &QComboBox::currentIndexChanged, page, [this] { update(); });
    }

    void MarketPage::initTable(QFrame* page, QVBoxLayout* layout)
    {
        table = trtutils::createHeaderTable(page, {"Exchange", "Type", "Icon", "Instrument", "Last Price", "Volume", "Change"});
        table->setItemDelegateForColumn(4, new PriceDelegate(table));

        QHeaderView* header = table->horizontalHeader();
        header->setSectionResizeMode(0, QHeaderView::Interactive);
        header->setSectionResizeMode(4, QHeaderView::Interactive);
        header->setSectionResizeMode(5, QHeaderView::Interactive);
        header->setSectionResizeMode(6, QHeaderView::Interactive);
        header->setSectionResizeMode(1, QHeaderView::Fixed);

        header->setMinimumSectionSize(32);
        header->setSectionResizeMode(2, QHeaderView::Fixed);
        header->setSectionResizeMode(3, QHeaderView::Stretch);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        table->setColumnWidth(0, 72);
        table->setColumnWidth(1, 100);
        table->setColumnWidth(4, 70);
        table->setColumnWidth(5, 130);
        table->setColumnWidth(6, 65);
        table->setColumnWidth(2, 48);
        table->verticalHeader()->hide();
        updateColumnVisibility();
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(table, 1);
    }

    void MarketPage::initActions(QFrame* page, QVBoxLayout* layout)
    {
        QHBoxLayout* actions = new QHBoxLayout;
        actions->addStretch();
        QPushButton* manageButton = new QPushButton("Manage", page);
        QPushButton* addButton = new QPushButton("Add", page);
        actions->addWidget(manageButton);
        actions->addWidget(addButton);
        layout->addLayout(actions);

        QObject::connect(addButton, &QPushButton::clicked, page, [this, page]
        {
            InstrumentDialog dialog(InstrumentDialog::Mode::Add, page);
            if (dialog.exec() == QDialog::Accepted)
                update();
        });
        QObject::connect(manageButton, &QPushButton::clicked, page, [this, page]
        {
            InstrumentDialog dialog(InstrumentDialog::Mode::Manage, page);
            if (dialog.exec() == QDialog::Accepted)
                update();
        });

    }

    void MarketPage::initChartPlaceholder(QFrame* page)
    {
        page->setMinimumWidth(200);
        QVBoxLayout* layout = new QVBoxLayout(page);
        QLabel* title = new QLabel("Chart", page);
        layout->addWidget(title);
        QLabel* placeholder = new QLabel("Chart coming soon", page);
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setForegroundRole(QPalette::PlaceholderText);
        layout->addWidget(placeholder, 1);
    }

    void MarketPage::updateColumnVisibility()
    {
        const trtdata::MarketColumnVisibility& columns = ntrt::configManager->getAppearanceManager()->getMarketColumns();
        table->setColumnHidden(0, !columns.exchange);
        table->setColumnHidden(1, !columns.type);
        table->setColumnHidden(2, !columns.icon);
        table->setColumnHidden(5, !columns.volume24h);

        const QHeaderView* header = table->horizontalHeader();
        int minimumWidth = 120 + table->verticalScrollBar()->sizeHint().width();
        for (int column = 0; column < header->count(); column++)
        {
            if (!header->isSectionHidden(column) && column != 3)
                minimumWidth += header->sectionSize(column);
        }
        table->setMinimumWidth(minimumWidth + 2 * table->frameWidth());
    }

    void MarketPage::update()
    {
        updateColumnVisibility();
        const std::span<const std::shared_ptr<trtdata::Instrument>> instruments = ntrt::configManager->getInstruments();
        const trtcore::MarketData& marketData = ntrt::dataMonitor->getMarketData();
        const trtcore::CandlesManager& candles = ntrt::dataMonitor->getCandlesManager();
        const QString timeZone = QString::fromUtf8(trtdata::tradingDataTimeZoneName(candles.getTimeZone()));
        table->horizontalHeaderItem(5)->setToolTip("Today's daily-candle volume; day starts at 00:00 " + timeZone + ".");
        table->horizontalHeaderItem(6)->setToolTip("(Latest price - daily open) / daily open; day starts at 00:00 " + timeZone + ".");
        const QString query = search->text().trimmed();
        const int filter = typeFilter->currentData().toInt();
        const QLocale locale(QLocale::English);
        const bool flashEnabled = ntrt::configManager->getAppearanceManager()->isPriceFlashEnabled();
        int row = 0;
        for (const std::shared_ptr<trtdata::Instrument>& instrument : instruments)
        {
            const QString exchange = QString::fromUtf8(trtdata::getExchangeStr(instrument->exchange));
            const QString symbol = QString::fromStdString(instrument->instId);
            if ((filter >= 0 && filter != static_cast<int>(instrument->instType)) ||
                (!symbol.contains(query, Qt::CaseInsensitive) && !exchange.contains(query, Qt::CaseInsensitive)))
                continue;

            if (row == table->rowCount())
                table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(exchange));
            table->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(trttrade::instDisplayStr(instrument->instType))));
            QLabel* icon = static_cast<QLabel*>(table->cellWidget(row, 2));
            if (!icon)
            {
                icon = new QLabel(table);
                icon->setObjectName("instrumentIcon");
                icon->setAlignment(Qt::AlignCenter);
                table->setCellWidget(row, 2, icon);
            }
            icon->setPixmap(ntrt::dataMonitor->getInstrumentIconManager().getIcon(symbol).pixmap(QSize(20, 20)));
            icon->setToolTip(symbol);
            table->setItem(row, 3, new QTableWidgetItem(symbol));

            const trtdata::InstIdentifier& identifier = instrument->getUniqueIdentifier();
            const trtcore::MarketSnapshot* snapshot = marketData.getSnapshot(identifier);
            QTableWidgetItem* priceItem = new QTableWidgetItem("No data");
            if (snapshot && snapshot->ticker)
            {
                const trtdata::TickerData& data = *snapshot->ticker;
                priceItem->setText(data.last >= 1 ? locale.toString(data.last, 'f', 2) : locale.toString(data.last, 'g', 4));
                PriceDelegate::setMovement(priceItem, *snapshot, table->palette(), flashEnabled);
            }
            if (snapshot && !snapshot->error.isEmpty())
                priceItem->setToolTip("Market feed: " + snapshot->error);
            table->setItem(row, 4, priceItem);

            const trttrade::CandleData* daily = candles.getDailyCandle(identifier);
            const trtcore::CandleSnapshot* candleSnapshot = candles.getSnapshot(identifier);
            QTableWidgetItem* volume = new QTableWidgetItem("-");
            QTableWidgetItem* change = new QTableWidgetItem("-");
            change->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (daily)
            {
                double baseVolume = 0;
                switch (instrument->instType)
                {
                case trttrade::InstType::Spot:
                case trttrade::InstType::Margin:
                    baseVolume = daily->volume;
                    break;
                case trttrade::InstType::Swap:
                case trttrade::InstType::Futures:
                case trttrade::InstType::Option:
                case trttrade::InstType::Events:
                    baseVolume = daily->volumeCcy;
                    break;
                }
                volume->setText(locale.toString(baseVolume, 'f', 2) + " " + symbol.section('-', 0, 0));
                volume->setToolTip(locale.toString(daily->volumeQuote, 'f', 2) + " " + symbol.section('-', 1, 1));
                const std::optional<double> dailyChange = snapshot && snapshot->ticker ?
                    candles.getDailyChangePercent(identifier, snapshot->ticker->last) : std::nullopt;
                if (dailyChange)
                {
                    const double percent = *dailyChange;
                    const QString text = QString::number(percent, 'f', 2);
                    if (text == "0.00" || text == "-0.00")
                        change->setText("0.00%");
                    else
                    {
                        change->setText((percent > 0 ? "+" : "") + text + "%");
                        const bool dark = table->palette().color(QPalette::Base).lightness() < 128;
                        change->setForeground(percent > 0 ? QColor(dark ? 0x4ade80 : 0x15803d) : QColor(dark ? 0xfb7185 : 0xb91c1c));
                    }
                }
            }
            if (candleSnapshot && !candleSnapshot->error.isEmpty())
            {
                const QString error = "Daily candles: " + candleSnapshot->error;
                volume->setToolTip(volume->toolTip() + "\n" + error);
                change->setToolTip(error);
            }
            table->setItem(row, 5, volume);
            table->setItem(row, 6, change);
            row++;
        }
        table->setRowCount(row);
    }
} // trtui
