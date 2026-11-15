#include "BookDepthPanel.h"
#include "main/ui/widgets/NumericWidgets.h"
#include "main/NTRT.h"
#include "main/util/NumberUtil.h"
#include "main/util/PaletteUtil.h"
#include "main/util/TradeUtil.h"
#include <QScrollBar>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QHeaderView>
#include <QLabel>
#include <QMdiSubWindow>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QToolButton>
#include <algorithm>

namespace trtui
{
    BookDepthPanel::BookDepthPanel(QWidget* parent) : QFrame(parent), monitor(ntrt::dataMonitor)
    {
        setObjectName("bookDepthPanel");
        setMinimumSize(390, 360);
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(4);
        initControls(layout);
        initContent(layout);
        rebuildInstruments();
        applyControls();
        connect(ntrt::configManager.get(), &trtdata::ConfigManager::instrumentsChanged,
                this, &BookDepthPanel::rebuildInstruments);
    }

    BookDepthPanel::~BookDepthPanel()
    {
        releaseBook();
    }

    void BookDepthPanel::initControls(QVBoxLayout* layout)
    {
        instrumentBox = new QComboBox(this);
        instrumentBox->setObjectName("bookInstrument");
        instrumentBox->setEditable(true);
        instrumentBox->setInsertPolicy(QComboBox::NoInsert);
        instrumentBox->completer()->setFilterMode(Qt::MatchContains);
        instrumentBox->completer()->setCaseSensitivity(Qt::CaseInsensitive);
        QHBoxLayout* toolbar = new QHBoxLayout;
        toolbar->addWidget(instrumentBox, 1);
        settingsToggle = new QToolButton(this);
        settingsToggle->setObjectName("bookSettingsToggle");
        settingsToggle->setText("Settings");
        settingsToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        settingsToggle->setArrowType(Qt::DownArrow);
        settingsToggle->setCheckable(true);
        settingsToggle->setChecked(true);
        settingsToggle->setToolTip("Collapse or expand order book settings");
        toolbar->addWidget(settingsToggle);
        layout->addLayout(toolbar);
        settingsControls = new QWidget(this);
        settingsControls->setObjectName("bookSettingsControls");
        QVBoxLayout* settingsLayout = new QVBoxLayout(settingsControls);
        settingsLayout->setContentsMargins(0, 0, 0, 0);
        settingsLayout->setSpacing(4);
        layout->addWidget(settingsControls);
        connect(settingsToggle, &QToolButton::toggled, this, [this](bool expanded)
        {
            settingsControls->setVisible(expanded);
            settingsToggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
            state.settingsExpanded = expanded;
        });
        QHBoxLayout* aggregation = new QHBoxLayout;
        depthBox = new QSpinBox(this);
        depthBox->setObjectName("bookDepth");
        depthBox->setRange(1, 400);
        depthBox->setValue(state.depth);
        depthBox->setToolTip("Raw levels per side included before price aggregation (maximum 400)");
        aggregation->addWidget(new QLabel("Depth", this));
        aggregation->addWidget(depthBox);
        stepBox = new DecimalSpinBox(this);
        stepBox->setObjectName("bookPriceStep");
        stepBox->setDecimals(12);
        stepBox->setRange(0, 1e12);
        stepBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        stepBox->setMinimumWidth(110);
        stepBox->setSpecialValueText("Native");
        stepBox->setToolTip("Price bucket width; zero keeps native prices. Bids round down, asks round up.");
        aggregation->addWidget(new QLabel("Price step", this));
        aggregation->addWidget(stepBox, 1);
        settingsLayout->addLayout(aggregation);
        QHBoxLayout* display = new QHBoxLayout;
        rowsBox = new QSpinBox(this);
        rowsBox->setObjectName("bookRows");
        rowsBox->setRange(1, 100);
        rowsBox->setValue(state.rows);
        rowsBox->setToolTip("Displayed price buckets per side");
        display->addWidget(new QLabel("Rows", this));
        display->addWidget(rowsBox);
        layoutBox = new QComboBox(this);
        layoutBox->setObjectName("bookLayout");
        layoutBox->addItems({"Buy + Sell", "Buy only", "Sell only"});
        display->addWidget(layoutBox, 1);
        cumulativeBox = new QCheckBox("Total", this);
        cumulativeBox->setChecked(true);
        display->addWidget(cumulativeBox);
        settingsLayout->addLayout(display);
        alignmentBox = new QComboBox(settingsControls);
        alignmentBox->setObjectName("bookPriceAlignment");
        alignmentBox->addItems({"Price: Left", "Price: Center", "Price: Right"});
        settingsLayout->addWidget(alignmentBox);
        connect(alignmentBox, &QComboBox::currentIndexChanged, this, &BookDepthPanel::applyControls);
        connect(instrumentBox, &QComboBox::activated, this, [this](int index)
        {
            if (index > 0 && index <= static_cast<int>(choices.size()))
                setInstrument(choices[static_cast<std::size_t>(index - 1)]);
            else
                rebuildInstruments();
        });
        connect(depthBox, &QSpinBox::valueChanged, this, &BookDepthPanel::applyControls);
        connect(stepBox, &QDoubleSpinBox::valueChanged, this, &BookDepthPanel::applyControls);
        connect(rowsBox, &QSpinBox::valueChanged, this, &BookDepthPanel::applyControls);
        connect(layoutBox, &QComboBox::currentIndexChanged, this, &BookDepthPanel::applyControls);
        connect(cumulativeBox, &QCheckBox::toggled, this, &BookDepthPanel::applyControls);
    }

