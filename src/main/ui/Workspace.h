#ifndef NTRT_WORKSPACE_H
#define NTRT_WORKSPACE_H

#include <QFrame>
#include <memory>
#include <vector>

class QAction;
class QMenu;
class QMdiArea;
class QMdiSubWindow;

namespace trtui
{
    class InstrumentListPanel;
    class Workspace final : public QFrame
    {
        struct WindowEntry
        {
            QMdiSubWindow* window = nullptr;
            QAction* action = nullptr;
        };
        QMdiArea* desktop = nullptr;
        QMdiSubWindow* instrumentWindow = nullptr;
        QMdiSubWindow* chartWindow = nullptr;
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
        void initChartPanel();
        QMdiSubWindow* addWindow(QWidget* content, const QString& title, const QString& id);
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;

    public:
        explicit Workspace(QWidget* parent = nullptr);
        ~Workspace() override;
        void addWindowActions(QMenu* menu);
        void resetLayout();
        void updateData();
    };
}
#endif //NTRT_WORKSPACE_H
