#ifndef NTRT_INSTRUMENTDIALOG_H
#define NTRT_INSTRUMENTDIALOG_H

#include "main/data/ConfigManager.h"
#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace trtui
{
    class InstrumentDialog final : public QDialog
    {
    public:
        enum class Mode { Add, Manage };
        InstrumentDialog(trtdata::ConfigManager& config, Mode mode, QWidget* parent);

    private:
        using InstrumentPtr = std::shared_ptr<trtdata::Instrument>;
        trtdata::ConfigManager& config;
        const Mode mode;
        std::vector<InstrumentPtr> entries;
        QTableWidget* table;
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
