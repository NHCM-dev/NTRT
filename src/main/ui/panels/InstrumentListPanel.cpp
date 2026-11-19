//
// Created by NHCM on 2026/11/7.
//

#include "InstrumentListPanel.h"
#include "main/util/CandleUtil.h"
#include "main/util/TimeUtil.h"
#include "main/util/TradeUtil.h"
#include "../dialog/InstrumentDialog.h"
#include <QTableWidget>
#include "main/NTRT.h"
#include <QHeaderView>
#include <QLocale>
#include <QPainter>
#include <QStyledItemDelegate>
#include "main/util/PaintUtil.h"
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
#include <QMetaType>
#include <QMouseEvent>
#include <QCursor>
#include <QTimer>
#include <QDateTime>

#include "main/util/QtUtil.h"
#include "main/util/NumberUtil.h"
#include "main/util/PaletteUtil.h"

namespace trtui
{
    class InstrumentDelegate : public QStyledItemDelegate
    {
    protected:
        void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
        {
            QStyledItemDelegate::initStyleOption(option, index);
            const bool selected = option->state.testFlag(QStyle::State_Selected);
            const bool hovered = option->widget && option->widget->property("hoveredRow").toInt() == index.row();
            // Apply the same row feedback to every cell, including the custom price renderer.
            if (selected)
                option->backgroundBrush = option->palette.brush(QPalette::Midlight);
            else if (hovered)
                option->backgroundBrush = option->palette.brush(QPalette::AlternateBase);
            option->state.setFlag(QStyle::State_Selected, false);
            option->state.setFlag(QStyle::State_MouseOver, false);
        }

    public:
        explicit InstrumentDelegate(QObject* parent) : QStyledItemDelegate(parent)
        {}
    };

    class PriceDelegate final : public InstrumentDelegate
    {
    public:
        explicit PriceDelegate(QObject* parent) : InstrumentDelegate(parent)
        {}

        static void setMovement(QTableWidgetItem* item, const trtcore::MarketSnapshot& snapshot,
                                const QPalette& palette, bool flashEnabled)
        {
            const trtdata::TickerData& ticker = *snapshot.ticker;
            const double difference = ticker.previousLast ? ticker.last - *ticker.previousLast : 0;
            const qint64 age = snapshot.priceChangeAge.isValid() ? snapshot.priceChangeAge.elapsed() : -1;
            const QString price = item->text();
            const bool stale = !snapshot.error.isEmpty() || QDateTime::currentMSecsSinceEpoch() - ticker.ts > 15000;
            const qsizetype start = trtutils::priceFlashOffset(price, difference, age, flashEnabled && !stale);
            const QColor color = difference > 0 ? trtutils::risingColor(palette) : trtutils::fallingColor(palette);
            if (start >= 0 && start < price.size())
            {
                item->setData(Qt::UserRole, start);
                item->setData(Qt::UserRole + 1, color.rgba());
            }
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            const QVariant start = index.data(Qt::UserRole);
            QStyleOptionViewItem styled(option);
            initStyleOption(&styled, index);
            QStyle* style = styled.widget ? styled.widget->style() : QApplication::style();
            const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &styled, styled.widget) + 1;
            const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &styled, styled.widget).adjusted(
                margin, 0, -margin, 0);
            const QString price = styled.fontMetrics.elidedText(styled.text, styled.textElideMode, textRect.width());
            if (price.isEmpty())
            {
                InstrumentDelegate::paint(painter, option, index);
                return;
            }
            styled.text.clear();
            style->drawControl(QStyle::CE_ItemViewItem, &styled, painter, styled.widget);

