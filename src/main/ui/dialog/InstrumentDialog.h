#ifndef NTRT_INSTRUMENTDIALOG_H
#define NTRT_INSTRUMENTDIALOG_H

#include "main/data/persistence/ConfigManager.h"
#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "main/ui/widgets/SelectionTable.h"

namespace trtui
{
    class InstrumentDialog final : public QDialog
    {
    public:
        enum class Mode { Add, Manage };

        InstrumentDialog(Mode mode, QWidget* parent);

    private:
        using InstrumentPtr = std::shared_ptr<trtdata::Instrument>;
        const Mode mode;
        std::vector<InstrumentPtr> entries;
        SelectionTable* table;
        QLineEdit* search;
        QComboBox* exchange = nullptr;
        QComboBox* type = nullptr;
        QLabel* status;
        QPushButton* applyButton;
        QPushButton* reloadButton = nullptr;
        QPushButton* upButton = nullptr;
        QPushButton* downButton = nullptr;

        void loadTypes();

        void fetchCatalog();

        void populate();

        void filterRows();

        void moveCurrent(int direction);

        void updateMoveButtons();

        void accept() override;
    };
}

#endif //NTRT_INSTRUMENTDIALOG_H
