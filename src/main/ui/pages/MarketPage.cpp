//
// Created by NHCM on 2026/11/7.
//

#include "MarketPage.h"
#include <QTableWidget>
#include "main/NTRT.h"
#include <QHeaderView>
#include <QLocale>


namespace trtui
{
    QWidget* MarketPage::generateWidget(QWidget* parent)
    {
        table = new QTableWidget(parent);
        QStringList headers = {
            "Exchange",
            "Instrument",
            "Last Price",
            "24H Volume",
            "24H Change"};
        table->setColumnCount(headers.count());
        table->setHorizontalHeaderLabels(headers);

        // Auto adjust column size
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        // Hide vertical header
        table->verticalHeader()->hide();

        // Disable editing
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);

        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->setFocusPolicy(Qt::NoFocus);

        // Do not display grids
        table->setShowGrid(false);
        return table;
    }

    void MarketPage::update()
    {
        const std::span<const std::shared_ptr<trtdata::Instrument>> instruments = ntrt::configManager->getInstruments();
        const trtcore::MarketData& marketData = ntrt::dataMonitor->getMarketData();
        const int count = static_cast<int>(instruments.size());
        const QLocale locale(QLocale::English);
        table->setRowCount(count);
        for (int row = 0; row < count; row++)
        {
            const std::shared_ptr<trtdata::Instrument>& instrument = instruments[row];
            table->setItem(row, 0, new QTableWidgetItem(trtdata::getExchangeStr(instrument->exchange).data()));
            table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(instrument->instId)));

            const trtcore::MarketSnapshot* snapshot = marketData.getSnapshot(instrument);
            if (!snapshot || !snapshot->ticker)
            {
                QTableWidgetItem* status = new QTableWidgetItem("No data");
                if (snapshot && !snapshot->error.isEmpty())
                {
                    status->setText("Request failed");
                    status->setToolTip(snapshot->error);
                }
                table->setItem(row, 2, status);
                table->setItem(row, 3, new QTableWidgetItem("-"));
                table->setItem(row, 4, new QTableWidgetItem("-"));
                continue;
            }

            const trtdata::TickerData& data = *snapshot->ticker;
            const QString price = data.last >= 1 ? locale.toString(data.last, 'f', 2) : locale.toString(data.last, 'g', 4);
            QTableWidgetItem* priceItem = new QTableWidgetItem(price);
            if (!snapshot->error.isEmpty())
                priceItem->setToolTip("Last successful quote: " + snapshot->error);
            table->setItem(row, 2, priceItem);

            const QString instId = QString::fromStdString(data.instId);
            const bool spot = data.instType == trtdata::InstType::Spot || data.instType == trtdata::InstType::Margin;
            const double baseVol = spot ? data.vol24h : data.volCcy24h;
            const double quoteVol = spot ? data.volCcy24h : data.volCcy24h * data.last;
            QTableWidgetItem* volume = new QTableWidgetItem(locale.toString(baseVol, 'f', 2) + " " + instId.section('-', 0, 0));
            volume->setToolTip(locale.toString(quoteVol, 'f', 2) + " " + instId.section('-', 1, 1));
            table->setItem(row, 3, volume);

            QString change = "N/A";
            if (data.open24h != 0)
            {
                const double percent = (data.last - data.open24h) / data.open24h * 100.0;
                change = QString::number(percent, 'f', 2) + "%";
                if (percent > 0)
                    change.prepend('+');
            }
            table->setItem(row, 4, new QTableWidgetItem(change));
        }
    }
} // trtui
