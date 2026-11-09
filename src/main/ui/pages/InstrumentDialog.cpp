#include "InstrumentDialog.h"
#include "main/data/api/AbstractApi.h"
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

namespace trtui
{
    InstrumentDialog::InstrumentDialog(trtdata::ConfigManager& config, Mode mode, QWidget* parent)
        : QDialog(parent), config(config), mode(mode)
    {
        const bool adding = mode == Mode::Add;
        setWindowTitle(adding ? "Add Trading Pairs" : "Manage Trading Pairs");
        resize(650, 480);
        QVBoxLayout* layout = new QVBoxLayout(this);
        if (adding)
        {
            QHBoxLayout* selectors = new QHBoxLayout;
            exchange = new QComboBox(this);
            type = new QComboBox(this);
            for (trtdata::Exchange item : trtcore::registeredExchanges())
                exchange->addItem(QString::fromUtf8(trtdata::getExchangeStr(item)), static_cast<int>(item));
            exchange->setCurrentIndex(exchange->findData(static_cast<int>(config.getExchange())));
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
        layout->addWidget(search);
        table = trtutils::createHeaderTable(this, {adding ? "Add" : "Remove", "Exchange", "Instrument", "Type"});
        table->verticalHeader()->hide();
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(adding ? QAbstractItemView::NoSelection : QAbstractItemView::SingleSelection);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        layout->addWidget(table, 1);
        connect(search, &QLineEdit::textChanged, this, &InstrumentDialog::filterRows);

        if (!adding)
        {
            QHBoxLayout* actions = new QHBoxLayout;
            upButton = new QPushButton("Move Up", this);
            downButton = new QPushButton("Move Down", this);
            actions->addWidget(upButton);
            actions->addWidget(downButton);
            actions->addStretch();
            layout->addLayout(actions);
            connect(upButton, &QPushButton::clicked, this, [this] { moveCurrent(-1); });
            connect(downButton, &QPushButton::clicked, this, [this] { moveCurrent(1); });
            connect(table, &QTableWidget::currentCellChanged, this, &InstrumentDialog::updateMoveButtons);
        }

        status = new QLabel(this);
        status->setWordWrap(true);
        layout->addWidget(status);
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        applyButton = buttons->button(QDialogButtonBox::Ok);
        applyButton->setText(adding ? "Add Checked" : "Apply Changes");
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &InstrumentDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        if (adding)
        {
            connect(exchange, &QComboBox::currentIndexChanged, this, &InstrumentDialog::loadTypes);
            connect(type, &QComboBox::currentIndexChanged, this, &InstrumentDialog::fetchCatalog);
            loadTypes();
        }
        else
        {
            const std::span<const InstrumentPtr> saved = config.getInstruments();
            entries.assign(saved.begin(), saved.end());
            populate();
            status->setText("Check rows to remove. Clear search to reorder the full list.");
        }
    }

    void InstrumentDialog::loadTypes()
    {
        const QSignalBlocker blocker(type);
        type->clear();
        const trtcore::AbstractApi& api = trtcore::exchangeApi(static_cast<trtdata::Exchange>(exchange->currentData().toInt()));
        for (trtdata::InstType item : api.instrumentTypes())
            type->addItem(QString::fromUtf8(trtdata::instrumentTypeName(item)), static_cast<int>(item));
        fetchCatalog();
    }

    void InstrumentDialog::fetchCatalog()
    {
        if (type->currentIndex() < 0)
            return;

        using Catalog = std::vector<InstrumentPtr>;
        const trtdata::Exchange selectedExchange = static_cast<trtdata::Exchange>(exchange->currentData().toInt());
        const trtdata::InstType selectedType = static_cast<trtdata::InstType>(type->currentData().toInt());
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
                const std::span<const InstrumentPtr> saved = config.getInstruments();
                for (const InstrumentPtr& candidate : catalog)
                {
                    const bool exists = std::ranges::any_of(saved, [&candidate](const InstrumentPtr& instrument)
                    {
                        return instrument->exchange == candidate->exchange && instrument->instType == candidate->instType && instrument->instId == candidate->instId;
                    });
                    if (!exists)
                        entries.push_back(candidate);
                }
                std::ranges::sort(entries, {}, &trtdata::Instrument::instId);
                populate();
                status->setText(entries.empty() ? "No instruments to add." : "Check the instruments you want to add.");
            }
            catch (const std::exception& error)
            {
                status->setText(QString::fromUtf8(error.what()));
            }
            exchange->setEnabled(true);
            type->setEnabled(true);
            reloadButton->setEnabled(true);
            applyButton->setEnabled(!entries.empty());
            watcher->deleteLater();
        });
        watcher->setFuture(api.fetchInstruments(selectedType));
    }

    void InstrumentDialog::populate()
    {
        table->clearContents();
        table->setRowCount(static_cast<int>(entries.size()));
        for (int row = 0; row < table->rowCount(); row++)
        {
            const InstrumentPtr& instrument = entries[row];
            QTableWidgetItem* check = new QTableWidgetItem;
            check->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            check->setCheckState(Qt::Unchecked);
            table->setItem(row, 0, check);
            table->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(trtdata::getExchangeStr(instrument->exchange))));
            table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(instrument->instId)));
            table->setItem(row, 3, new QTableWidgetItem(QString::fromUtf8(trtdata::instrumentTypeName(instrument->instType))));
        }
        filterRows();
    }

    void InstrumentDialog::filterRows()
    {
        const QString query = search->text().trimmed();
        for (int row = 0; row < table->rowCount(); row++)
        {
            const QString text = table->item(row, 1)->text() + " " + table->item(row, 2)->text() + " " + table->item(row, 3)->text();
            table->setRowHidden(row, !text.contains(query, Qt::CaseInsensitive));
        }
        updateMoveButtons();
    }

    void InstrumentDialog::moveCurrent(int direction)
    {
        const int row = table->currentRow();
        const int target = row + direction;
        std::swap(entries[row], entries[target]);
        for (int column = 0; column < table->columnCount(); column++)
        {
            QTableWidgetItem* source = table->takeItem(row, column);
            QTableWidgetItem* destination = table->takeItem(target, column);
            table->setItem(row, column, destination);
            table->setItem(target, column, source);
        }
        table->setCurrentCell(target, 2);
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
            const std::span<const InstrumentPtr> saved = config.getInstruments();
            updated.assign(saved.begin(), saved.end());
        }

        for (int row = 0; row < table->rowCount(); row++)
        {
            const bool checked = table->item(row, 0)->checkState() == Qt::Checked;
            if (checked == (mode == Mode::Add))
                updated.push_back(entries[row]);
        }

        const std::span<const InstrumentPtr> saved = config.getInstruments();
        if (!std::ranges::equal(saved, updated))
        {
            try
            {
                if (!config.replaceInstruments(std::move(updated)))
                {
                    status->setText("Could not save the market configuration.");
                    return;
                }
            }
            catch (const std::exception& error)
            {
                status->setText(QString::fromUtf8(error.what()));
                return;
            }
        }
        QDialog::accept();
    }
}