            trtutils::drawHighlightedText(*painter, textRect, price, styled.font, styled.palette.color(QPalette::Text),
                                          styled.displayAlignment, start.isValid() ? start.toInt() : -1, price.size(),
                                          QColor::fromRgba(index.data(Qt::UserRole + 1).toUInt()));
        }

    };

    QFrame* InstrumentListPanel::generateWidget(QWidget* parent)
    {
        QFrame* page = new QFrame(parent);

        page->setBackgroundRole(QPalette::Base);
        page->setAutoFillBackground(true);

        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setContentsMargins(8, 8, 8, 8);
        initFilters(page, layout);
        initTable(page, layout);
        initActions(page, layout);

        return page;
    }

    void InstrumentListPanel::initFilters(QFrame* page, QVBoxLayout* layout)
    {
        QHBoxLayout* filters = new QHBoxLayout;
        search = new QLineEdit(page);
        search->setObjectName("instrumentSearch");
        search->setPlaceholderText("Search instruments");
        typeFilter = new QComboBox(page);
        typeFilter->addItem("All types", -1);
        for (trttrade::InstType type : {
                 trttrade::InstType::Spot,
                 trttrade::InstType::Margin,
                 trttrade::InstType::Swap,
                 trttrade::InstType::Futures,
                 trttrade::InstType::Option,
                 trttrade::InstType::Events
             })
            typeFilter->addItem(QString::fromUtf8(instDisplayStr(type)), static_cast<int>(type));
        filters->addWidget(search, 1);
        filters->addWidget(typeFilter);
        layout->addLayout(filters);

        QObject::connect(search, &QLineEdit::textChanged, page, [this]
        {
            update();
        });
        QObject::connect(typeFilter, &QComboBox::currentIndexChanged, page, [this]
        {
            update();
        });
    }

    void InstrumentListPanel::initTable(QFrame* page, QVBoxLayout* layout)
    {
        table = trtutils::createHeaderTable(page, {
                                                "Exchange",
                                                "Type",
                                                "Icon",
                                                "Instrument",
                                                "Last Price",
                                                "Volume",
                                                "Change",
                                                "Change %"
                                            });
        table->setObjectName("instrumentTable");
        table->setProperty("hoveredRow", -1);
        table->setItemDelegate(new InstrumentDelegate(table));
        table->setItemDelegateForColumn(4, new PriceDelegate(table));
        table->setMouseTracking(true);
        qApp->installEventFilter(this);
        connect(table->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]
        {
            updateHoveredRow();
        });

        QHeaderView* header = table->horizontalHeader();
        header->setSectionResizeMode(0, QHeaderView::Interactive);
        header->setSectionResizeMode(4, QHeaderView::Interactive);
        header->setSectionResizeMode(5, QHeaderView::Interactive);
        header->setSectionResizeMode(6, QHeaderView::Interactive);
        header->setSectionResizeMode(7, QHeaderView::Interactive);
        header->setSectionResizeMode(1, QHeaderView::Fixed);

        header->setMinimumSectionSize(32);
        header->setSectionResizeMode(2, QHeaderView::Fixed);
        header->setSectionResizeMode(3, QHeaderView::Stretch);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        table->setColumnWidth(0, 72);
        table->setColumnWidth(1, 100);
        table->setColumnWidth(4, 70);
        table->setColumnWidth(5, 130);
        table->setColumnWidth(6, 60);
        table->setColumnWidth(7, 70);
        table->setColumnWidth(2, 48);
        table->verticalHeader()->hide();
        updateColumnVisibility();
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(table, 1);
        QLabel* hint = new QLabel("Double-click an instrument to open its chart", page);
        hint->setObjectName("instrumentListHint");
        layout->addWidget(hint);
        connect(table, &QTableWidget::cellDoubleClicked, this, [this](int row, int)
        {
            const QTableWidgetItem* item = table->item(row, 3);
            if (!item)
                return;
            const trtdata::InstIdentifier identifier{
                static_cast<trtdata::Exchange>(item->data(Qt::UserRole).toInt()),
                static_cast<trttrade::InstType>(item->data(Qt::UserRole + 1).toInt()),
                item->text().toStdString()
            };
            emit instrumentActivated(identifier);
        });
    }

    void InstrumentListPanel::initActions(QFrame* page, QVBoxLayout* layout)
    {
        QHBoxLayout* actions = new QHBoxLayout;
        QPushButton* add = new QPushButton("Add Instrument", page);
        QPushButton* manage = new QPushButton("Manage Instrument", page);
        actions->addWidget(add);
        actions->addWidget(manage);
        actions->addStretch();
        layout->addLayout(actions);
        connect(add, &QPushButton::clicked, page, [this, page]
        {
            InstrumentDialog dialog(InstrumentDialog::Mode::Add, page);
            if (dialog.exec() == QDialog::Accepted)
                update();
        });
        connect(manage, &QPushButton::clicked, page, [this, page]
        {
            InstrumentDialog dialog(InstrumentDialog::Mode::Manage, page);
            if (dialog.exec() == QDialog::Accepted)
                update();
        });
    }

    bool InstrumentListPanel::eventFilter(QObject* watched, QEvent* event)
    {
        if (event->type() == QEvent::MouseButtonPress && table->isVisible() && table->currentRow() >= 0)
        {
            const QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
            const bool clickedRow = watched == table->viewport() && table->indexAt(mouse->position().toPoint()).isValid();
            if (!clickedRow)
                clearInstrumentSelection();
        }
        if (watched == table->viewport())
        {
            if (event->type() == QEvent::MouseMove)
                setHoveredRow(table->indexAt(static_cast<QMouseEvent*>(event)->position().toPoint()).row());
            else if (event->type() == QEvent::Leave)
                setHoveredRow(-1);
            else if (event->type() == QEvent::Enter)
                updateHoveredRow();
        }
        return QObject::eventFilter(watched, event);
    }

    void InstrumentListPanel::clearInstrumentSelection()
    {
        table->clearSelection();
        table->setCurrentItem(nullptr);
    }

    void InstrumentListPanel::updateHoveredRow()
    {
        const QPoint position = table->viewport()->mapFromGlobal(QCursor::pos());
        const bool inside = table->viewport()->underMouse() || table->property("hoveredRow").toInt() >= 0;
        setHoveredRow(inside && table->viewport()->rect().contains(position) ? table->indexAt(position).row() : -1);
    }

    void InstrumentListPanel::setHoveredRow(int row)
    {
        const int previous = table->property("hoveredRow").toInt();
        if (previous == row)
            return;
        table->setProperty("hoveredRow", row);
        for (int changed : {previous, row})
        {
            if (changed >= 0 && changed < table->rowCount())
                table->viewport()->update(QRect(0, table->rowViewportPosition(changed), table->viewport()->width(),
                                                table->rowHeight(changed)));
        }
    }

    void InstrumentListPanel::updateColumnVisibility()
    {
        const trtdata::MarketColumnVisibility& columns = ntrt::configManager->getAppearanceManager()->
                                                                              getMarketColumns();
        table->setColumnHidden(0, !columns.exchange);
        table->setColumnHidden(1, !columns.type);
        table->setColumnHidden(2, !columns.icon);
        table->setColumnHidden(5, !columns.volume24h);
        table->setColumnHidden(6, !columns.changeAmount);

        const QHeaderView* header = table->horizontalHeader();
        int minimumWidth = 120 + table->verticalScrollBar()->sizeHint().width();
        for (int column = 0; column < header->count(); column++)
        {
            if (!header->isSectionHidden(column) && column != 3)
                minimumWidth += header->sectionSize(column);
        }
        table->setMinimumWidth(minimumWidth + 2 * table->frameWidth());
    }

    trtdata::InstrumentListState InstrumentListPanel::getState() const
    {
        trtdata::InstrumentListState state;
        state.search = search->text();
        state.typeFilter = typeFilter->currentData().toInt();
        state.header = table->horizontalHeader()->saveState();
        state.scroll = table->verticalScrollBar()->value();
        const QTableWidgetItem* item = table->item(table->currentRow(), 3);
        if (item)
            state.selected = trtdata::InstIdentifier{static_cast<trtdata::Exchange>(item->data(Qt::UserRole).toInt()),
                static_cast<trttrade::InstType>(item->data(Qt::UserRole + 1).toInt()), item->text().toStdString()};
        return state;
    }

    void InstrumentListPanel::restoreState(const trtdata::InstrumentListState& state)
    {
        search->setText(state.search);
        typeFilter->setCurrentIndex(typeFilter->findData(state.typeFilter));
        table->horizontalHeader()->restoreState(state.header);
        update();
        if (state.selected)
        {
            for (int row = 0; row < table->rowCount(); row++)
            {
                const QTableWidgetItem* item = table->item(row, 3);
                if (item && item->text().toStdString() == state.selected->id &&
                    item->data(Qt::UserRole).toInt() == static_cast<int>(state.selected->exchange) &&
                    item->data(Qt::UserRole + 1).toInt() == static_cast<int>(state.selected->type))
                {
                    table->setCurrentCell(row, 3);
                    break;
                }
            }
        }
        QTimer::singleShot(0, table, [this, state]
        {
            table->verticalScrollBar()->setValue(state.scroll);
        });
    }

    void InstrumentListPanel::update()
    {
        const QTableWidgetItem* selected = table->item(table->currentRow(), 3);
        const QString selectedSymbol = selected ? selected->text() : QString();
        const QVariant selectedExchange = selected ? selected->data(Qt::UserRole) : QVariant();
        const QVariant selectedType = selected ? selected->data(Qt::UserRole + 1) : QVariant();
        int selectedRow = -1;
        updateColumnVisibility();
        const std::span<const std::shared_ptr<trtdata::Instrument>> instruments = ntrt::configManager->getInstruments();
        const std::shared_ptr<const trtcore::DataSnapshot> data = ntrt::dataMonitor->getSnapshot();
        const trtcore::MarketDataSnapshot& marketData = data->marketData;
        const trtcore::CandlesSnapshot& candles = data->candles;
        const QString timeZone = QString::fromUtf8(trtdata::tradingDataTimeZoneName(candles.getTimeZone()));
        table->horizontalHeaderItem(6)->setToolTip("Latest price minus daily open; day starts at 00:00 " + timeZone + ".");
        table->horizontalHeaderItem(5)->
               setToolTip("Today's daily-candle volume; day starts at 00:00 " + timeZone + ".");
        table->horizontalHeaderItem(7)->setToolTip(
            "(Latest price - daily open) / daily open; day starts at 00:00 " + timeZone + ".");
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
            trtutils::updateTableItem(table, row, 0, exchange);
            trtutils::updateTableItem(table, row, 1, QString::fromUtf8(trttrade::instDisplayStr(instrument->instType)));
            QLabel* icon = static_cast<QLabel*>(table->cellWidget(row, 2));
            if (!icon)
            {
                icon = new QLabel(table);
                icon->setObjectName("instrumentIcon");
                icon->setAttribute(Qt::WA_TransparentForMouseEvents);
                icon->setAlignment(Qt::AlignCenter);
                table->setCellWidget(row, 2, icon);
            }
            const QIcon image = ntrt::dataMonitor->getInstrumentIconManager().getIcon(symbol);
            if (icon->property("iconCacheKey").toLongLong() != image.cacheKey())
            {
                icon->setPixmap(image.pixmap(QSize(20, 20)));
                icon->setProperty("iconCacheKey", image.cacheKey());
            }
            QTableWidgetItem* symbolItem = trtutils::updateTableItem(table, row, 3, symbol);
            symbolItem->setData(Qt::UserRole, static_cast<int>(instrument->exchange));
            symbolItem->setData(Qt::UserRole + 1, static_cast<int>(instrument->instType));
            if (symbol == selectedSymbol && symbolItem->data(Qt::UserRole) == selectedExchange && symbolItem->
                data(Qt::UserRole + 1) == selectedType)
                selectedRow = row;

            const trtdata::InstIdentifier& identifier = instrument->getUniqueIdentifier();
            const trtcore::MarketSnapshot* snapshot = marketData.getSnapshot(identifier);
            QTableWidgetItem* priceItem = trtutils::updateTableItem(table, row, 4, "No data");
            priceItem->setToolTip(QString());
            priceItem->setData(Qt::UserRole, QVariant());
            priceItem->setData(Qt::UserRole + 1, QVariant());
            if (snapshot && snapshot->ticker)
            {
                const trtdata::TickerData& data = *snapshot->ticker;
                priceItem->setText(trtutils::formatPrice(data.last));
                PriceDelegate::setMovement(priceItem, *snapshot, table->palette(), flashEnabled);
            }
            if (snapshot && !snapshot->error.isEmpty())
                priceItem->setToolTip("Market feed: " + snapshot->error);

            const trttrade::CandleData* daily = candles.getDailyCandle(identifier);
            const trtcore::CandleSnapshot* candleSnapshot = candles.getSnapshot(identifier);
            QTableWidgetItem* volume = trtutils::updateTableItem(table, row, 5, "-");
            volume->setToolTip(QString());
            QTableWidgetItem* change = trtutils::updateTableItem(table, row, 7, "-");
            change->setData(Qt::ForegroundRole, QVariant());
            change->setToolTip(QString());
            change->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            QTableWidgetItem* amount = trtutils::updateTableItem(table, row, 6, "-");
            amount->setData(Qt::ForegroundRole, QVariant());
            amount->setToolTip(QString());
            amount->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            const std::optional<double> dailyAmount = snapshot && snapshot->ticker
                ? trtutils::dailyChangeAmount(daily, snapshot->ticker->last) : std::nullopt;
            if (dailyAmount)
            {
                amount->setText(trtutils::formatPriceChange(*dailyAmount));
                if (*dailyAmount != 0)
                    amount->setForeground(*dailyAmount > 0 ? trtutils::risingColor(table->palette())
                                                          : trtutils::fallingColor(table->palette()));
            }
            if (daily)
            {
                const double baseVolume = trtutils::baseVolume(*daily, instrument->instType);
                volume->setText(locale.toString(baseVolume, 'f', 2) + " " + symbol.section('-', 0, 0));
                volume->setToolTip(locale.toString(daily->volumeQuote, 'f', 2) + " " + symbol.section('-', 1, 1));
                const std::optional<double> dailyChange = snapshot && snapshot->ticker
                                                              ? candles.getDailyChangePercent(
                                                                  identifier, snapshot->ticker->last)
                                                              : std::nullopt;
                if (dailyChange)
                {
                    const double percent = *dailyChange;
                    const QString text = QString::number(percent, 'f', 2);
                    if (text == "0.00" || text == "-0.00")
                        change->setText("0.00%");
                    else
                    {
                        change->setText((percent > 0 ? "+" : "") + text + "%");
                        change->setForeground(percent > 0
                                                  ? trtutils::risingColor(table->palette())
                                                  : trtutils::fallingColor(table->palette()));
                    }
                }
            }
            if (candleSnapshot && !candleSnapshot->error.isEmpty())
            {
                const QString error = "Daily candles: " + candleSnapshot->error;
                volume->setToolTip(volume->toolTip() + "\n" + error);
                change->setToolTip(error);
                amount->setToolTip(amount->toolTip() + "\n" + error);
            }
            row++;
        }
        table->setRowCount(row);
        if (selectedRow >= 0 && table->currentRow() != selectedRow)
            table->setCurrentCell(selectedRow, 3);
        else if (selectedRow < 0)
        {
            clearInstrumentSelection();
        }
        updateHoveredRow();
    }
} // trtui
