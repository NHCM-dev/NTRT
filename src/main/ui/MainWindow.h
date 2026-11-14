#ifndef NTRT_MAINWINDOW_H
#define NTRT_MAINWINDOW_H

#include <QMainWindow>

class QHBoxLayout;
class QMenu;
class QTimer;

namespace trtui
{
    class Workspace;
    class SettingsDialog;
    class MainWindow final : public QMainWindow
    {
        QWidget* root = nullptr;
        QHBoxLayout* rootLayout = nullptr;
        Workspace* workspace = nullptr;
        SettingsDialog* settings = nullptr;
        QTimer* timer200ms = nullptr;

        void initLayouts();
        void initWindowGeometry();
        void initMenuBar();
        void initNavigation();
        void initStatusBar();
        void init200msSchedules();
        void showSettings();

    public:
        explicit MainWindow(QWidget* parent = nullptr);
    };
}
#endif //NTRT_MAINWINDOW_H
