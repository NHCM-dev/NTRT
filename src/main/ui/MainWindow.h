#ifndef NTRT_MAINWINDOW_H
#define NTRT_MAINWINDOW_H

#include <QMainWindow>
#include "main/data/config/WorkspaceConfig.h"
#include <QTabWidget>
#include <QAction>
#include <QMenu>
#include <QTimer>
#include "main/ui/Workspace.h"
#include "main/ui/dialog/SettingsDialog.h"

namespace trtui
{
    class MainWindow final : public QMainWindow
    {
        QTabWidget* workspaces = nullptr;
        QAction* windowAction = nullptr;
        QMenu* elementsMenu = nullptr;
        int nextWorkspaceId = 1;

        Workspace* getCurrentWorkspace() const;

        void createWorkspace(const trtdata::WorkspaceState* state = nullptr);

        void initWorkspacePersistence();

        void closeEvent(QCloseEvent* event) override;

        bool closing = false;

        void renameWorkspace(int index);

        void closeWorkspace(int index);

        void showWorkspaceMenu(const QPoint& position);

        SettingsDialog* settings = nullptr;
        QTimer* timer200ms = nullptr;

        void initLayouts();

        void initWindowGeometry();

        void initMenuBar();

        void initWorkspaceTabs();

        void initStatusBar();

        void init200msSchedules();

        void showSettings();

    public:
        explicit MainWindow(QWidget* parent = nullptr);

        bool saveWorkspaceState(bool notify = false);
    };
}
#endif //NTRT_MAINWINDOW_H
