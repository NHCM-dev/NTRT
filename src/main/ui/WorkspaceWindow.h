#ifndef NTRT_WORKSPACEWINDOW_H
#define NTRT_WORKSPACEWINDOW_H
#include <QMdiSubWindow>

namespace trtui
{
    class WorkspaceWindow final : public QMdiSubWindow
    {
        bool positionConstraintPending = false;
        QRect previousGeometry;
        void initAppearance();
        void schedulePositionConstraint();
        void constrainPosition();
        void updateExposedFrame();
        void paintEvent(QPaintEvent* event) override;
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
