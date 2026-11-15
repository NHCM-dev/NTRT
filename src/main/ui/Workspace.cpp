#include "Workspace.h"
#include "WorkspaceWindow.h"
#include "panels/InstrumentListPanel.h"
#include "panels/ChartPanel.h"
#include "panels/BookDepthPanel.h"
#include <QAction>
#include <QEvent>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

namespace trtui
{
    Workspace::Workspace(QWidget* parent, const trtdata::WorkspaceState* state) : QFrame(parent)
    {
        initDesktop();
        if (state)
        {
            restoreState(*state);
            return;
        }
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
            selectedWindow = nullptr;
        });
    }

    trtdata::WorkspaceState Workspace::getState() const
    {
        if (pendingState)
            return *pendingState;
        trtdata::WorkspaceState state;
        state.id = objectName();
        state.nextChartId = nextChartId;
        state.nextBookDepthId = nextBookDepthId;
        if (selectedWindow)
            state.activeWindow = selectedWindow->objectName();
        if (lastChartWindow)
            state.lastChart = lastChartWindow->objectName();
        if (instruments)
            state.instruments = instruments->getState();
        for (QMdiSubWindow* window : desktop->subWindowList(QMdiArea::StackingOrder))
        {
            for (const WindowEntry& entry : windows)
            {
                if (entry.window != window)
                    continue;
                trtdata::WorkspaceWindowState saved;
                saved.id = window->objectName();
                saved.type = trtdata::WorkspaceWindowType::Instruments;
                if (entry.chart)
                    saved.type = trtdata::WorkspaceWindowType::Chart;
                else if (entry.bookDepth)
                    saved.type = trtdata::WorkspaceWindowType::BookDepth;
                saved.geometry = entry.window->getNormalGeometry();
                saved.visible = !window->isHidden();
                if (window->isMaximized())
                    saved.mode = trtdata::WorkspaceWindowMode::Maximized;
                else if (window->isMinimized())
                    saved.mode = trtdata::WorkspaceWindowMode::Minimized;
                if (entry.chart)
                    saved.chart = entry.chart->getState();
                if (entry.bookDepth)
                    saved.bookDepth = entry.bookDepth->getState();
                state.windows.push_back(std::move(saved));
                break;
            }
        }
        return state;
    }

    void Workspace::restoreState(const trtdata::WorkspaceState& state)
    {
        restoring = true;
        for (const trtdata::WorkspaceWindowState& saved : state.windows)
        {
            switch (saved.type)
            {
                case trtdata::WorkspaceWindowType::Instruments:
                    initInstrumentPanel();
                    instruments->restoreState(state.instruments);
                    break;
                case trtdata::WorkspaceWindowType::Chart:
                    nextChartId = saved.id.mid(5).toInt();
                    addChartWindow();
                    windows.back().chart->restoreState(saved.chart);
                    break;
                case trtdata::WorkspaceWindowType::BookDepth:
                    nextBookDepthId = saved.id.mid(9).toInt();
                    addBookDepthWindow();
                    windows.back().bookDepth->restoreState(saved.bookDepth);
                    break;
            }
            windows.back().window->setObjectName(saved.id);
        }
        nextChartId = state.nextChartId;
        nextBookDepthId = state.nextBookDepthId;
        for (const WindowEntry& entry : windows)
        {
            if (entry.chart)
                nextChartId = std::max(nextChartId, entry.window->objectName().mid(5).toInt() + 1);
            if (entry.bookDepth)
                nextBookDepthId = std::max(nextBookDepthId, entry.window->objectName().mid(9).toInt() + 1);
        }
        pendingState = state;
        QTimer::singleShot(0, this, &Workspace::applyRestoredLayout);
    }

    void Workspace::applyRestoredLayout()
    {
        if (!pendingState || !isVisible())
            return;
        const trtdata::WorkspaceState state = *pendingState;
        QMdiSubWindow* active = nullptr;
        lastChartWindow = nullptr;
        for (std::size_t index = 0; index < state.windows.size(); index++)
        {
            const trtdata::WorkspaceWindowState& saved = state.windows[index];
            QMdiSubWindow* window = windows[index].window;
            window->showNormal();
            window->setGeometry(saved.geometry);
            const QRect normalGeometry = window->geometry();
            switch (saved.mode)
            {
                case trtdata::WorkspaceWindowMode::Normal:
                    break;
                case trtdata::WorkspaceWindowMode::Minimized:
                    window->showMinimized();
                    break;
                case trtdata::WorkspaceWindowMode::Maximized:
                    window->showMaximized();
                    break;
            }
            window->setVisible(saved.visible);
            window->raise();
            windows[index].window->setNormalGeometry(normalGeometry);
            if (saved.id == state.activeWindow && saved.visible)
                active = window;
        }
        desktop->setActiveSubWindow(active);
        selectedWindow = active;
        restoring = false;
        for (const WindowEntry& entry : windows)
            rememberInstrumentWindow(entry.window);
        rememberInstrumentWindow(active);
        pendingState.reset();
        for (const WindowEntry& entry : windows)
        {
            if (entry.chart && entry.window->objectName() == state.lastChart)
                lastChartWindow = entry.window;
        }
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
        desktop->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        desktop->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        desktop->setBackground(palette().brush(QPalette::Shadow));
        layout->addWidget(desktop);
        connect(desktop, &QMdiArea::subWindowActivated, this, [this](QMdiSubWindow* active)
        {
            if (active)
                selectedWindow = active;
            rememberInstrumentWindow(active);
            for (const WindowEntry& entry : windows)
            {
                if (entry.window == active && entry.chart)
                    lastChartWindow = active;
                entry.window->update();
            }
        });
    }

    WorkspaceWindow* Workspace::addWindow(QWidget* content, const QString& title, const QString& id)
    {
        WorkspaceWindow* window = new WorkspaceWindow(content, title, id);
        desktop->addSubWindow(window);
        window->installEventFilter(this);
        windows.push_back({window, nullptr, nullptr});
        if (windowMenu)
            window->addWindowAction(windowMenu, windowActionsEnd);
        connect(window, &QObject::destroyed, this, [this, window]
        {
            std::erase_if(windows, [window](const WindowEntry& entry)
            {
                return entry.window == window;
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
        connect(instruments.get(), &InstrumentListPanel::instrumentActivated, this,
                [this](const trtdata::InstIdentifier& identifier)
                {
                    showInstrument(identifier);
                });
    }

    WorkspaceWindow* Workspace::addChartWindow()
    {
        const int id = nextChartId++;
        ChartPanel* panel = new ChartPanel(desktop);
        WorkspaceWindow* window = addWindow(panel, QString("Chart %1 · Choose instrument").arg(id),
                                          QString("chart%1").arg(id));
        windows.back().chart = panel;
        rememberInstrumentWindow(window);
        connect(panel, &ChartPanel::titleChanged, this, [window, id](const QString& title)
        {
            window->setWindowTitle(QString("Chart %1 · %2").arg(id).arg(title));
        });
        lastChartWindow = window;
        const int offset = (id - 1) % 7 * 28;
        const QRect area = desktop->viewport()->rect();
        window->setGeometry(32 + offset, 32 + offset, std::max(500, area.width() * 2 / 3),
                            std::max(360, area.height() * 3 / 4));
        return window;
    }

    WorkspaceWindow* Workspace::addBookDepthWindow()
    {
        const int id = nextBookDepthId++;
        BookDepthPanel* panel = new BookDepthPanel(desktop);
        WorkspaceWindow* window = addWindow(panel, QString("BookDepth %1 · Choose instrument").arg(id),
                                         QString("bookDepth%1").arg(id));
        windows.back().bookDepth = panel;
        rememberInstrumentWindow(window);
        connect(panel, &BookDepthPanel::titleChanged, this, [window, id](const QString& title)
        {
            window->setWindowTitle(QString("BookDepth %1 · %2").arg(id).arg(title));
        });
        window->setGeometry(48 + (id - 1) % 7 * 24, 48 + (id - 1) % 7 * 24, 440, 660);
        return window;
    }

    void Workspace::createBookDepthWindow()
    {
        addBookDepthWindow()->showWindow();
    }

    void Workspace::createChartWindow()
    {
        addChartWindow()->showWindow();
    }

    void Workspace::rememberInstrumentWindow(QMdiSubWindow* window)
    {
        if (restoring || !window)
            return;
        for (const WindowEntry& entry : windows)
        {
            if (entry.window != window || (!entry.chart && !entry.bookDepth))
                continue;
            std::erase_if(instRequiredWindow, [window](const QPointer<QMdiSubWindow>& value)
            {
                return !value || value == window;
            });
            instRequiredWindow.emplace_back(window);
            break;
        }
    }

    void Workspace::showInstrument(const trtdata::InstIdentifier& identifier)
    {
        WindowEntry* chart = nullptr;
        WindowEntry* book = nullptr;
        // A frozen target set prevents activation during apply from changing this dispatch.
        for (std::vector<QPointer<QMdiSubWindow>>::const_reverse_iterator iterator = instRequiredWindow.crbegin();
             iterator != instRequiredWindow.crend(); iterator++)
        {
            if (!*iterator || (*iterator)->isHidden() || (*iterator)->isMinimized())
                continue;
            for (WindowEntry& entry : windows)
            {
                if (entry.window != *iterator)
                    continue;
                if (entry.chart && (!chart || (!entry.chart->getInstrument() && chart->chart->getInstrument())))
                    chart = &entry;
                if (entry.bookDepth && (!book || (!entry.bookDepth->getState().instrument && book->bookDepth->getState().instrument)))
                    book = &entry;
            }
        }
        if (chart)
            chart->chart->setInstrument(identifier);
        if (book)
            book->bookDepth->setInstrument(identifier);
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
        instrumentWindow->showWindow();
    }

    void Workspace::addWindowActions(QMenu* menu)
    {
        windowMenu = menu;
        windowActionsEnd = menu->addSeparator();
        for (WindowEntry& entry : windows)
            entry.window->addWindowAction(menu, windowActionsEnd);
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
                const QRect geometry = entry.window->getNormalGeometry();
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
                selectedWindow = nullptr;
                return true;
            }
        }
        if (event->type() == QEvent::Hide && selectedWindow == watched && selectedWindow->isHidden())
            selectedWindow = nullptr;
        return QFrame::eventFilter(watched, event);
    }

    void Workspace::showEvent(QShowEvent* event)
    {
        const QPointer<QMdiSubWindow> selected = selectedWindow;
        QFrame::showEvent(event);
        if (pendingState)
            QTimer::singleShot(0, this, &Workspace::applyRestoredLayout);
        else if (!restoring)
        {
            QTimer::singleShot(0, this, [this, selected]
            {
                desktop->setActiveSubWindow(selected);
                selectedWindow = selected;
            });
        }
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
            if (!entry.chart && !entry.bookDepth)
                continue;
            entry.window->showNormal();
            charts.push_back({entry.window, entry.window->geometry()});
        }
        arrangeWindows(std::move(charts), QRect(listWidth + gap * 2, gap, std::max(480, width - listWidth - gap * 3),
                                                std::max(340, height - gap * 2)));
        desktop->setActiveSubWindow(nullptr);
        selectedWindow = nullptr;
    }

    void Workspace::updateData()
    {
        if (!isVisible() || window()->isMinimized())
            return;
        if (instrumentWindow && instrumentWindow->isVisible() && !instrumentWindow->isMinimized())
            instruments->update();
        for (const WindowEntry& entry : windows)
        {
            if (entry.chart && entry.window->isVisible() && !entry.window->isMinimized())
                entry.chart->updateData();
            if (entry.bookDepth && entry.window->isVisible() && !entry.window->isMinimized())
                entry.bookDepth->updateData();
        }
    }
}
