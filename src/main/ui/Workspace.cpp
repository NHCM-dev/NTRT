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
        addChartWindow();
        QTimer::singleShot(0, this, [this]
        {
            resetLayout();
            QMdiSubWindow* active = desktop->activeSubWindow();
            if (active && active->focusWidget())
                active->focusWidget()->clearFocus();
            setFocus(Qt::OtherFocusReason);
            desktop->setActiveSubWindow(nullptr);
        });
    }

    Workspace::~Workspace()
    {
        disconnect(desktop, nullptr, this, nullptr);
        desktop->viewport()->removeEventFilter(this);
        for (const WindowEntry& entry : windows)
        {
            entry.window->removeEventFilter(this);
            disconnect(entry.window, nullptr, this, nullptr);
        }
    }

    void Workspace::initDesktop()
    {
        setObjectName("workspace");
        setFocusPolicy(Qt::StrongFocus);
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        desktop = new QMdiArea(this);
        desktop->setObjectName("workspaceDesktop");
        desktop->viewport()->installEventFilter(this);
        desktop->setViewMode(QMdiArea::SubWindowView);
        desktop->setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
        desktop->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        desktop->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        desktop->setBackground(palette().brush(QPalette::Shadow));
        layout->addWidget(desktop);
        connect(desktop, &QMdiArea::subWindowActivated, this, [this](QMdiSubWindow* active)
        {
            for (const WindowEntry& entry : windows)
            {
                if (entry.window == active && entry.chart)
                    lastChartWindow = active;
                entry.window->update();
            }
        });
    }

    QMdiSubWindow* Workspace::addWindow(QWidget* content, const QString& title, const QString& id)
    {
        WorkspaceWindow* window = new WorkspaceWindow;
        window->setWidget(content);
        desktop->addSubWindow(window);
        window->setObjectName(id);
        window->setWindowTitle(title);
        window->setAttribute(Qt::WA_DeleteOnClose, true);
        window->installEventFilter(this);
        windows.push_back({window, nullptr});
        connect(window, &QObject::destroyed, this, [this, window]
        {
            std::erase_if(windows, [window](const WindowEntry& entry)
            {
                if (entry.window != window)
                    return false;
                if (entry.action)
                    delete entry.action;
                return true;
            });
            if (lastChartWindow == window)
                lastChartWindow = nullptr;
            if (instrumentWindow == window)
            {
                instrumentWindow = nullptr;
                instruments.reset();
            }
        });
        window->show();
        return window;
    }

    void Workspace::initInstrumentPanel()
    {
        instruments = std::make_unique<InstrumentListPanel>(desktop);
        instrumentWindow = addWindow(instruments->widget, "Instrument List", "instrumentList");
        addWindowAction(windows.back());
        connect(instruments.get(), &InstrumentListPanel::instrumentActivated, this,
                [this](const trtdata::InstIdentifier& identifier)
                {
                    showInstrument(identifier);
                });
    }

    QMdiSubWindow* Workspace::addChartWindow()
    {
        const int id = nextChartId++;
        ChartPanel* panel = new ChartPanel(desktop);
        QMdiSubWindow* window = addWindow(panel, QString("Chart %1 · Choose instrument").arg(id),
                                          QString("chart%1").arg(id));
        windows.back().chart = panel;
        addWindowAction(windows.back());
        connect(panel, &ChartPanel::titleChanged, this, [this, window, id](const QString& title)
        {
            window->setWindowTitle(QString("Chart %1 · %2").arg(id).arg(title));
            for (const WindowEntry& entry : windows)
            {
                if (entry.window == window && entry.action)
                    entry.action->setText(window->windowTitle());
            }
        });
        lastChartWindow = window;
        const int offset = (id - 1) % 7 * 28;
        const QRect area = desktop->viewport()->rect();
        window->setGeometry(32 + offset, 32 + offset, std::max(500, area.width() * 2 / 3),
                            std::max(360, area.height() * 3 / 4));
        return window;
    }

    void Workspace::showWindow(QMdiSubWindow* window)
    {
        if (window->isMinimized())
            window->showNormal();
        else
            window->show();
        desktop->setActiveSubWindow(window);
        window->raise();
    }

    void Workspace::createChartWindow()
    {
        showWindow(addChartWindow());
    }

    void Workspace::showInstrument(const trtdata::InstIdentifier& identifier)
    {
        QMdiSubWindow* target = lastChartWindow;
        if (!target)
        {
            for (const WindowEntry& entry : windows)
            {
                if (entry.chart)
                {
                    target = entry.window;
                    break;
                }
            }
        }
        if (!target)
            target = addChartWindow();
        for (const WindowEntry& entry : windows)
        {
            if (entry.window == target && entry.chart)
            {
                entry.chart->setInstrument(identifier);
                break;
            }
        }
        showWindow(target);
    }

    void Workspace::addWindowAction(WindowEntry& entry)
    {
        if (!windowMenu || entry.action)
            return;
        QMdiSubWindow* window = entry.window;
        entry.action = new QAction(window->windowTitle(), windowMenu);
        windowMenu->insertAction(windowActionsEnd, entry.action);
        entry.action->setCheckable(true);
        entry.action->setChecked(!window->isHidden());
        connect(entry.action, &QAction::toggled, this, [this, window](bool visible)
        {
            if (visible)
                showWindow(window);
            else
                window->hide();
        });
    }

    QMenu* Workspace::getWindowMenu()
    {
        if (!windowMenu)
            addWindowActions(new QMenu("Window", this));
        return windowMenu;
    }

    void Workspace::showInstrumentList()
    {
        if (!instrumentWindow)
            initInstrumentPanel();
        showWindow(instrumentWindow);
    }

    void Workspace::addWindowActions(QMenu* menu)
    {
        windowMenu = menu;
        windowActionsEnd = menu->addSeparator();
        for (WindowEntry& entry : windows)
            addWindowAction(entry);
        connect(menu->addAction("Stack Windows"), &QAction::triggered, desktop, &QMdiArea::cascadeSubWindows);
        connect(menu->addAction("Sort Window"), &QAction::triggered, this, [this]
        {
            sortWindows();
        });
        connect(menu->addAction("Reset Layout"), &QAction::triggered, this, [this]
        {
            resetLayout();
        });
    }

    void Workspace::sortWindows()
    {
        std::vector<WindowPlacement> placements;
        for (const WindowEntry& entry : windows)
        {
            if (!entry.window->isHidden() && !entry.window->isMinimized())
            {
                const QRect geometry = entry.window->isMaximized()
                                           ? entry.window->normalGeometry()
                                           : entry.window->geometry();
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
        const int gap = 12;
        int minimumWidth = 1;
        int minimumHeight = 1;
        for (const WindowPlacement& placement : placements)
        {
            minimumWidth = std::max(minimumWidth, placement.window->minimumSizeHint().width());
            minimumHeight = std::max(minimumHeight, placement.window->minimumSizeHint().height());
        }
        std::stable_sort(placements.begin(), placements.end(),
                         [](const WindowPlacement& left, const WindowPlacement& right)
                         {
                             const QPoint first = left.geometry.center();
                             const QPoint second = right.geometry.center();
                             return first.y() == second.y() ? first.x() < second.x() : first.y() < second.y();
                         });
        const int count = static_cast<int>(placements.size());
        const int columns = std::clamp((area.width() + gap) / (minimumWidth + gap), 1, count);
        const int rows = (count + columns - 1) / columns;
        const int width = std::max(minimumWidth, (area.width() - (columns - 1) * gap) / columns);
        const int height = std::max(minimumHeight, (area.height() - (rows - 1) * gap) / rows);
        for (int index = 0; index < count; index++)
        {
            placements[static_cast<std::size_t>(index)].window->setGeometry(
                area.left() + index % columns * (width + gap),
                area.top() + index / columns * (height + gap), width, height);
        }
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
        if (!instrumentWindow)
            initInstrumentPanel();
        instrumentWindow->showNormal();
        const int gap = 16;
        const int width = desktop->viewport()->width();
        const int height = desktop->viewport()->height();
        const int listWidth = std::max(340, instrumentWindow->minimumSizeHint().width());
        instrumentWindow->setGeometry(gap, gap, listWidth, std::max(300, height - gap * 2));
        std::vector<WindowPlacement> charts;
        for (const WindowEntry& entry : windows)
        {
            if (!entry.chart)
                continue;
            entry.window->showNormal();
            charts.push_back({entry.window, entry.window->geometry()});
        }
        arrangeWindows(std::move(charts), QRect(listWidth + gap * 2, gap, std::max(480, width - listWidth - gap * 3),
                                                std::max(340, height - gap * 2)));
        desktop->setActiveSubWindow(nullptr);
    }

    void Workspace::updateData()
    {
        if (!isVisible() || window()->isMinimized())
            return;
        if (instrumentWindow&& instrumentWindow->isVisible() && !instrumentWindow->isMinimized() && isVisible())
        instruments->update();
        for (const WindowEntry& entry : windows)
        {
            if (entry.chart && entry.window->isVisible() && !entry.window->isMinimized())
                entry.chart->updateData();
        }
    }
}
