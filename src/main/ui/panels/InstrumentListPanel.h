//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_INSTRUMENTLISTPANEL_H
#define NTRT_INSTRUMENTLISTPANEL_H
#include <QTableWidget>
#include "main/data/WorkspaceDef.h"
#include <QObject>

#include "main/ui/AbstractPage.h"

class QComboBox;
class QLineEdit;
class QVBoxLayout;

namespace trtui
{
    class InstrumentListPanel final : public QObject, public AbstractPage
    {
        Q_OBJECT
        QTableWidget* table = nullptr;
        QLineEdit* search = nullptr;
        QComboBox* typeFilter = nullptr;

        void initFilters(QFrame* page, QVBoxLayout* layout);

        void initTable(QFrame* page, QVBoxLayout* layout);

        void initActions(QFrame* page, QVBoxLayout* layout);

        void updateColumnVisibility();

        void clearInstrumentSelection();

        void updateHoveredRow();

        void setHoveredRow(int row);

        bool eventFilter(QObject* watched, QEvent* event) override;

    public:
        explicit InstrumentListPanel(QWidget* parent = nullptr) : QObject(parent)
        {
            initialize(parent);
        }

        QFrame* generateWidget(QWidget* parent) override;

        void update() override;

        trtdata::InstrumentListState getState() const;

        void restoreState(const trtdata::InstrumentListState& state);

        signals  :




        void instrumentActivated(const trtdata::InstIdentifier& identifier);
    };
} // trtui

#endif //NTRT_INSTRUMENTLISTPANEL_H
