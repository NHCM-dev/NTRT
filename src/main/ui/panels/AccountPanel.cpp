#include "AccountPanel.h"
#include "main/NTRT.h"
#include "main/ui/widgets/NumericWidgets.h"
#include "main/util/TradeUtil.h"
#include "main/util/PaletteUtil.h"
#include <algorithm>
#include <QVBoxLayout>
#include <functional>
#include <QHeaderView>
#include <QTimer>
#include <QScrollBar>
namespace trtui
{
    AccountPanel::AccountPanel(trtdata::WorkspaceWindowType type, QWidget* parent) : QFrame(parent), type(type) { initUi(); }
    trtdata::WorkspaceWindowType AccountPanel::getType() const { return type; }

    void AccountPanel::initUi()
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        status = new QLabel("OKX · Live account · Waiting for data…", this);
        status->setWordWrap(true);
        layout->addWidget(status);
        table = new QTableWidget(this);
        QStringList headers;
        switch (type)
        {
            case trtdata::WorkspaceWindowType::AccountBalance: headers = {"Currency", "Equity (USD)", "Equity", "Available", "Unrealized P&L", "Frozen"}; break;
            case trtdata::WorkspaceWindowType::AccountPositions: headers = {"Instrument", "Side", "Leverage", "Margin", "Position", "P&L", "Return", "Entry price", "Mark price", "Liq. price", "Break-even", "Margin mode", "Currency", "Available"}; break;
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
        const trtcore::AccountSnapshot& account = snapshot->account;
        const std::function<QString(const std::string&)> text = [](const std::string& value) { return QString::fromStdString(value); };
        const std::function<QString(const std::string&)> number = [](const std::string& value) { return trttrade::accountNumber(value); };
        switch (type)
        {
            case trtdata::WorkspaceWindowType::AccountBalance:
                timestamp = account.balance.timestamp; error = account.balance.error;
                if (timestamp != displayedTimestamp)
                    for (const trttrade::BalanceDetail& row : account.balance.data.details)
                        rows.push_back({text(row.ccy), number(row.eqUsd), number(row.eq), number(row.availBal), trttrade::accountNumber(row.upl, true), number(row.frozenBal)});
                break;
            case trtdata::WorkspaceWindowType::AccountPositions:
                timestamp = account.positions.timestamp; error = account.positions.error;
                if (timestamp != displayedTimestamp)
                    for (const trttrade::PositionData& row : account.positions.data)
                        rows.push_back({text(row.instId), trttrade::accountPositionSide(row), number(row.lever) + "x", number(row.margin) + " " + text(row.ccy), number(row.pos) + (row.instType == "MARGIN" ? " " + text(row.posCcy) : " contracts"), trttrade::accountNumber(row.upl, true) + " " + text(row.ccy), trttrade::accountPercent(row.uplRatio), number(row.avgPx), number(row.markPx), number(row.liqPx), number(row.bePx), text(row.mgnMode), text(row.ccy), number(row.availPos)});
                break;
            case trtdata::WorkspaceWindowType::AccountOrders:
                timestamp = account.orders.timestamp; error = account.orders.error;
                if (timestamp != displayedTimestamp)
                    for (const trttrade::OrderData& row : account.orders.data)
                        rows.push_back({text(row.instId), text(row.ordType), text(row.side), number(row.px), number(row.sz), number(row.accFillSz), number(row.avgPx), text(row.state), text(row.ordId)});
                break;
            default: return;
        }
        if (timestamp != displayedTimestamp) { displayRows(rows); displayedTimestamp = timestamp; }
        QString message = "OKX · Live";
        if (!error.isEmpty()) message += " · " + error + " (previous data may be stale)";
        status->setText(message);
    }
}
