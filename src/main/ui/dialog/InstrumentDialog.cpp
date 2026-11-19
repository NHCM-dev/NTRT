#include "InstrumentDialog.h"
#include "main/util/TradeUtil.h"
#include "main/api/AbstractApi.h"
#include "main/util/QtUtil.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <algorithm>
#include <exception>

#include "main/NTRT.h"

namespace trtui
{
    InstrumentDialog::InstrumentDialog(Mode mode, QWidget* parent)
    : QDialog(parent), mode(mode)
    {
        const bool adding = mode == Mode::Add;
        setWindowTitle(adding ? "Add Trading Pairs" : "Manage Trading Pairs");
        setObjectName("listManagerDialog");
        resize(680, 480);
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(12);
        if (adding)
        {
            QHBoxLayout* selectors = new QHBoxLayout;
            exchange = new QComboBox(this);
            type = new QComboBox(this);
            for (trtdata::Exchange item : trtcore::availableExchanges())
                exchange->addItem(QString::fromUtf8(trtdata::getExchangeStr(item)), static_cast<int>(item));
            exchange->setCurrentIndex(exchange->findData(static_cast<int>(ntrt::configManager->getExchange())));
            if (exchange->currentIndex() < 0)
                exchange->setCurrentIndex(0);
            selectors->addWidget(exchange);
            selectors->addWidget(type, 1);
            reloadButton = new QPushButton("Reload", this);
            selectors->addWidget(reloadButton);
            connect(reloadButton, &QPushButton::clicked, this, &InstrumentDialog::fetchCatalog);
            layout->addLayout(selectors);
        }

        search = new QLineEdit(this);
        search->setPlaceholderText("Search instruments");
        search->setClearButtonEnabled(true);
        QHBoxLayout* searchBar = new QHBoxLayout;
        searchBar->addWidget(search, 1);
        QPushButton* selectAll = new QPushButton("Select All", this);
        QPushButton* clear = new QPushButton("Clear", this);
        selectAll->setAutoDefault(false);
        clear->setAutoDefault(false);
        searchBar->addWidget(selectAll);
        searchBar->addWidget(clear);
        layout->addLayout(searchBar);
        table = new SelectionTable({"Exchange", "Type", "Instrument"}, true, this);
        table->setColumnWidth(1, 100);
        table->setColumnWidth(2, 100);
        layout->addWidget(table, 1);
        connect(search, &QLineEdit::textChanged, this, &InstrumentDialog::filterRows);
        connect(selectAll, &QPushButton::clicked, this, [this] { table->selectVisible(true); });
        connect(clear, &QPushButton::clicked, this, [this] { table->selectVisible(false); });

        if (!adding)
        {
            QHBoxLayout* actions = new QHBoxLayout;
            upButton = new QPushButton("Move Up", this);
            downButton = new QPushButton("Move Down", this);
            actions->addWidget(upButton);
            actions->addWidget(downButton);
            actions->addStretch();
            layout->addLayout(actions);
            connect(upButton, &QPushButton::clicked, this, [this]
            {
                moveCurrent(-1);
            });
            connect(downButton, &QPushButton::clicked, this, [this]
            {
                moveCurrent(1);
            });
            connect(table, &QTableWidget::currentCellChanged, this, &InstrumentDialog::updateMoveButtons);
        }

        status = new QLabel(this);
        status->setWordWrap(true);
        status->setObjectName("selectionCount");
        layout->addWidget(status);
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        applyButton = buttons->button(QDialogButtonBox::Ok);
        applyButton->setText(adding ? "Add Selected" : "Apply");
        QHBoxLayout* footer = new QHBoxLayout;
        footer->addStretch();
        if (!adding)
        {
            QPushButton* removeButton = new QPushButton("Remove Selected", this);
            removeButton->setAutoDefault(false);
            removeButton->setEnabled(false);
            connect(table, &SelectionTable::checkedRowsChanged, removeButton, [this, removeButton] { removeButton->setEnabled(!table->getCheckedIds().isEmpty()); });
            footer->addWidget(removeButton);
            connect(removeButton, &QPushButton::clicked, this, [this, removeButton]
            {
                for (int row = table->rowCount() - 1; row >= 0; row--)
                {
                    if (table->isRowChecked(row))
                    {
                        entries.erase(entries.begin() + row);
                        table->removeRow(row);
                    }
                }
                updateMoveButtons();
                removeButton->setEnabled(false);
                status->setText("0 selected");
            });
        }
        footer->addWidget(buttons);
        layout->addLayout(footer);
        connect(table, &SelectionTable::checkedRowsChanged, this, [this, adding]
        {
            const qsizetype selected = table->getCheckedIds().size();
            status->setText(QString::number(selected) + " selected");
            if (adding) applyButton->setEnabled(selected > 0);
        });
        connect(buttons, &QDialogButtonBox::accepted, this, &InstrumentDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        if (adding)
        {
            connect(exchange, &QComboBox::currentIndexChanged, this, &InstrumentDialog::loadTypes);
            connect(type, &QComboBox::currentIndexChanged, this, &InstrumentDialog::fetchCatalog);
            loadTypes();
        } else
        {
            const std::span<const InstrumentPtr> saved = ntrt::configManager->getInstruments();
            entries.assign(saved.begin(), saved.end());
            populate();
            status->setText("0 selected");
        }
    }

    void InstrumentDialog::loadTypes()
    {
        const QSignalBlocker blocker(type);
        type->clear();
        const trtcore::AbstractApi& api = trtcore::exchangeApi(
            static_cast<trtdata::Exchange>(exchange->currentData().toInt()));
        for (trttrade::InstType item : api.instrumentTypes())
            type->addItem(QString::fromUtf8(trttrade::instDisplayStr(item)), static_cast<int>(item));
        fetchCatalog();
    }

    void InstrumentDialog::fetchCatalog()
    {
        if (type->currentIndex() < 0)
            return;

        using Catalog = std::vector<InstrumentPtr>;
        const trtdata::Exchange selectedExchange = static_cast<trtdata::Exchange>(exchange->currentData().toInt());
        const trttrade::InstType selectedType = static_cast<trttrade::InstType>(type->currentData().toInt());
        const trtcore::AbstractApi& api = trtcore::exchangeApi(selectedExchange);
        entries.clear();
        populate();
        status->setText("Loading instruments...");
        exchange->setEnabled(false);
        type->setEnabled(false);
        applyButton->setEnabled(false);
        reloadButton->setEnabled(false);

        QFutureWatcher<Catalog>* watcher = new QFutureWatcher<Catalog>(this);
        connect(watcher, &QFutureWatcher<Catalog>::finished, this, [this, watcher]
        {
            try
            {
                const Catalog catalog = watcher->result();
                const std::span<const InstrumentPtr> saved = ntrt::configManager->getInstruments();
                for (const InstrumentPtr& candidate : catalog)
                {
                    const bool exists = std::ranges::any_of(saved, [&candidate](const InstrumentPtr& instrument)
                    {
                        return instrument->exchange == candidate->exchange && instrument->instType == candidate->
                            instType && instrument->instId == candidate->instId;
                    });
                    if (!exists)
                        entries.push_back(candidate);
                }
                std::ranges::sort(entries, {}, &trtdata::Instrument::instId);
                populate();
                status->setText(entries.empty() ? "No instruments to add." : "0 selected");
            } catch (const std::exception& error)
            {
                status->setText(QString::fromUtf8(error.what()));
            }
            exchange->setEnabled(true);
            type->setEnabled(true);
            reloadButton->setEnabled(true);
            applyButton->setEnabled(!table->getCheckedIds().isEmpty());
            watcher->deleteLater();
        });
        watcher->setFuture(api.fetchInstruments(selectedType));
    }

    void InstrumentDialog::populate()
    {
        table->setRowCount(0);
        for (const InstrumentPtr& instrument : entries)
            table->appendRow(QString::fromStdString(instrument->instId), {QString::fromUtf8(trtdata::getExchangeStr(instrument->exchange)), QString::fromUtf8(instDisplayStr(instrument->instType)), QString::fromStdString(instrument->instId)});
        filterRows();
    }

    void InstrumentDialog::filterRows()
    {
        table->filterRows(search->text());
        updateMoveButtons();
    }

    void InstrumentDialog::moveCurrent(int direction)
    {
        const int row = table->currentRow();
        const int target = row + direction;
        std::swap(entries[row], entries[target]);
        trtutils::swapTableRows(table, row, target);
        table->setCurrentCell(target, 3);
        updateMoveButtons();
    }

    void InstrumentDialog::updateMoveButtons()
    {
        if (mode == Mode::Add)
            return;
        const int row = table->currentRow();
        const bool canMove = row >= 0 && search->text().trimmed().isEmpty();
        upButton->setEnabled(canMove && row > 0);
        downButton->setEnabled(canMove && row + 1 < table->rowCount());
    }

    void InstrumentDialog::accept()
    {
        std::vector<InstrumentPtr> updated;
        if (mode == Mode::Add)
        {
            const std::span<const InstrumentPtr> saved = ntrt::configManager->getInstruments();
            updated.assign(saved.begin(), saved.end());
        }

        for (int row = 0; row < table->rowCount(); row++)
        {
            const bool checked = table->isRowChecked(row);
            if (mode == Mode::Manage || checked)
                updated.push_back(entries[row]);
        }

        const std::span<const InstrumentPtr> saved = ntrt::configManager->getInstruments();
        if (!std::ranges::equal(saved, updated))
        {
            try
            {
                if (!ntrt::configManager->replaceInstruments(std::move(updated)))
                {
                    status->setText("Could not save the market configuration.");
                    return;
                }
            } catch (const std::exception& error)
            {
                status->setText(QString::fromUtf8(error.what()));
                return;
            }
        }
        QDialog::accept();
    }
}
