#include "Workspace.h"
#include "WorkspaceWindow.h"
#include "panels/InstrumentListPanel.h"
#include "panels/ChartPanel.h"
#include <QAction>
#include <QEvent>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMouseEvent>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

namespace trtui
{
    Workspace::Workspace(QWidget* parent) : QFrame(parent)
    {
        initDesktop();
        initInstrumentPanel();
        initChartPanel();
        QTimer::singleShot(0, this, [this]
        {
            resetLayout();
            QMdiSubWindow* active = desktop->activeSubWindow();
            if (active && active->focusWidget())
                active->focusWidget()->clearFocus();
            desktop->setActiveSubWindow(nullptr);
        });
    }

    Workspace::~Workspace() = default;

    void Workspace::initDesktop()
    {
        setObjectName("workspace");
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        desktop = new QMdiArea(this);
        desktop->setObjectName("workspaceDesktop");
        desktop->viewport()->installEventFilter(this);
        desktop->setViewMode(QMdiArea::SubWindowView);
        desktop->setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
        desktop->setBackground(palette().brush(QPalette::Shadow));
        layout->addWidget(desktop);
        connect(desktop, &QMdiArea::subWindowActivated, this, [this](QMdiSubWindow*)
        {
            for (const WindowEntry& entry : windows)
                entry.window->update();
        });
    }

    QMdiSubWindow* Workspace::addWindow(QWidget* content, const QString& title, const QString& id)
    {
        WorkspaceWindow* window = new WorkspaceWindow(desktop);
        window->setWidget(content);
        desktop->addSubWindow(window);
        window->setObjectName(id);
        window->setWindowTitle(title);
        window->setAttribute(Qt::WA_DeleteOnClose, false);
        window->installEventFilter(this);
        windows.push_back({window, nullptr});
        window->show();
        return window;
    }

    void Workspace::initInstrumentPanel()
    {
        instruments = std::make_unique<InstrumentListPanel>(desktop);
        instrumentWindow = addWindow(instruments->widget, "Instrument List", "instrumentList");
    }

    void Workspace::initChartPanel()
    {
        chartWindow = addWindow(new ChartPanel(desktop), "Chart", "chart");
    }

    void Workspace::addWindowActions(QMenu* menu)
    {
        for (WindowEntry& entry : windows)
        {
            QMdiSubWindow* window = entry.window;
            entry.action = menu->addAction(window->windowTitle());
            entry.action->setCheckable(true);
            entry.action->setChecked(!window->isHidden());
            connect(entry.action, &QAction::toggled, this, [this, window](bool visible)
            {
                if (visible)
                {
                    window->showNormal();
                    desktop->setActiveSubWindow(window);
                    window->raise();
                }
                else
                    window->hide();
            });
        }
        menu->addSeparator();
        connect(menu->addAction("Stack Windows"), &QAction::triggered, desktop, &QMdiArea::cascadeSubWindows);
        connect(menu->addAction("Sort Window"), &QAction::triggered, this, [this] { sortWindows(); });
        connect(menu->addAction("Reset Layout"), &QAction::triggered, this, [this] { resetLayout(); });
    }

    void Workspace::sortWindows()
    {
        std::vector<WindowPlacement> placements;
        for (const WindowEntry& entry : windows)
        {
            if (!entry.window->isHidden() && !entry.window->isMinimized())
            {
                const QRect geometry = entry.window->isMaximized() ? entry.window->normalGeometry() : entry.window->geometry();
                placements.push_back({entry.window, geometry});
            }
        }
        // Capture all positions before restoring or moving any window.
        for (const WindowPlacement& placement : placements)
            placement.window->showNormal();
        arrangeWindows(std::move(placements), desktop->viewport()->rect().adjusted(12, 12, -12, -12));
    }

    void Workspace::arrangeWindows(std::vector<WindowPlacement> placements, const QRect& area)
    {
        if (placements.empty())
            return;
        if (placements.size() == 1)
        {
            placements.front().window->setGeometry(area);
            return;
        }
        int minX = placements.front().geometry.center().x();
        int maxX = minX;
        int minY = placements.front().geometry.center().y();
        int maxY = minY;
        for (const WindowPlacement& placement : placements)
        {
            minX = std::min(minX, placement.geometry.center().x());
            maxX = std::max(maxX, placement.geometry.center().x());
            minY = std::min(minY, placement.geometry.center().y());
            maxY = std::max(maxY, placement.geometry.center().y());
        }
        const bool horizontal = maxX - minX == maxY - minY ? area.width() >= area.height() : maxX - minX > maxY - minY;
        std::stable_sort(placements.begin(), placements.end(), [horizontal](const WindowPlacement& left, const WindowPlacement& right)
        {
            const QPoint first = left.geometry.center();
            const QPoint second = right.geometry.center();
            if (horizontal)
                return first.x() == second.x() ? first.y() < second.y() : first.x() < second.x();
            return first.y() == second.y() ? first.x() < second.x() : first.y() < second.y();
        });
        const std::size_t middle = placements.size() / 2;
        const int gap = 12;
        QRect firstArea = area;
        QRect secondArea = area;
        if (horizontal)
        {
            const int split = static_cast<int>((area.width() - gap) * middle / placements.size());
            firstArea.setWidth(split);
            secondArea.setLeft(area.left() + split + gap);
        }
        else
        {
            const int split = static_cast<int>((area.height() - gap) * middle / placements.size());
            firstArea.setHeight(split);
            secondArea.setTop(area.top() + split + gap);
        }
        arrangeWindows(std::vector<WindowPlacement>(placements.begin(), placements.begin() + middle), firstArea);
        arrangeWindows(std::vector<WindowPlacement>(placements.begin() + middle, placements.end()), secondArea);
    }

    bool Workspace::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched == desktop->viewport() && event->type() == QEvent::MouseButtonPress)
        {
            const QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton && !desktop->viewport()->childAt(mouse->position().toPoint()))
            {
                QMdiSubWindow* active = desktop->activeSubWindow();
                if (active && active->focusWidget())
                    active->focusWidget()->clearFocus();
                desktop->setActiveSubWindow(nullptr);
                return true;
            }
        }
        if (event->type() == QEvent::Show || event->type() == QEvent::Hide)
        {
            for (const WindowEntry& entry : windows)
            {
                if (entry.window == watched && entry.action)
                {
                    const QSignalBlocker blocker(entry.action);
                    entry.action->setChecked(!entry.window->isHidden());
                    break;
                }
            }
        }
        return QFrame::eventFilter(watched, event);
    }

    void Workspace::changeEvent(QEvent* event)
    {
        QFrame::changeEvent(event);
        if (event->type() == QEvent::PaletteChange && desktop)
            desktop->setBackground(palette().brush(QPalette::Shadow));
    }

    void Workspace::resetLayout()
    {
        instrumentWindow->showNormal();
        chartWindow->showNormal();
        const int gap = 16;
        const int width = desktop->viewport()->width();
        const int height = desktop->viewport()->height();
        const int listWidth = std::max(340, instrumentWindow->minimumSizeHint().width());
        instrumentWindow->setGeometry(gap, gap, listWidth, std::max(300, height - gap * 2));
        chartWindow->setGeometry(listWidth + gap * 2, gap, std::max(300, width - listWidth - gap * 3), std::max(260, height - gap * 3));
        desktop->setActiveSubWindow(instrumentWindow);
    }

    void Workspace::updateData()
    {
        if (instrumentWindow->isVisible() && !instrumentWindow->isMinimized() && isVisible())
            instruments->update();
    }
}
