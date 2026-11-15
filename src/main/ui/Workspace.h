#ifndef NTRT_WORKSPACE_H
#define NTRT_WORKSPACE_H

#include <QFrame>
#include <QPointer>
#include <QMdiSubWindow>
#include "main/data/WorkspaceManager.h"
#include <memory>
#include <vector>
#include "main/market/abstract/Instrument.h"

class QAction;
class QMenu;
class QMdiArea;

namespace trtui
{
    class InstrumentListPanel;
    class ChartPanel;

    class Workspace final : public QFrame
    {
        struct WindowEntry
        {
            QMdiSubWindow* window = nullptr;
            QAction* action = nullptr;
            ChartPanel* chart = nullptr;
            QRect normalGeometry;
        };

        QMdiArea* desktop = nullptr;
        QMdiSubWindow* instrumentWindow = nullptr;
        QMdiSubWindow* lastChartWindow = nullptr;
        QPointer<QMdiSubWindow> selectedWindow;
        bool restoring = false;
        std::optional<trtdata::WorkspaceState> pendingState;
        QMenu* windowMenu = nullptr;
        QAction* windowActionsEnd = nullptr;
        int nextChartId = 1;
        std::unique_ptr<InstrumentListPanel> instruments;
        std::vector<WindowEntry> windows;

        struct WindowPlacement
        {
            QMdiSubWindow* window = nullptr;
            QRect geometry;
        };

        void sortWindows();

        void arrangeWindows(std::vector<WindowPlacement> placements, const QRect& area);

        void initDesktop();

        void initInstrumentPanel();

        QMdiSubWindow* addChartWindow();

        void showInstrument(const trtdata::InstIdentifier& identifier);

        void showWindow(QMdiSubWindow* window);

        void addWindowAction(WindowEntry& entry);

        QMdiSubWindow* addWindow(QWidget* content, const QString& title, const QString& id);

        bool eventFilter(QObject* watched, QEvent* event) override;

        void changeEvent(QEvent* event) override;

        void showEvent(QShowEvent* event) override;

        void restoreState(const trtdata::WorkspaceState& state);

        void applyRestoredLayout();

    public:
        explicit Workspace(QWidget* parent = nullptr, const trtdata::WorkspaceState* state = nullptr);

        trtdata::WorkspaceState getState() const;

        ~Workspace() override;

        QMenu* getWindowMenu();

        void showInstrumentList();

        void addWindowActions(QMenu* menu);

        void resetLayout();

        void updateData();

        void createChartWindow();
    };
}
#endif //NTRT_WORKSPACE_H
