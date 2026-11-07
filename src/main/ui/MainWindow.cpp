#include "MainWindow.h"
#include <QAction>
#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QTableWidget>

#include "main/NTRT.h"
#include "main/Ver.h"

namespace ntrt
{
    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
    {
        initMenuBar();
    }

    void MainWindow::initMenuBar()
    {
        menuBar = new QMenuBar(this);

        fileMenu = new QMenu(QStringLiteral("File"), menuBar);
        QAction* exit = fileMenu->addAction(QStringLiteral("Exit"));
        connect(exit, &QAction::triggered, this, []
        {
            quitApp();
        });

        aboutMenu = new QMenu(QStringLiteral("About"), menuBar);
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
                QStringLiteral("About"),
                QString::fromStdString(aboutString)
                );
        });

        menuBar->addMenu(fileMenu);
        menuBar->addMenu(aboutMenu);
    }
}
