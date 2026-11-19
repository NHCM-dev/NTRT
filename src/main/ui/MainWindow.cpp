#include <QTimeZone>
#include <QApplication>
#include <QCloseEvent>
#include "MainWindow.h"
#include "dialog/ListManagerDialog.h"
#include "main/util/QtUtil.h"
#include "Workspace.h"
#include "dialog/SettingsDialog.h"
#include "dialog/InstrumentDialog.h"
#include <QTabWidget>
#include <QTabBar>
#include <QToolButton>
#include <QInputDialog>
#include <QLineEdit>
#include "main/NTRT.h"
#include "main/Ver.h"
#include <QAction>
#include <QDateTime>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QScreen>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <functional>
#include <algorithm>

namespace trtui
{
    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
    {
        initLayouts();
        initMenuBar();
        initWorkspaceTabs();
        initStatusBar();
        initWorkspacePersistence();
    }

    void MainWindow::initWorkspacePersistence()
    {
        const trtdata::WorkspaceConfig& config = ntrt::configManager->getWorkspaceManager()->getConfig();
        if (config.mainGeometry.isEmpty() || !restoreGeometry(config.mainGeometry))
            initWindowGeometry();
        nextWorkspaceId = config.nextWorkspaceId;
        for (const trtdata::WorkspaceState& state : config.workspaces)
        {
            createWorkspace(&state);
            nextWorkspaceId = std::max(nextWorkspaceId, state.id.mid(9).toInt() + 1);
        }
        if (workspaces->count() == 0)
            createWorkspace();
        else
            workspaces->setCurrentIndex(config.currentWorkspace);
        QTimer* saveTimer = new QTimer(this);
        connect(saveTimer, &QTimer::timeout, this, [this]
        {
            if (!closing && !ntrt::restarting)
                saveWorkspaceState();
        });
        saveTimer->start(2000);
        connect(qApp, &QCoreApplication::aboutToQuit, this, [this]
        {
            if (!closing && !ntrt::restarting)
                saveWorkspaceState();
        });
    }

    bool MainWindow::saveWorkspaceState(bool notify)
    {
        trtdata::WorkspaceConfig config;
        config.mainGeometry = saveGeometry();
        config.currentWorkspace = workspaces->currentIndex();
        config.nextWorkspaceId = nextWorkspaceId;
        for (int index = 0; index < workspaces->count(); index++)
        {
            const Workspace* workspace = static_cast<const Workspace*>(workspaces->widget(index));
            trtdata::WorkspaceState state = workspace->getState();
            state.name = workspaces->tabText(index);
            config.workspaces.push_back(std::move(state));
        }
        const bool saved = ntrt::configManager->getWorkspaceManager()->saveConfig(config);
        if (!saved || notify)
            statusBar()->showMessage(saved ? "Workspace layout saved" : "Unable to save workspace layout; check workspaces.json and file permissions", 5000);
        return saved;
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        if (!ntrt::restarting)
            saveWorkspaceState();
        closing = true;
        QMainWindow::closeEvent(event);
        if (!event->isAccepted())
            closing = false;
    }

    void MainWindow::initWindowGeometry()
    {
        resize(QSize(1400, 700).boundedTo(screen()->availableGeometry().size()));
        QTimer::singleShot(0, this, [this]
        {
            trtutils::centerOnScreen(this);
        });
    }

    void MainWindow::initLayouts()
    {
        workspaces = new QTabWidget(this);
        workspaces->setObjectName("workspaceTabs");
        workspaces->setDocumentMode(true);
        workspaces->setMovable(true);
        workspaces->setTabsClosable(true);
        workspaces->setUsesScrollButtons(true);
        workspaces->tabBar()->setElideMode(Qt::ElideRight);
        workspaces->tabBar()->setDrawBase(false);
        setCentralWidget(workspaces);
    }

