//
// Created by NHCM on 2026/11/6.
//

#include "MainWindow.h"
#include <QAction>
#include <QMessageBox>
#include <QMenuBar>
#include <QButtonGroup>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <functional>

#include "main/NTRT.h"
#include "main/Ver.h"
#include "pages/MarketPage.h"

namespace trtui
{
    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
    {
        initLayouts();
        initMenuBar();
        initNavigation();
        initWidgets();
        initPages();
        init200msSchedules();
        resize(1000, 500);
    }

    void MainWindow::initLayouts()
    {
        root = new QWidget(this);
        root->setLayout(rootLayout = new QHBoxLayout);

        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        setCentralWidget(root);
        mainPages = new QStackedWidget(root);
    }

    void MainWindow::initMenuBar()
    {
        menuBar = new QMenuBar(this);

        fileMenu = new QMenu("File", menuBar);
        QAction* exit = fileMenu->addAction("Exit");
        connect(exit, &QAction::triggered, this, []
        {
            ntrt::quitApp();
        });

        aboutMenu = new QMenu("About", menuBar);
        std::string aboutActionStr = "About " + NTRT_NAME;
        QAction* about = aboutMenu->addAction(QString::fromStdString(aboutActionStr));
        aboutMenu->addAction(about);
        std::string aboutString = "About "+ NTRT_NAME + "\n\n" +
            NTRT_DESCRIPTION + "\n" +
            "Version " + NTRT_VERSION + " " + NTRT_RELEASE + " " + NTRT_BUILD + "\n\n" +
            "Made by " + NTRT_AUTHOR + "\n\n" +
                "Build date " + NTRT_DATE + "\n";
        connect(about, &QAction::triggered, this, [this, aboutString]
        {
            QMessageBox::about(
                this,
                "About",
                QString::fromStdString(aboutString)
                );
        });

        menuBar->addMenu(fileMenu);
        menuBar->addMenu(aboutMenu);

        setMenuBar(menuBar);
    }

    void MainWindow::initNavigation()
    {
        navigationBar = new QWidget(root);
        navigationBar->setFixedWidth(100);
        QVBoxLayout* layout = new QVBoxLayout(navigationBar);
        navigationGroup = new QButtonGroup(navigationBar);

        navigationGroup->setExclusive(true);

        const std::function<void(const QString&, Navigation)> addButton = [&](const QString& title, Navigation id)
        {
            QPushButton* button = new QPushButton(title, navigationBar);

            button->setCheckable(true);

            const int index = static_cast<int>(id);

            navigationGroup->addButton(button, index);
            layout->addWidget(button);

            connect(button, &QPushButton::clicked, this, [this, id]
            {
                mainPages->setCurrentWidget(widgets.at(id)->widget);
            });
        };

        addButton("Market", Navigation::Market);
        addButton("Trade", Navigation::Trade);
        addButton("Account", Navigation::Account);
        addButton("Research", Navigation::Research);

        layout->addStretch();

        addButton("Settings", Navigation::Settings);

        QPushButton* exitButton = new QPushButton("Exit", navigationBar);
        navigationGroup->addButton(exitButton, -999);
        layout->addWidget(exitButton);
        connect(exitButton, &QPushButton::clicked, this, []
        {
            ntrt::quitApp();
        });
    }

    void MainWindow::initWidgets()
    {
        rootLayout->addWidget(navigationBar);
        rootLayout->addWidget(mainPages, 1);
    }

    void MainWindow::initPages()
    {
        std::unique_ptr<MarketPage> marketPage = std::make_unique<MarketPage>(mainPages);
        mainPages->addWidget(marketPage->widget);
        mainPages->setCurrentWidget(marketPage->widget);
        widgets.emplace(Navigation::Market, std::move(marketPage));

        for (QAbstractButton* button : navigationGroup->buttons())
        {
            const int id = navigationGroup->id(button);
            if (button->isCheckable())
            {
                button->setEnabled(widgets.contains(static_cast<Navigation>(id)));
            }
        }
        navigationGroup->button(static_cast<int>(Navigation::Market))->setChecked(true);
    }

    void MainWindow::init200msSchedules()
    {
        connect(timer200ms, &QTimer::timeout, this, [this] {
            updateData200ms();
        });

        updateData200ms();
        timer200ms->start(200);
    }

    void MainWindow::updateData200ms()
    {
        for (const auto& [key, value] : widgets)
        {
            value->update();
        }
    }
}