    void BookDepthPanel::initContent(QVBoxLayout* layout)
    {
        asks = new QTableWidget(this);
        bids = new QTableWidget(this);
        asks->setObjectName("bookAsks");
        bids->setObjectName("bookBids");
        for (QTableWidget* table : {asks, bids})
        {
            table->setItemDelegate(new ValueBarDelegate(table));
            table->setColumnCount(3);
            table->setHorizontalHeaderLabels({"Price", "Size", "Total"});
            table->horizontalHeaderItem(1)->setToolTip("Bucket size; bar length relative to the largest displayed bucket on this side.");
            table->horizontalHeaderItem(2)->setToolTip("Cumulative size from the best price; bar length relative to the sum of displayed buckets on this side.");
            table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            table->verticalHeader()->hide();
            table->verticalHeader()->setDefaultSectionSize(22);
            table->setEditTriggers(QAbstractItemView::NoEditTriggers);
            table->setSelectionMode(QAbstractItemView::NoSelection);
            table->setFocusPolicy(Qt::NoFocus);
            table->setShowGrid(false);
            table->setMinimumHeight(60);
            table->setToolTip("Size and cumulative total: base currency for spot, contracts for derivatives. Best prices next to latest price.");
        }
        layout->addWidget(asks, 1);
        latest = new PriceLabel(this);
        latest->setPriceText({}, "—", {});
        latest->setObjectName("bookLatest");
        latest->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        latest->setTextFormat(Qt::PlainText);
        layout->addWidget(latest);
        layout->addWidget(bids, 1);
        balance = new RatioBar("B", "S", this);
        balance->setObjectName("bookBalance");
        balance->setToolTip("Buy / sell share of total size in the selected raw depth per side, independent of visible rows or layout. Base currency for spot; contracts for derivatives.");
        layout->addWidget(balance);
        status = new QLabel("Select an instrument to begin", this);
        status->setObjectName("bookStatus");
        status->setWordWrap(true);
        status->setTextFormat(Qt::PlainText);
        layout->addWidget(status);
    }

    void BookDepthPanel::rebuildInstruments()
    {
        const QSignalBlocker blocker(instrumentBox);
        choices.clear();
        instrumentBox->clear();
        instrumentBox->addItem("Choose instrument…");
        for (const std::shared_ptr<trtdata::Instrument>& tracked : ntrt::configManager->getInstruments())
        {
            choices.push_back(tracked->getUniqueIdentifier());
            instrumentBox->addItem(trtutils::instrumentLabel(choices.back()));
        }
        if (!state.instrument)
            return;
        std::vector<trtdata::InstIdentifier>::const_iterator found = std::find(choices.cbegin(), choices.cend(), *state.instrument);
        if (found == choices.cend())
        {
            choices.push_back(*state.instrument);
            instrumentBox->addItem(trtutils::instrumentLabel(*state.instrument));
            instrumentBox->setCurrentIndex(static_cast<int>(choices.size()));
        } else
            instrumentBox->setCurrentIndex(static_cast<int>(found - choices.cbegin()) + 1);
    }

