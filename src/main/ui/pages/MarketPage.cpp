//
// Created by NHCM on 2026/11/7.
//

#include "MarketPage.h"
#include "../dialog/InstrumentDialog.h"
#include <QTableWidget>
#include "main/NTRT.h"
#include <QHeaderView>
#include <QLocale>
#include <QPushButton>
#include <QObject>
#include <QComboBox>
#include <QLineEdit>
#include <QScrollBar>
#include <QVBoxLayout>

#include "main/util/QtUtil.h"

namespace trtui
{
    QFrame* MarketPage::generateWidget(QWidget* parent)
    {
        QFrame* page = new QFrame(parent);

        page->setBackgroundRole(QPalette::Base);
        page->setAutoFillBackground(true);

        QVBoxLayout* layout = new QVBoxLayout(page);
        QHBoxLayout* filters = new QHBoxLayout;
        search = new QLineEdit(page);
        search->setPlaceholderText("Search instruments");
        typeFilter = new QComboBox(page);
        typeFilter->addItem("All types", -1);
        for (trtdata::InstType type : {trtdata::InstType::Spot, trtdata::InstType::Margin, trtdata::InstType::Swap,
                                     trtdata::InstType::Futures, trtdata::InstType::Option, trtdata::InstType::Events})
            typeFilter->addItem(QString::fromUtf8(trtdata::instrumentTypeName(type)), static_cast<int>(type));
        filters->addWidget(search, 1);
        filters->addWidget(typeFilter);
        layout->addLayout(filters);

        table = trtutils::createHeaderTable(page, {"Exchange", "Type", "Instrument", "Last Price", "24H Volume", "24H Change"});

        QHeaderView* header = table->horizontalHeader();
        header->setSectionResizeMode(0, QHeaderView::Interactive);
        header->setSectionResizeMode(3, QHeaderView::Interactive);
        header->setSectionResizeMode(4, QHeaderView::Interactive);
        header->setSectionResizeMode(5, QHeaderView::Interactive);
        header->setSectionResizeMode(1, QHeaderView::Fixed);

        header->setMinimumSectionSize(100);
        header->setSectionResizeMode(2, QHeaderView::Stretch);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        table->setColumnWidth(0, 100);
        table->setColumnWidth(1, 100);
        table->setColumnWidth(3, 110);
        table->setColumnWidth(4, 130);
        table->setColumnWidth(5, 100);
        table->verticalHeader()->hide();
        int minimumWidth = header->minimumSectionSize() + table->verticalScrollBar()->sizeHint().width();
        for (int column = 0; column < header->count(); column++)
        {
            if (header->sectionResizeMode(column) != QHeaderView::Stretch)
                minimumWidth += header->sectionSize(column);
        }
        table->setMinimumWidth(minimumWidth + 2 * table->frameWidth());
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(table, 1);

        QHBoxLayout* actions = new QHBoxLayout;
        actions->addStretch();
        QPushButton* manageButton = new QPushButton("Manage", page);
        QPushButton* addButton = new QPushButton("Add", page);
        actions->addWidget(manageButton);
        actions->addWidget(addButton);
        layout->addLayout(actions);

        QObject::connect(search, &QLineEdit::textChanged, page, [this] { update(); });
        QObject::connect(typeFilter, &QComboBox::currentIndexChanged, page, [this] { update(); });
        QObject::connect(addButton, &QPushButton::clicked, page, [this, page]
        {
            InstrumentDialog dialog(*ntrt::configManager, InstrumentDialog::Mode::Add, page);
            if (dialog.exec() == QDialog::Accepted)
                update();
        });
        QObject::connect(manageButton, &QPushButton::clicked, page, [this, page]
        {
            InstrumentDialog dialog(*ntrt::configManager, InstrumentDialog::Mode::Manage, page);
            if (dialog.exec() == QDialog::Accepted)
                update();
        });

        return page;
    }

    void MarketPage::update()
    {
        const std::span<const std::shared_ptr<trtdata::Instrument>> instruments = ntrt::configManager->getInstruments();
        const trtcore::MarketData& marketData = ntrt::dataMonitor->getMarketData();
        const QString query = search->text().trimmed();
        const int filter = typeFilter->currentData().toInt();
        const QLocale locale(QLocale::English);
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
            table->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(trtdata::instrumentTypeName(instrument->instType))));
            table->setItem(row, 2, new QTableWidgetItem(symbol));

            const trtcore::MarketSnapshot* snapshot = marketData.getSnapshot(instrument->getUniqueIdentifier());
            if (!snapshot || !snapshot->ticker)
            {
                QTableWidgetItem* status = new QTableWidgetItem("No data");
                if (snapshot && !snapshot->error.isEmpty())
                {
                    status->setText("Request failed");
                    status->setToolTip(snapshot->error);
                }
                table->setItem(row, 3, status);
                table->setItem(row, 4, new QTableWidgetItem("-"));
                QTableWidgetItem* change = new QTableWidgetItem("-");
                change->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                table->setItem(row, 5, change);
                row++;
                continue;
            }

            const trtdata::TickerData& data = *snapshot->ticker;
            const QString price = data.last >= 1 ? locale.toString(data.last, 'f', 2) : locale.toString(data.last, 'g', 4);
            QTableWidgetItem* priceItem = new QTableWidgetItem(price);
            if (!snapshot->error.isEmpty())
                priceItem->setToolTip("Last successful quote: " + snapshot->error);
            table->setItem(row, 3, priceItem);

            const QString instId = QString::fromStdString(data.instId);
            const bool spot = data.instType == trtdata::InstType::Spot || data.instType == trtdata::InstType::Margin;
            const double baseVol = spot ? data.vol24h : data.volCcy24h;
            const double quoteVol = spot ? data.volCcy24h : data.volCcy24h * data.last;
            QTableWidgetItem* volume = new QTableWidgetItem(locale.toString(baseVol, 'f', 2) + " " + instId.section('-', 0, 0));
            volume->setToolTip(locale.toString(quoteVol, 'f', 2) + " " + instId.section('-', 1, 1));
            table->setItem(row, 4, volume);

            QTableWidgetItem* change = new QTableWidgetItem("N/A");
            if (data.open24h != 0)
            {
                const double percent = (data.last - data.open24h) / data.open24h * 100.0;
                const QString text = QString::number(percent, 'f', 2);
                if (text == "0.00" || text == "-0.00")
                    change->setText("0.00%");
                else
                {
                    change->setText((percent > 0 ? "+" : "") + text + "%");
                    change->setForeground(percent >= 0 ? QColor(0x15803d) : QColor(0xb91c1c));
                }
                change->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            table->setItem(row, 5, change);
            row++;
        }
        table->setRowCount(row);
    }
} // trtui
