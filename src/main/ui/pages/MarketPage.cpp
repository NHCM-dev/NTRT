//
// Created by NHCM on 2026/11/7.
//

#include "MarketPage.h"
#include <QTableWidget>
#include "main/NTRT.h"
#include <QHeaderView>
#include <QLocale>

#include <QFutureWatcher>
#include <exception>

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

        int instCounts = ntrt::dataManager->instrumentsCount();
        table->setRowCount(instCounts);

        for (int row = 0; row < instCounts; row++)
        {
            trtdata::Instrument& instrument = ntrt::dataManager->getInstrument(row);
            table->setItem(row, 0, new QTableWidgetItem(trtdata::getExchangeStr(instrument.exchange).data()));
            table->setItem(row, 1, new QTableWidgetItem(instrument.instId.data()));
            table->setItem(row, 2, new QTableWidgetItem("No data"));
            table->setItem(row, 3, new QTableWidgetItem("No data"));
            table->setItem(row, 4, new QTableWidgetItem("No data"));
        }

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
        if (pendingRequests != 0)
            return;

        const int count = ntrt::dataManager->instrumentsCount();
        table->setRowCount(count);
        for (int row = 0; row < count; row++)
        {
            trtdata::Instrument& instrument = ntrt::dataManager->getInstrument(row);
            table->setItem(row, 0, new QTableWidgetItem(trtdata::getExchangeStr(instrument.exchange).data()));
            table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(instrument.instId)));
            QFutureWatcher<trtdata::TickerData>* watcher = new QFutureWatcher<trtdata::TickerData>(table);
            pendingRequests++;

            QObject::connect(watcher, &QFutureWatcher<trtdata::TickerData>::finished, table, [this, watcher, row]
            {
                try
                {
                    const trtdata::TickerData data = watcher->result();
                    const QLocale locale(QLocale::English);
                    const QString price = data.last >= 1 ? locale.toString(data.last, 'f', 2) : locale.toString(data.last, 'g', 4);
                    table->setItem(row, 2, new QTableWidgetItem(price));

                    const QString instId = QString::fromStdString(data.instId);
                    const bool spot = data.instType == trtdata::InstType::Spot || data.instType == trtdata::InstType::Margin;
                    const QString unit = instId.section('-', 0, 0);

                    double baseVol = spot ? data.vol24h : data.volCcy24h;
                    double quoteVol = spot ? data.volCcy24h : data.volCcy24h * data.last;

                    QTableWidgetItem* volume = new QTableWidgetItem(locale.toString(baseVol, 'f', 2) + " " + unit);
                    volume->setToolTip(locale.toString(quoteVol, 'f', 2) + " " + instId.section('-', 1, 1));
                    table->setItem(row, 3, volume);

                    QString change = "0.00%";
                    if (data.open24h != 0)
                    {
                        const double percent = (data.last - data.open24h) / data.open24h * 100.0;
                        change = QString::number(percent, 'f', 2) + "%";
                        if (percent > 0)
                            change.prepend('+');
                    }
                    table->setItem(row, 4, new QTableWidgetItem(change));
                }
                catch (const std::exception& error)
                {
                    QTableWidgetItem* status = new QTableWidgetItem("Request failed");
                    status->setToolTip(QString::fromUtf8(error.what()));
                    table->setItem(row, 2, status);
                    table->setItem(row, 3, new QTableWidgetItem("-"));
                    table->setItem(row, 4, new QTableWidgetItem("-"));
                }
                pendingRequests--;
                watcher->deleteLater();
            });
            watcher->setFuture(instrument.requestTicker());
        }
    }
} // trtui
