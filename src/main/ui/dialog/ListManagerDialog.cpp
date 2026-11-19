#include "ListManagerDialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>

namespace trtui
{
    ListManagerDialog::ListManagerDialog(const QString& title, std::function<Items()> provider, QWidget* parent, bool multiple)
        : QDialog(parent), provider(std::move(provider)), multiple(multiple)
    {
        setWindowTitle(title);
        initUi();
        refresh();
    }

    void ListManagerDialog::initUi()
    {
        setObjectName("listManagerDialog");
        resize(680, 480);
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(12);
        initSearch(layout);
        list = new SelectionTable({"Name"}, multiple, this);
        layout->addWidget(list, 1);
        connect(search, &QLineEdit::textChanged, this, [this](const QString& query) { list->filterRows(query); });
        connect(list, &SelectionTable::checkedRowsChanged, this, &ListManagerDialog::updateActions);
        initFooter(layout);
    }

    void ListManagerDialog::initSearch(QVBoxLayout* layout)
    {
        QHBoxLayout* toolbar = new QHBoxLayout;
        search = new QLineEdit(this);
        search->setPlaceholderText("Search by name");
        search->setClearButtonEnabled(true);
        toolbar->addWidget(search, 1);
        if (multiple)
        {
            QPushButton* select = new QPushButton("Select All", this);
            QPushButton* clear = new QPushButton("Clear", this);
            select->setAutoDefault(false);
            clear->setAutoDefault(false);
            toolbar->addWidget(select);
            toolbar->addWidget(clear);
            connect(select, &QPushButton::clicked, this, [this] { list->selectVisible(true); });
            connect(clear, &QPushButton::clicked, this, [this] { list->selectVisible(false); });
        }
        layout->addLayout(toolbar);
    }

    void ListManagerDialog::initFooter(QVBoxLayout* layout)
    {
        QHBoxLayout* footer = new QHBoxLayout;
        count = new QLabel(this);
        count->setObjectName("selectionCount");
        footer->addWidget(count);
        footer->addStretch();
        actions = new QHBoxLayout;
        actions->setSpacing(8);
        footer->addLayout(actions);
        QPushButton* close = new QPushButton("Close", this);
        close->setAutoDefault(false);
        footer->addWidget(close);
        connect(close, &QPushButton::clicked, this, &QDialog::reject);
        layout->addLayout(footer);
    }

    QStringList ListManagerDialog::getSelectedIds() const { return list->getCheckedIds(); }

    void ListManagerDialog::addAction(const QString& text, std::function<void(const QStringList&)> callback, int minimum, int maximum)
    {
        QPushButton* button = new QPushButton(text, this);
        button->setAutoDefault(false);
        actions->addWidget(button);
        buttons.push_back({button, minimum, maximum});
        connect(button, &QPushButton::clicked, this, [this, callback]
        {
            const QStringList selection = getSelectedIds();
            callback(selection);
            refresh();
        });
        updateActions();
    }

    void ListManagerDialog::reload() { refresh(); }

    void ListManagerDialog::refresh()
    {
        const QStringList selection = getSelectedIds();
        list->setRowCount(0);
        for (const QPair<QString, QString>& entry : provider())
        {
            list->appendRow(entry.first, {entry.second}, selection.contains(entry.first));
        }
        list->filterRows(search->text());
        updateActions();
    }

    void ListManagerDialog::updateActions()
    {
        const qsizetype selectedCount = list->getCheckedIds().size();
        count->setText(QString::number(selectedCount) + " selected");
        for (const Action& action : buttons)
            action.button->setEnabled(selectedCount >= action.minimum && (action.maximum < 0 || selectedCount <= action.maximum));
    }
}