    void BookDepthPanel::releaseBook()
    {
        const std::shared_ptr<trtcore::DataMonitor> backend = monitor.lock();
        if (backend && state.instrument)
            backend->releaseBook(*state.instrument);
    }

    void BookDepthPanel::setInstrument(const trtdata::InstIdentifier& instrument)
    {
        const trtdata::InstIdentifier selected = instrument;
        if (state.instrument && *state.instrument == selected)
            return;
        releaseBook();
        state.instrument = selected;
        const std::shared_ptr<trtcore::DataMonitor> backend = monitor.lock();
        if (backend)
            backend->acquireBook(selected);
        rebuildInstruments();
        updateSide(asks, {}, false);
        updateSide(bids, {}, true);
        latest->setPriceText({}, "—", {});
        balance->setShare(std::nullopt);
        status->setText("Waiting for book snapshot…");
        status->show();
        emit titleChanged(trtutils::instrumentLabel(selected));
        updateData();
    }

    void BookDepthPanel::applyControls()
    {
        state.depth = depthBox->value();
        state.priceStep = stepBox->value();
        state.rows = rowsBox->value();
        state.layout = static_cast<trtdata::BookLayout>(layoutBox->currentIndex());
        state.cumulative = cumulativeBox->isChecked();
        state.priceAlignment = static_cast<trtdata::PriceAlignment>(alignmentBox->currentIndex());
        switch (state.priceAlignment)
        {
            case trtdata::PriceAlignment::Left: latest->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); break;
            case trtdata::PriceAlignment::Center: latest->setAlignment(Qt::AlignCenter); break;
            case trtdata::PriceAlignment::Right: latest->setAlignment(Qt::AlignRight | Qt::AlignVCenter); break;
        }
        switch (state.layout)
        {
            case trtdata::BookLayout::Both:
                asks->show();
                bids->show();
                break;
            case trtdata::BookLayout::BuyOnly:
                asks->hide();
                bids->show();
                break;
            case trtdata::BookLayout::SellOnly:
                asks->show();
                bids->hide();
                break;
        }
        asks->setColumnHidden(2, !state.cumulative);
        bids->setColumnHidden(2, !state.cumulative);
        updateData();
    }

    trtdata::BookDepthPanelState BookDepthPanel::getState() const
    {
        return state;
    }

    void BookDepthPanel::restoreState(const trtdata::BookDepthPanelState& value)
    {
        const QSignalBlocker depthBlock(depthBox), stepBlock(stepBox), rowsBlock(rowsBox);
        const QSignalBlocker alignmentBlock(alignmentBox);
        const QSignalBlocker layoutBlock(layoutBox), totalBlock(cumulativeBox);
        depthBox->setValue(value.depth);
        stepBox->setValue(value.priceStep);
        rowsBox->setValue(value.rows);
        layoutBox->setCurrentIndex(static_cast<int>(value.layout));
        cumulativeBox->setChecked(value.cumulative);
        alignmentBox->setCurrentIndex(static_cast<int>(value.priceAlignment));
        applyControls();
        settingsToggle->setChecked(value.settingsExpanded);
        if (value.instrument)
            setInstrument(*value.instrument);
    }

    void BookDepthPanel::updateSide(QTableWidget* table, const std::vector<trttrade::BookLevel>& levels, bool buy)
    {
        const bool changedRows = table->rowCount() != static_cast<int>(levels.size());
        if (changedRows)
        {
            table->setRowCount(static_cast<int>(levels.size()));
            for (int row = 0; row < table->rowCount(); row++)
            {
                for (int column = 0; column < 3; column++)
                {
                    if (!table->item(row, column))
                    {
                        QTableWidgetItem* item = new QTableWidgetItem;
                        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                        table->setItem(row, column, item);
                    }
                }
            }
        }
        if (changedRows && !buy)
            table->scrollToBottom();
        double maximum = 0;
        double maximumSize = 0;
        for (const trttrade::BookLevel& level : levels)
        {
            maximum += level.size;
            maximumSize = std::max(maximumSize, level.size);
        }
        double total = 0;
        const QColor color = buy ? trtutils::risingColor(palette()) : trtutils::fallingColor(palette());
        for (std::size_t index = 0; index < levels.size(); index++)
        {
            total += levels[index].size;
            const int row = buy ? static_cast<int>(index) : static_cast<int>(levels.size() - index - 1);
            const QString values[] = {trtutils::formatDecimal(levels[index].price), trtutils::formatVolume(levels[index].size),
                                      trtutils::formatVolume(total)};
            for (int column = 0; column < 3; column++)
            {
                QTableWidgetItem* item = table->item(row, column);
                if (item->text() != values[column])
                    item->setText(values[column]);
                item->setForeground(column == 0 ? color : palette().color(QPalette::Text));
                const double fraction = column == 1 && maximumSize > 0 ? levels[index].size / maximumSize
                                        : column == 2 && maximum > 0 ? total / maximum : 0;
                item->setData(ValueBarDelegate::FractionRole, fraction);
                item->setData(ValueBarDelegate::RisingRole, buy);
                item->setToolTip(column == 0 ? trtutils::formatDecimal(levels[index].price)
                                           : trtutils::formatQuantity(column == 1 ? levels[index].size : total));
            }
        }
    }

    void BookDepthPanel::updateData()
    {
        if (!isVisible() || window()->isMinimized())
            return;
        for (QWidget* ancestor = parentWidget(); ancestor; ancestor = ancestor->parentWidget())
        {
            QMdiSubWindow* child = qobject_cast<QMdiSubWindow*>(ancestor);
            if (child && child->isMinimized())
                return;
        }
        const std::shared_ptr<trtcore::DataMonitor> backend = monitor.lock();
        if (!backend || !state.instrument)
            return;
        const std::shared_ptr<const trtcore::MarketDataSnapshot> snapshot = backend->getMarketData();
        const trtcore::MarketSnapshot* ticker = snapshot->getSnapshot(*state.instrument);
        if (ticker && ticker->ticker)
        {
            const trtdata::TickerData& data = *ticker->ticker;
            const QString price = trtutils::formatPrice(data.last);
            const bool stale = !ticker->error.isEmpty() || QDateTime::currentMSecsSinceEpoch() - data.ts > 15000;
            const double movement = data.previousLast ? data.last - *data.previousLast : 0;
            const qint64 age = ticker->priceChangeAge.isValid() ? ticker->priceChangeAge.elapsed() : -1;
            latest->setPriceText({}, price, stale ? " · stale" : "", movement, age,
                                 !stale && ntrt::configManager->getAppearanceManager()->isPriceFlashEnabled());
        } else
            latest->setPriceText({}, "—", {});
        const trttrade::BookSnapshot* book = snapshot->getBook(*state.instrument);
        if (!book || book->sequence < 0)
        {
            updateSide(asks, {}, false);
            updateSide(bids, {}, true);
            balance->setShare(std::nullopt);
            status->show();
            status->setText(book && !book->error.isEmpty() ? book->error : "Waiting for book snapshot…");
            return;
        }
        balance->setShare(trtutils::bookBuyShare(book->bids, book->asks, state.depth));
        if (!asks->isHidden())
            updateSide(asks, trtutils::aggregateBook(book->asks, false, state.depth, state.priceStep, state.rows), false);
        if (!bids->isHidden())
            updateSide(bids, trtutils::aggregateBook(book->bids, true, state.depth, state.priceStep, state.rows), true);
        const qint64 age = std::max<qint64>(0, (QDateTime::currentMSecsSinceEpoch() - book->updatedAt) / 1000);
        QString text;
        if (age > 15)
            text = "Waiting for feed";
        if (!book->error.isEmpty())
            text = book->error;
        status->setText(text);
        status->setVisible(!text.isEmpty());
    }

    void BookDepthPanel::changeEvent(QEvent* event)
    {
        QFrame::changeEvent(event);
        if (event->type() == QEvent::PaletteChange && asks && bids)
            updateData();
    }

    void BookDepthPanel::showEvent(QShowEvent* event)
    {
        QFrame::showEvent(event);
        updateData();
    }
}
