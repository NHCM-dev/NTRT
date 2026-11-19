#ifndef NTRT_BOOKDEPTHPANEL_H
#define NTRT_BOOKDEPTHPANEL_H

#include "main/data/config/WorkspaceConfig.h"
#include "main/trade/TradeDataDef.h"
#include "main/trade/TradeSnapshotDef.h"
#include <QFrame>
#include <QTimer>
#include <memory>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QToolButton>
#include "main/monitor/DataMonitor.h"
#include "main/ui/widgets/NumericWidgets.h"

namespace trtui
{
    class BookDepthPanel final : public QFrame
    {
        Q_OBJECT
        std::weak_ptr<trtcore::DataMonitor> monitor;
        trtdata::BookDepthPanelState state;
        std::vector<trtdata::InstIdentifier> choices;
        QWidget* settingsControls = nullptr;
        QToolButton* settingsToggle = nullptr;
        QComboBox* instrumentBox = nullptr;
        QComboBox* alignmentBox = nullptr;
        QComboBox* layoutBox = nullptr;
        QSpinBox* depthBox = nullptr;
        QSpinBox* rowsBox = nullptr;
        QDoubleSpinBox* stepBox = nullptr;
        QCheckBox* cumulativeBox = nullptr;
        QTableWidget* asks = nullptr;
        QTableWidget* bids = nullptr;
        PriceLabel* latest = nullptr;
        RatioBar* balance = nullptr;
        QLabel* status = nullptr;
        QTimer displayDeadline;
        std::shared_ptr<const trttrade::BookSnapshot> displayedBook;

        void initControls(QVBoxLayout* layout);
        void initContent(QVBoxLayout* layout);
        void rebuildInstruments();
        void applyControls();
        void releaseBook();
        void updateSide(QTableWidget* table, const std::vector<trttrade::BookLevel>& levels, bool buy);
        void changeEvent(QEvent* event) override;
        void showEvent(QShowEvent* event) override;

    public:
        explicit BookDepthPanel(QWidget* parent = nullptr);
        ~BookDepthPanel() override;
        void setInstrument(const trtdata::InstIdentifier& instrument);
        trtdata::BookDepthPanelState getState() const;
        void restoreState(const trtdata::BookDepthPanelState& value);
        void updateData();

    signals:
        void titleChanged(const QString& title);
    };
}
#endif
