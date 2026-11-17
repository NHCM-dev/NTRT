#ifndef NTRT_WORKSPACE_H
#define NTRT_WORKSPACE_H

#include <QFrame>
#include <QPointer>
#include <QMdiSubWindow>
#include "main/data/WorkspaceDef.h"
#include <memory>
#include <vector>
#include <QAction>
#include <QMenu>
#include <QMdiArea>
#include "main/ui/WorkspaceWindow.h"
#include "main/ui/panels/InstrumentListPanel.h"
#include "main/ui/panels/ChartPanel.h"
#include "main/ui/panels/BookDepthPanel.h"

namespace trtui
{
    class Workspace final : public QFrame
    {
        struct WindowEntry
        {
            WorkspaceWindow* window = nullptr;
            ChartPanel* chart = nullptr;
            BookDepthPanel* bookDepth = nullptr;
        };

        QMdiArea* desktop = nullptr;
        WorkspaceWindow* instrumentWindow = nullptr;
        QMdiSubWindow* lastChartWindow = nullptr;
        QPointer<QMdiSubWindow> selectedWindow;
        bool restoring = false;
        std::optional<trtdata::WorkspaceState> pendingState;
        QMenu* windowMenu = nullptr;
        QAction* windowActionsEnd = nullptr;
        int nextChartId = 1;
        int nextBookDepthId = 1;
        std::unique_ptr<InstrumentListPanel> instruments;
        std::vector<WindowEntry> windows;
        std::vector<QPointer<QMdiSubWindow>> instRequiredWindow;

        void rememberInstrumentWindow(QMdiSubWindow* window);

        struct WindowPlacement
        {
            WorkspaceWindow* window = nullptr;
            QRect geometry;
        };

        void sortWindows();

        void arrangeWindows(std::vector<WindowPlacement> placements, const QRect& area);

        void initDesktop();

        void initInstrumentPanel();

        WorkspaceWindow* addChartWindow();

        WorkspaceWindow* addBookDepthWindow();

        void showInstrument(const trtdata::InstIdentifier& identifier);

        WorkspaceWindow* addWindow(QWidget* content, const QString& title, const QString& id);

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

        void createBookDepthWindow();
    };
}
#endif //NTRT_WORKSPACE_H