    void MainWindow::initMenuBar()
    {
        menuBar()->setMouseTracking(true);
        QMenu* file = menuBar()->addMenu("File");
        connect(file->addAction("Save Workspace Layout"), &QAction::triggered, this, [this]
        {
            saveWorkspaceState(true);
        });
        file->addSeparator();
        QAction* preferences = file->addAction("Settings…");
        connect(preferences, &QAction::triggered, this, [this]
        {
            showSettings();
        });
        file->addSeparator();
        connect(file->addAction("Exit"), &QAction::triggered, this, []
        {
            ntrt::quitApp();
        });
        QMenu* workspaceMenu = menuBar()->addMenu("Workspace");
        connect(workspaceMenu->addAction("New Workspace"), &QAction::triggered, this, [this]
        {
            createWorkspace();
        });
        connect(workspaceMenu->addAction("Rename Current…"), &QAction::triggered, this, [this]
        {
            renameWorkspace(workspaces->currentIndex());
        });
        connect(workspaceMenu->addAction("Close Current"), &QAction::triggered, this, [this]
        {
            closeWorkspace(workspaces->currentIndex());
        });
        workspaceMenu->addSeparator();
        connect(workspaceMenu->addAction("Manage Workspace…"), &QAction::triggered, this, &MainWindow::showManageWorkspaceDialog);
        QAction* newChart = new QAction("New Chart", this);
        connect(newChart, &QAction::triggered, this, [this]
        {
            if (getCurrentWorkspace())
                getCurrentWorkspace()->createChartWindow();
        });
        QMenu* elements = menuBar()->addMenu("Elements");
        elementsMenu = elements;
        elements->addAction(newChart);
        connect(elements->addAction("New BookDepth Window"), &QAction::triggered, this, [this]
        {
            if (getCurrentWorkspace())
                getCurrentWorkspace()->createBookDepthWindow();
        });
        connect(elements->addAction("Instrument List"), &QAction::triggered, this, [this]
        {
            if (getCurrentWorkspace())
                getCurrentWorkspace()->showInstrumentList();
        });
        elements->addSeparator();
        connect(elements->addAction("Account Balances"), &QAction::triggered, this, [this]
        {
            if (getCurrentWorkspace()) getCurrentWorkspace()->createAccountWindow(trtdata::WorkspaceWindowType::AccountBalance);
        });
        connect(elements->addAction("Account Positions"), &QAction::triggered, this, [this]
        {
            if (getCurrentWorkspace()) getCurrentWorkspace()->createAccountWindow(trtdata::WorkspaceWindowType::AccountPositions);
        });
        connect(elements->addAction("Current Orders"), &QAction::triggered, this, [this]
        {
            if (getCurrentWorkspace()) getCurrentWorkspace()->createAccountWindow(trtdata::WorkspaceWindowType::AccountOrders);
        });
        connect(elements->addAction("Order History"), &QAction::triggered, this, [this]
        {
            if (getCurrentWorkspace()) getCurrentWorkspace()->createAccountWindow(trtdata::WorkspaceWindowType::AccountOrderHistory);
        });
        connect(elements->addAction("Position History"), &QAction::triggered, this, [this]
        {
            if (getCurrentWorkspace()) getCurrentWorkspace()->createAccountWindow(trtdata::WorkspaceWindowType::AccountPositionHistory);
        });
        elements->addSeparator();
        connect(elements->addAction("Add Instruments…"), &QAction::triggered, this, [this]
        {
            InstrumentDialog dialog(InstrumentDialog::Mode::Add, this);
            dialog.exec();
        });
        connect(elements->addAction("Manage Instruments…"), &QAction::triggered, this, [this]
        {
            InstrumentDialog dialog(InstrumentDialog::Mode::Manage, this);
            dialog.exec();
        });
        windowAction = menuBar()->addAction("Window");
        QMenu* about = menuBar()->addMenu("About");
        connect(about->addAction("About NTRT"), &QAction::triggered, this, [this]
        {
            ntrt::displayAbout(this);
        });
    }

    void MainWindow::initWorkspaceTabs()
    {
        QWidget* corner = new QWidget(workspaces);
        QHBoxLayout* cornerLayout = new QHBoxLayout(corner);
        cornerLayout->setContentsMargins(0, 0, 0, 0);
        cornerLayout->setSpacing(0);
        QToolButton* add = new QToolButton(corner);
        add->setObjectName("newWorkspaceButton");
        add->setText("+");
        add->setFixedSize(28, 28);
        add->setToolTip("New Workspace");
        cornerLayout->addWidget(add);
        workspaces->setCornerWidget(corner, Qt::TopRightCorner);
        connect(add, &QToolButton::clicked, this, [this]
        {
            createWorkspace();
        });
        connect(workspaces, &QTabWidget::currentChanged, this, [this](int)
        {
            Workspace* current = getCurrentWorkspace();
            windowAction->setMenu(current ? current->getWindowMenu() : nullptr);
            if (current)
                current->updateData();
        });
        connect(workspaces, &QTabWidget::tabCloseRequested, this, [this](int index)
        {
            closeWorkspace(index);
        });
        connect(workspaces->tabBar(), &QTabBar::tabBarDoubleClicked, this, [this](int index)
        {
            if (index < 0)
                createWorkspace();
            else
                renameWorkspace(index);
        });
        workspaces->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(workspaces->tabBar(), &QTabBar::customContextMenuRequested, this, &MainWindow::showWorkspaceMenu);
    }

    Workspace* MainWindow::getCurrentWorkspace() const
    {
        return static_cast<Workspace*>(workspaces->currentWidget());
    }

    void MainWindow::createWorkspace(const trtdata::WorkspaceState* state)
    {
        Workspace* workspace = new Workspace(workspaces, state);
        workspace->setObjectName(state ? state->id : QString("workspace%1").arg(nextWorkspaceId));
        const QString name = state ? state->name : QString("Workspace %1").arg(nextWorkspaceId++);
        const int index = workspaces->addTab(workspace, name);
        QToolButton* close = new QToolButton(workspaces->tabBar());
        close->setObjectName("workspaceTabClose");
        close->setText("×");
        close->setFixedSize(18, 18);
        close->setToolTip("Close Workspace");
        workspaces->tabBar()->setTabButton(index, QTabBar::RightSide, close);
        connect(close, &QToolButton::clicked, this, [this, workspace]
        {
            closeWorkspace(workspaces->indexOf(workspace));
        });
        workspaces->setCurrentIndex(index);
    }

