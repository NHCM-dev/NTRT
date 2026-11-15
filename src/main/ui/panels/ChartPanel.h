#ifndef NTRT_CHARTPANEL_H
#define NTRT_CHARTPANEL_H
#include <QFrame>
#include "main/data/WorkspaceDef.h"
#include "main/trade/TradeDef.h"
#include <memory>
#include <vector>

class QComboBox;
class QLabel;
class QStackedWidget;
class QToolButton;
class QVBoxLayout;

namespace trtcore
{
    class DataMonitor;
}

namespace trtui
{
    class ChartView;

    class ChartPanel final : public QFrame
    {
        Q_OBJECT

        std::weak_ptr<trtcore::DataMonitor> monitor;
        std::optional<trtdata::InstIdentifier> instrument;
        std::vector<trtdata::InstIdentifier> choices;
        trttrade::CandleTimeframe timeframe = trttrade::CandleTimeframe::Minute15;
        QComboBox* instrumentBox = nullptr;
        QComboBox* timeframeBox = nullptr;
        QToolButton* followButton = nullptr;
        QToolButton* volumeButton = nullptr;
        QStackedWidget* content = nullptr;
        ChartView* chart = nullptr;
        QLabel* status = nullptr;
        qint64 displayedUpdate = -1;
        QString renderingError;

        void initToolbar(QVBoxLayout* layout);

        void initContent(QVBoxLayout* layout);

        void initStatus(QVBoxLayout* layout);

        void rebuildInstruments();

        void selectInstrument();

        void setTimeframe(trttrade::CandleTimeframe value);

        void releaseSeries();

    public:
        explicit ChartPanel(QWidget* parent = nullptr);

        ~ChartPanel() override;

        void setInstrument(const trtdata::InstIdentifier& identifier);

        const std::optional<trtdata::InstIdentifier>& getInstrument() const;

        QString getTitle() const;

        trtdata::ChartPanelState getState() const;

        void restoreState(const trtdata::ChartPanelState& state);

        void updateData();

        signals  :




        void titleChanged(const QString& title);
    };
}
#endif //NTRT_CHARTPANEL_H
