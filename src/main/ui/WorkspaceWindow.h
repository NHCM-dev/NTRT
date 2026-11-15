#ifndef NTRT_WORKSPACEWINDOW_H
#define NTRT_WORKSPACEWINDOW_H
#include <QMdiSubWindow>

namespace trtui
{
    class WorkspaceWindow final : public QMdiSubWindow
    {
        bool constrainingPosition = false;
        void initAppearance();
        void constrainPosition();
        void moveEvent(QMoveEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

        void closeEvent(QCloseEvent* event) override;

        void mouseMoveEvent(QMouseEvent* event) override;

        void leaveEvent(QEvent* event) override;

    public:
        explicit WorkspaceWindow(QWidget* parent = nullptr);
    };
}
#endif //NTRT_WORKSPACEWINDOW_H
