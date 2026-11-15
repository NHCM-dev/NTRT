#ifndef NTRT_MAINWINDOW_H
#define NTRT_MAINWINDOW_H

#include <QMainWindow>

class QTabWidget;
class QAction;
class QMenu;
class QTimer;

namespace trtui
{
    class Workspace;
    class SettingsDialog;

    class MainWindow final : public QMainWindow
    {
        QTabWidget* workspaces = nullptr;
        QAction* windowAction = nullptr;
        QMenu* elementsMenu = nullptr;
        int nextWorkspaceId = 1;

        Workspace* getCurrentWorkspace() const;

        void createWorkspace();

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
    };
}
#endif //NTRT_MAINWINDOW_H