    void MainWindow::renameWorkspace(int index)
    {
        if (index < 0)
            return;
        bool accepted = false;
        const QString name = QInputDialog::getText(this, "Rename Workspace", "Name", QLineEdit::Normal,
                                                   workspaces->tabText(index), &accepted).trimmed();
        if (accepted && !name.isEmpty())
            workspaces->setTabText(index, name);
    }

    void MainWindow::closeWorkspace(int index)
    {
        if (index < 0)
            return;
        QWidget* removed = workspaces->widget(index);
        if (workspaces->count() == 1)
            createWorkspace();
        workspaces->removeTab(workspaces->indexOf(removed));
        removed->deleteLater();
    }

    void MainWindow::showManageWorkspaceDialog()
    {
        ListManagerDialog dialog("Manage Workspace", [this]
        {
            ListManagerDialog::Items items;
            for (int index = 0; index < workspaces->count(); index++)
                items.append({workspaces->widget(index)->objectName(), workspaces->tabText(index)});
            return items;
        }, this);
        dialog.addAction("New Workspace", [this](const QStringList&) { createWorkspace(); });
        dialog.addAction("Rename…", [this](const QStringList& ids)
        {
            for (int index = 0; index < workspaces->count(); index++)
                if (workspaces->widget(index)->objectName() == ids.first()) { renameWorkspace(index); break; }
        }, 1, 1);
        dialog.addAction("Close Selected", [this](const QStringList& ids)
        {
            std::vector<QPointer<QWidget>> targets;
            for (int index = 0; index < workspaces->count(); index++)
                if (ids.contains(workspaces->widget(index)->objectName())) targets.push_back(workspaces->widget(index));
            for (const QPointer<QWidget>& target : targets) if (target) closeWorkspace(workspaces->indexOf(target));
        }, 1);
        dialog.exec();
    }

    void MainWindow::showWorkspaceMenu(const QPoint& position)
    {
        QTabBar* bar = workspaces->tabBar();
        const int index = bar->tabAt(position);
        QMenu menu(this);
        connect(menu.addAction("New Workspace"), &QAction::triggered, this, [this]
        {
            createWorkspace();
        });
        if (index >= 0)
        {
            workspaces->setCurrentIndex(index);
            connect(menu.addAction("Rename…"), &QAction::triggered, this, [this, index]
            {
                renameWorkspace(index);
            });
            QAction* left = menu.addAction("Move Left");
            left->setEnabled(index > 0);
            connect(left, &QAction::triggered, this, [bar, index]
            {
                bar->moveTab(index, index - 1);
            });
            QAction* right = menu.addAction("Move Right");
            right->setEnabled(index + 1 < workspaces->count());
            connect(right, &QAction::triggered, this, [bar, index]
            {
                bar->moveTab(index, index + 1);
            });
            connect(menu.addAction("Close Workspace"), &QAction::triggered, this, [this, index]
            {
                closeWorkspace(index);
            });

        }
        menu.exec(bar->mapToGlobal(position));
    }

    void MainWindow::showSettings()
    {
        if (!settings)
            settings = new SettingsDialog(this);
        settings->show();
        settings->raise();
        settings->activateWindow();
    }

    void MainWindow::initStatusBar()
    {
        QStatusBar* bar = statusBar();

        QWidget* container = new QWidget(bar);
        QHBoxLayout* layout = new QHBoxLayout(container);

        layout->setContentsMargins(8, 0, 8, 0);

        QLabel* software = new QLabel(
            QString("%1 %2").arg(
                QString::fromStdString(NTRT_NAME), QString::fromStdString(NTRT_FULL_VER))
        );

        layout->addWidget(software);
        software->setMinimumWidth(software->sizeHint().width());

        // Left
        bar->addWidget(container, 1);

        // Right
        QLabel* clock = new QLabel();
        bar->addPermanentWidget(clock);

        // Time update
        const std::function<void()> updateClock = [clock]()
        {
            clock->setText(QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone(QByteArray::fromStdString(ntrt::configManager->getDisplayTimeZone()))).toString("yyyy-MM-dd HH:mm:ss t (UTCtt)"));
            clock->setMinimumWidth(clock->sizeHint().width());
        };

        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, updateClock);

        updateClock();
        timer->start(1000);
    }

    void MainWindow::changeEvent(QEvent* event)
    {
        QMainWindow::changeEvent(event);
        if (event->type() == QEvent::WindowStateChange && workspaces && !isMinimized())
            if (Workspace* workspace = getCurrentWorkspace())
                workspace->updateData();
    }
}
