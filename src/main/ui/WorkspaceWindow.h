#ifndef NTRT_WORKSPACEWINDOW_H
#define NTRT_WORKSPACEWINDOW_H
#include <QMdiSubWindow>

class QAction;
class QMenu;

namespace trtui
{
    class WorkspaceWindow final : public QMdiSubWindow
    {
        QRect previousGeometry;
        QRect normalGeometry;
        QRect requestedGeometry;
        QRect resizeGeometry;
        QPoint resizeOrigin;
        Qt::Edges resizeEdges;
        QAction* windowAction = nullptr;
        bool constraining = false;
        bool constraintPending = false;
        void initAppearance();
        void initContent(QWidget* content, const QString& title, const QString& id);
        void constrainGeometry();
        void scheduleGeometryConstraint();
        Qt::Edges getResizeEdges(const QPoint& position) const;
        void updateWindowAction();
        bool eventFilter(QObject* watched, QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void updateExposedFrame();
        void paintEvent(QPaintEvent* event) override;
        void moveEvent(QMoveEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

        void closeEvent(QCloseEvent* event) override;

        void changeEvent(QEvent* event) override;

        void mouseMoveEvent(QMouseEvent* event) override;

        void leaveEvent(QEvent* event) override;

    public:
        explicit WorkspaceWindow(QWidget* content, const QString& title, const QString& id, QWidget* parent = nullptr);
        void addWindowAction(QMenu* menu, QAction* before);
        void showWindow();
        QRect getNormalGeometry() const;
        void setNormalGeometry(const QRect& geometry);
    };
}
#endif //NTRT_WORKSPACEWINDOW_H
