#include "AccountPanel.h"
#include "main/NTRT.h"
#include "main/ui/widgets/NumericWidgets.h"
#include "main/util/TradeUtil.h"
#include "main/util/PaletteUtil.h"
#include "main/util/QtUtil.h"
#include <algorithm>
#include <QVBoxLayout>
#include <functional>
#include <QHeaderView>
#include <QTimer>
#include <QScrollBar>
#include <QUuid>
#include <QDateTime>
#include <QCalendarWidget>
#include <QIcon>
#include <QGraphicsColorizeEffect>
#include <QGraphicsOpacityEffect>
#include <QMenu>
#include <QPushButton>
#include <QMouseEvent>
#include <QTableView>
#include <QSpinBox>
#include <QLocale>
#include <QMdiSubWindow>
#include <QSignalBlocker>
#include <QTextCharFormat>
#include <QToolButton>
#include <QTimeZone>
#include <QHBoxLayout>
namespace trtui
{
    AccountPanel::AccountPanel(trtdata::WorkspaceWindowType type, QWidget* parent) : QFrame(parent), type(type) { initUi(); }
    trtdata::WorkspaceWindowType AccountPanel::getType() const { return type; }

    void AccountPanel::initUi()
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        status = new QLabel(this);
        status->hide();
        status->setWordWrap(true);
        layout->addWidget(status);
        switch (type)
        {
            case trtdata::WorkspaceWindowType::AccountOrderHistory:
            case trtdata::WorkspaceWindowType::AccountPositionHistory: initHistoryUi(layout); break;
            default: break;
        }
        table = new QTableWidget(this);
        QStringList headers;
        switch (type)
        {
            case trtdata::WorkspaceWindowType::AccountBalance: headers = {"Currency", "Equity (USD)", "Equity", "Available", "Unrealized P&L", "Frozen"}; break;
            case trtdata::WorkspaceWindowType::AccountPositions: headers = {"Instrument", "Side", "Leverage", "Margin", "Position", "P&L", "Return", "Entry price", "Mark price", "Liq. price", "Break-even", "Margin mode", "Currency", "Available"}; break;
            case trtdata::WorkspaceWindowType::AccountOrderHistory: headers = {"Instrument", "Type", "Side", "Price", "Size", "Filled", "Average price", "State", "P&L", "Fee", "Updated (UTC)", "Order ID"}; break;
            case trtdata::WorkspaceWindowType::AccountPositionHistory: headers = {"Instrument", "Side", "Realized P&L", "Return", "Open price", "Close price", "Max position", "Closed", "Fee", "Funding fee", "Currency", "Updated (UTC)", "Position ID"}; break;
            case trtdata::WorkspaceWindowType::AccountOrders: headers = {"Instrument", "Type", "Side", "Price", "Size", "Filled", "Average price", "State", "Order ID"}; break;
            default: break;
        }
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        table->setItemDelegate(new ForegroundItemDelegate(table));
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setAlternatingRowColors(true);
        table->verticalHeader()->hide();
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        table->viewport()->installEventFilter(this);
        table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
        layout->addWidget(table, 1);
        setMinimumWidth(440);
        updateColumnWidths();
    }

    AccountPanel::~AccountPanel()
    {
        if (!historyId.isEmpty()) ntrt::dataMonitor->releaseAccountHistory(historyId);
    }

    void AccountPanel::initHistoryUi(QVBoxLayout* layout)
    {
        historyId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QHBoxLayout* filters = new QHBoxLayout;
        instrumentType = new QComboBox(this);
        instrumentType->addItem("Perpetual", static_cast<int>(trttrade::InstType::Swap));
        instrumentType->addItem("Futures", static_cast<int>(trttrade::InstType::Futures));
        instrumentType->addItem("Margin", static_cast<int>(trttrade::InstType::Margin));
        instrumentType->addItem("Options", static_cast<int>(trttrade::InstType::Option));
        if (type == trtdata::WorkspaceWindowType::AccountOrderHistory) instrumentType->addItem("Spot", static_cast<int>(trttrade::InstType::Spot));
        filters->addWidget(instrumentType);
        instrument = new QLineEdit(this);
        instrument->setPlaceholderText("Instrument (optional)");
        filters->addWidget(instrument, 1);
        historySource = new QComboBox(this);
        historySource->addItem("Archive (3 months)", true);
        historySource->addItem("Recent (7 days, includes unfilled cancellations)", false);
        historySource->setVisible(type == trtdata::WorkspaceWindowType::AccountOrderHistory);
        historySource->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        historySource->setMinimumContentsLength(18);
        filters->addWidget(historySource);
        QPushButton* refresh = new QPushButton("Refresh", this);
        filters->addWidget(refresh);
        connect(refresh, &QPushButton::clicked, this, [this] { queryHistory(true); });
        layout->addLayout(filters);
        initHistoryDates(layout);

        historyInputDelay.setSingleShot(true);
        historyInputDelay.setInterval(350);
        connect(&historyInputDelay, &QTimer::timeout, this, [this] { queryHistory(false); });
        connect(instrumentType, &QComboBox::currentIndexChanged, this, &AccountPanel::scheduleHistoryQuery);
        connect(instrument, &QLineEdit::textChanged, this, &AccountPanel::scheduleHistoryQuery);
        connect(historySource, &QComboBox::currentIndexChanged, this, [this]
        {
            const QDate today = QDateTime::currentDateTimeUtc().date();
            const QDate minimum = historySource->currentData().toBool() ? today.addMonths(-3) : today.addDays(-7);
            beginDate->setMinimumDate(minimum);
            endDate->setMinimumDate(minimum);
            scheduleHistoryQuery();
        });
        QTimer::singleShot(0, this, [this] { queryHistory(false); });
    }

    void AccountPanel::initHistoryDates(QVBoxLayout* layout)
    {
        QHBoxLayout* dates = new QHBoxLayout;
        const QDate today = QDateTime::currentDateTimeUtc().date();
        beginDate = new QDateEdit(today.addDays(-7), this);
        endDate = new QDateEdit(today, this);
        QLabel* from = new QLabel("From", this);
        QLabel* to = new QLabel("To", this);
        from->setBuddy(beginDate);
        to->setBuddy(endDate);
        dates->addWidget(from);
        dates->addWidget(beginDate);
        dates->addWidget(to);
        dates->addWidget(endDate);
        dates->addWidget(new QLabel(historySource->isHidden() ? "Updated date · UTC" : "Created date · UTC", this));
        dates->addStretch();

        for (QDateEdit* control : {beginDate, endDate})
        {
            control->setObjectName("historyDate");
            initHistoryHover(control);
            control->setLocale(QLocale::English);
            control->setDisplayFormat("dd MMM yyyy");
            control->setKeyboardTracking(false);
            control->setDateRange(today.addMonths(-3), today);
            control->setCalendarPopup(true);
            QCalendarWidget* calendar = control->calendarWidget();
            calendar->window()->installEventFilter(this);
            calendar->setObjectName("historyCalendar");
            calendar->setLocale(QLocale::English);
            calendar->setFirstDayOfWeek(Qt::Monday);
            calendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
            calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
            calendar->setGridVisible(false);
            calendar->setMinimumSize(260, 180);
            calendar->setWeekdayTextFormat(Qt::Saturday, QTextCharFormat());
            calendar->setWeekdayTextFormat(Qt::Sunday, QTextCharFormat());
            QToolButton* month = calendar->findChild<QToolButton*>("qt_calendar_monthbutton");
            QToolButton* year = calendar->findChild<QToolButton*>("qt_calendar_yearbutton");
            if (month && month->menu())
            {
                QMenu* menu = month->menu();
                menu->setObjectName("historyMonthMenu");
                menu->installEventFilter(this);
                const std::function<void()> updateMonths = [calendar, menu]
                {
                    for (QAction* action : menu->actions())
                    {
                        const QDate first(calendar->yearShown(), action->data().toInt(), 1);
                        action->setVisible(first.isValid() && first <= calendar->maximumDate() &&
                            first.addMonths(1) > calendar->minimumDate());
                    }
                };
                connect(calendar, &QCalendarWidget::currentPageChanged, menu, updateMonths);
                connect(menu, &QMenu::aboutToShow, menu, updateMonths);
                updateMonths();
            }
            if (month) initHistoryHover(month);
            if (year) initHistoryHover(year);
            QSpinBox* yearEdit = calendar->findChild<QSpinBox*>("qt_calendar_yearedit");
            if (yearEdit) initHistoryHover(yearEdit);
            QTableView* days = calendar->findChild<QTableView*>();
            if (days) initHistoryDayHover(days);
            connect(calendar, &QCalendarWidget::clicked, control, &QDateEdit::setDate);
            connect(calendar, &QCalendarWidget::activated, control, &QDateEdit::setDate);
            QToolButton* previous = calendar->findChild<QToolButton*>("qt_calendar_prevmonth");
            QToolButton* next = calendar->findChild<QToolButton*>("qt_calendar_nextmonth");
            if (previous && next)
            {
                previous->setIcon(QIcon());
                next->setIcon(QIcon());
                previous->setToolButtonStyle(Qt::ToolButtonTextOnly);
                next->setToolButtonStyle(Qt::ToolButtonTextOnly);
                previous->setText("‹");
                next->setText("›");
                initHistoryHover(previous);
                initHistoryHover(next);
            }
        }
        connect(beginDate, &QDateEdit::dateChanged, this, [this](const QDate& date)
        {
            if (date > endDate->date())
            {
                const QSignalBlocker blocker(endDate);
                endDate->setDate(date);
            }
            scheduleHistoryQuery();
        });
        connect(endDate, &QDateEdit::dateChanged, this, [this](const QDate& date)
        {
            if (date < beginDate->date())
            {
                const QSignalBlocker blocker(beginDate);
                beginDate->setDate(date);
            }
            scheduleHistoryQuery();
        });
        layout->addLayout(dates);
    }

    void AccountPanel::scheduleHistoryQuery()
    {
        historyInputDelay.start();
    }

    void AccountPanel::initHistoryHover(QWidget* control)
    {
        QGraphicsColorizeEffect* tint = new QGraphicsColorizeEffect(control);
        tint->setStrength(0.0);
        control->setGraphicsEffect(tint);
        QVariantAnimation* animation = new QVariantAnimation(control);
        animation->setDuration(140);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        connect(animation, &QVariantAnimation::valueChanged, tint, [control, tint](const QVariant& value)
        {
            tint->setColor(control->palette().color(QPalette::Highlight));
            tint->setStrength(value.toReal());
        });
        historyHover.insert(control, animation);
        control->installEventFilter(this);
    }

    void AccountPanel::initHistoryDayHover(QTableView* days)
    {
        QWidget* viewport = days->viewport();
        viewport->setMouseTracking(true);
        QFrame* highlight = new QFrame(viewport);
        highlight->setObjectName("historyDayHover");
        highlight->setAttribute(Qt::WA_TransparentForMouseEvents);
        highlight->hide();
        QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect(highlight);
        opacity->setOpacity(0.0);
        highlight->setGraphicsEffect(opacity);
        QPropertyAnimation* animation = new QPropertyAnimation(opacity, "opacity", highlight);
        animation->setDuration(140);
        historyDayHovers.insert(viewport, {days, highlight, animation});
        viewport->installEventFilter(this);
    }

    void AccountPanel::updateHistoryDayHover(const DayHover& hover, const QRect& cell)
    {
        QGraphicsOpacityEffect* opacity = static_cast<QGraphicsOpacityEffect*>(hover.highlight->graphicsEffect());
        if (cell.isValid())
        {
            if (!hover.highlight->isHidden() && hover.highlight->geometry() == cell && hover.animation->endValue().toReal() > 0.0)
                return;
            if (hover.highlight->geometry() != cell) opacity->setOpacity(0.0);
            hover.highlight->setGeometry(cell);
            hover.highlight->show();
            hover.highlight->raise();
        }
        hover.animation->stop();
        hover.animation->setStartValue(opacity->opacity());
        hover.animation->setEndValue(cell.isValid() ? 0.32 : 0.0);
        hover.animation->start();
    }

    void AccountPanel::queryHistory(bool refresh)
    {
        historyInputDelay.stop();
        const trttrade::AccountHistoryQuery query{
            .positions = type == trtdata::WorkspaceWindowType::AccountPositionHistory,
            .instType = static_cast<trttrade::InstType>(instrumentType->currentData().toInt()),
            .instId = instrument->text().trimmed().toUpper().toStdString(),
            .begin = beginDate->date().startOfDay(QTimeZone::UTC).toMSecsSinceEpoch(),
            .end = endDate->date().endOfDay(QTimeZone::UTC).toMSecsSinceEpoch(),
            .archive = historySource->currentData().toBool()
        };
        ntrt::dataMonitor->queryAccountHistory(historyId, query, refresh);
    }

    void AccountPanel::displayRows(const std::vector<QStringList>& rows)
    {
        table->setRowCount(static_cast<int>(rows.size()));
        for (int row = 0; row < table->rowCount(); row++)
            for (int column = 0; column < table->columnCount(); column++)
            {
                QTableWidgetItem* item = table->item(row, column);
                if (!item) { item = new QTableWidgetItem; table->setItem(row, column, item); }
                const QString value = rows[row][column].isEmpty() ? QString("—") : rows[row][column];
                item->setText(value);
                item->setTextAlignment(column == 0 ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignRight | Qt::AlignVCenter);
            }
        updateColumnWidths();
        updateColors();
    }

    QSize AccountPanel::sizeHint() const
    {
        switch (type)
        {
            case trtdata::WorkspaceWindowType::AccountBalance: return QSize(850, 340);
            case trtdata::WorkspaceWindowType::AccountOrderHistory:
            case trtdata::WorkspaceWindowType::AccountPositionHistory:
            case trtdata::WorkspaceWindowType::AccountPositions: return QSize(1180, 360);
            case trtdata::WorkspaceWindowType::AccountOrders: return QSize(1050, 360);
            default: return QSize(850, 340);
        }
    }

    void AccountPanel::updateColumnWidths()
    {
        std::vector<int> widths;
        int total = 0;
        const QFontMetrics metrics(table->font());
        for (int column = 0; column < table->columnCount(); column++)
        {
            int width = metrics.horizontalAdvance(table->horizontalHeaderItem(column)->text()) + 24;
            for (int row = 0; row < std::min(50, table->rowCount()); row++)
                if (table->item(row, column)) width = std::max(width, metrics.horizontalAdvance(table->item(row, column)->text()) + 24);
            width = std::clamp(width, 65, column == 0 ? 300 : 240);
            widths.push_back(width);
            total += width;
        }
        table->setMinimumHeight(table->horizontalHeader()->sizeHint().height() + 2 * table->frameWidth()
            + (table->rowCount() > 0 ? table->verticalHeader()->defaultSectionSize() : 0)
            + (total > table->viewport()->width() ? table->horizontalScrollBar()->sizeHint().height() : 0));
        const int extra = std::max(0, table->viewport()->width() - total);
        for (int column = 0; column < table->columnCount(); column++)
            table->setColumnWidth(column, widths[column] + extra / table->columnCount() + (column == 0 ? extra % table->columnCount() : 0));
    }

    void AccountPanel::updateColors()
    {
        for (int column = 0; column < table->columnCount(); column++)
        {
            const QString header = table->horizontalHeaderItem(column)->text();
            const bool profit = header.contains("P&L") || header == "Return";
            for (int row = 0; row < table->rowCount(); row++)
            {
                QTableWidgetItem* item = table->item(row, column);
                if (!item) continue;
                QColor color = table->palette().color(QPalette::Text);
                const QString direction = item->text().toLower();
                if (header == "Side")
                {
                    if (direction == "buy" || direction == "long") color = trtutils::risingColor(table->palette());
                    if (direction == "sell" || direction == "short") color = trtutils::fallingColor(table->palette());
                }
                if (profit)
                {
                    bool valid = false;
                    const double amount = item->text().split(' ').first().remove('%').toDouble(&valid);
                    if (valid && amount > 0) color = trtutils::risingColor(table->palette());
                    if (valid && amount < 0) color = trtutils::fallingColor(table->palette());
                }
                item->setForeground(color);
            }
        }
    }

    bool AccountPanel::eventFilter(QObject* watched, QEvent* event)
    {
        if (table && watched == table->viewport() && event->type() == QEvent::Resize) updateColumnWidths();
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave || event->type() == QEvent::EnabledChange)
        {
            QWidget* control = qobject_cast<QWidget*>(watched);
            QVariantAnimation* animation = historyHover.value(control, nullptr);
            if (animation)
            {
                QGraphicsColorizeEffect* tint = static_cast<QGraphicsColorizeEffect*>(control->graphicsEffect());
                animation->stop();
                animation->setStartValue(tint->strength());
                const bool hovered = event->type() == QEvent::Enter || (event->type() == QEvent::EnabledChange && control->underMouse());
                animation->setEndValue(hovered ? 0.55 : 0.0);
                if (control->isEnabled()) animation->start();
                else tint->setStrength(0.0);
            }
        }
        const QHash<QWidget*, DayHover>::const_iterator hover = historyDayHovers.constFind(qobject_cast<QWidget*>(watched));
        if (hover != historyDayHovers.cend() && (event->type() == QEvent::MouseMove || event->type() == QEvent::Leave || event->type() == QEvent::Hide))
        {
            QRect cell;
            if (event->type() == QEvent::MouseMove)
            {
                const QModelIndex index = hover->days->indexAt(static_cast<QMouseEvent*>(event)->position().toPoint());
                if (index.row() > 0 && index.flags().testFlag(Qt::ItemIsEnabled)) cell = hover->days->visualRect(index);
            }
            updateHistoryDayHover(*hover, cell);
        }
        if (event->type() == QEvent::Show && beginDate && endDate &&
            (watched->objectName() == "historyMonthMenu" || watched == beginDate->calendarWidget()->window() || watched == endDate->calendarWidget()->window()))
        {
            QWidget* popup = qobject_cast<QWidget*>(watched);
            QWidget* container = parentWidget();
            while (container && !qobject_cast<QMdiSubWindow*>(container)) container = container->parentWidget();
            QWidget* viewport = container ? container->parentWidget() : window();
            QRect bounds(viewport->mapToGlobal(QPoint()), viewport->size());
            if (container)
            {
                const QRect subwindow(container->mapToGlobal(QPoint()), container->size());
                const QRect visible = bounds.intersected(subwindow);
                if (visible.width() >= popup->width() && visible.height() >= popup->height()) bounds = visible;
            }
            popup->move(trtutils::boundedWidgetGeometry(popup->geometry(), bounds).topLeft());
        }
        return QFrame::eventFilter(watched, event);
    }

    void AccountPanel::changeEvent(QEvent* event)
    {
        QFrame::changeEvent(event);
        if (!table) return;
        if (event->type() == QEvent::PaletteChange)
            QTimer::singleShot(0, this, [this] { updateColors(); });
        if (event->type() == QEvent::FontChange) updateColumnWidths();
    }

    void AccountPanel::updateData()
    {
        if (!isVisible()) return;
        const std::shared_ptr<const trtcore::DataSnapshot> snapshot = ntrt::dataMonitor->getSnapshot();
        qint64 timestamp = 0;
        QString error;
        std::vector<QStringList> rows;
        const trtcore::AccountSnapshot& account = *snapshot->account;
        const trtcore::AccountHistorySnapshot emptyHistory;
        const std::map<QString, trtcore::AccountHistorySnapshot>::const_iterator found = account.histories.find(historyId);
        const trtcore::AccountHistorySnapshot& history = found == account.histories.end() ? emptyHistory : found->second;
        const std::function<QString(const std::string&)> text = [](const std::string& value) { return QString::fromStdString(value); };
        const std::function<QString(const std::string&)> number = [](const std::string& value) { return trttrade::accountNumber(value); };
        const std::function<QString(const std::string&)> calculatedPrice = [](const std::string& value) { return trttrade::accountNumber(value, false, 2); };
        const std::function<QString(const std::string&)> amount = [](const std::string& value) { return trttrade::accountNumber(value, false, 4); };
        switch (type)
        {
            case trtdata::WorkspaceWindowType::AccountBalance:
                timestamp = account.balance.timestamp; error = account.balance.error;
                if (timestamp != displayedTimestamp)
                    for (const trttrade::BalanceDetail& row : account.balance.data.details)
                        rows.push_back({text(row.ccy), amount(row.eqUsd), amount(row.eq), amount(row.availBal), trttrade::accountNumber(row.upl, true, 4), amount(row.frozenBal)});
                break;
            case trtdata::WorkspaceWindowType::AccountPositions:
                timestamp = account.positions.timestamp; error = account.positions.error;
                if (timestamp != displayedTimestamp)
                    for (const trttrade::PositionData& row : account.positions.data)
                        rows.push_back({text(row.instId), trttrade::accountPositionSide(row), number(row.lever) + "x", amount(row.margin) + " " + text(row.ccy), number(row.pos) + (row.instType == "MARGIN" ? " " + text(row.posCcy) : " contracts"), trttrade::accountNumber(row.upl, true, 4) + " " + text(row.ccy), trttrade::accountPercent(row.uplRatio), calculatedPrice(row.avgPx), number(row.markPx), calculatedPrice(row.liqPx), calculatedPrice(row.bePx), text(row.mgnMode), text(row.ccy), number(row.availPos)});
                break;
            case trtdata::WorkspaceWindowType::AccountOrderHistory:
                timestamp = history.revision; error = history.error;
                if (timestamp != displayedTimestamp)
                    for (const trttrade::OrderData& row : history.orders)
                        rows.push_back({text(row.instId), text(row.ordType), text(row.side), number(row.px), number(row.sz), number(row.accFillSz), calculatedPrice(row.avgPx), text(row.state), trttrade::accountNumber(row.pnl, true, 4), amount(row.fee) + " " + text(row.feeCcy), trttrade::accountTime(row.uTime), text(row.ordId)});
                break;
            case trtdata::WorkspaceWindowType::AccountPositionHistory:
                timestamp = history.revision; error = history.error;
                if (timestamp != displayedTimestamp)
                    for (const trttrade::PositionHistoryData& row : history.positions)
                        rows.push_back({text(row.instId), trttrade::accountHistorySide(row), trttrade::accountNumber(row.realizedPnl, true, 4), trttrade::accountPercent(row.pnlRatio), calculatedPrice(row.openAvgPx), calculatedPrice(row.closeAvgPx), number(row.openMaxPos), number(row.closeTotalPos), amount(row.fee), amount(row.fundingFee), text(row.ccy), trttrade::accountTime(row.uTime), text(row.posId)});
                break;
            case trtdata::WorkspaceWindowType::AccountOrders:
                timestamp = account.orders.timestamp; error = account.orders.error;
                if (timestamp != displayedTimestamp)
                    for (const trttrade::OrderData& row : account.orders.data)
                        rows.push_back({text(row.instId), text(row.ordType), text(row.side), number(row.px), number(row.sz), number(row.accFillSz), calculatedPrice(row.avgPx), text(row.state), text(row.ordId)});
                break;
            default: return;
        }
        if (timestamp != displayedTimestamp) { displayRows(rows); displayedTimestamp = timestamp; }
        status->setText(error);
        status->setVisible(!error.isEmpty());
    }
}
