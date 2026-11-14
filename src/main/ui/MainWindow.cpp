#include "MainWindow.h"
#include "Workspace.h"
#include "dialog/SettingsDialog.h"
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

namespace trtui
{
    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
    {
        initLayouts();
        initMenuBar();
        initNavigation();
        initStatusBar();
        init200msSchedules();
        initWindowGeometry();
    }

    void MainWindow::initWindowGeometry()
    {
        resize(1400, 700);
        QTimer::singleShot(0, this, [this]
        {
            QRect geometry = frameGeometry();
            geometry.moveCenter(screen()->availableGeometry().center());
            move(geometry.topLeft());
        });
    }

    void MainWindow::initLayouts()
    {
        root = new QWidget(this);
        rootLayout = new QHBoxLayout(root);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);
        setCentralWidget(root);
        workspace = new Workspace(root);
        rootLayout->addWidget(workspace, 1);
    }

    void MainWindow::initMenuBar()
    {
        QMenu* file = menuBar()->addMenu("File");
        QAction* preferences = file->addAction("Settings...");
        preferences->setShortcut(QKeySequence("Ctrl+,"));
        connect(preferences, &QAction::triggered, this, [this] { showSettings(); });
        file->addSeparator();
        connect(file->addAction("Exit"), &QAction::triggered, this, [] { ntrt::quitApp(); });
        QMenu* windows = menuBar()->addMenu("Window");
        workspace->addWindowActions(windows);
        QMenu* about = menuBar()->addMenu("About");
        connect(about->addAction("About NTRT"), &QAction::triggered, this, [this] { ntrt::displayAbout(this); });
    }

    void MainWindow::initNavigation()
    {
        QWidget* navigation = new QWidget(root);
        navigation->setObjectName("workspaceNavigation");
        navigation->setFixedWidth(100);
        QVBoxLayout* layout = new QVBoxLayout(navigation);
        QPushButton* market = new QPushButton("Market", navigation);
        market->setCheckable(true);
        market->setChecked(true);
        layout->addWidget(market);
        for (const QString& name : {QString("Data"), QString("Trade"), QString("Account"), QString("Research"), QString("AI")})
        {
            QPushButton* button = new QPushButton(name, navigation);
            button->setEnabled(false);
            layout->addWidget(button);
        }
        layout->addStretch();
        QPushButton* preferences = new QPushButton("Settings", navigation);
        connect(preferences, &QPushButton::clicked, this, [this] { showSettings(); });
        layout->addWidget(preferences);
        QPushButton* exit = new QPushButton("Exit", navigation);
        connect(exit, &QPushButton::clicked, this, [] { ntrt::quitApp(); });
        layout->addWidget(exit);
        rootLayout->insertWidget(0, navigation);
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
            clock->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss t (UTCtt)"));
            clock->setMinimumWidth(clock->sizeHint().width());
        };

        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, updateClock);

        updateClock();
        timer->start(1000);
    }

    void MainWindow::init200msSchedules()
    {
        timer200ms = new QTimer(this);
        connect(timer200ms, &QTimer::timeout, this, [this] { workspace->updateData(); });
        workspace->updateData();
        timer200ms->start(200);
    }
}
