//
// Created by NHCM on 2026/11/6.
//

#include "NTRT.h"
#include "ui/MainWindow.h"
#include <QApplication>
#include <QString>

#include "util/FileUtil.h"

int main(int argc, char** argv)
{
    for (int i = 0; i < argc; i++)
    {
        ntrt::args.emplace_back(argv[i]);
    }

    int qtArgc = 1;
    char* appName = argv[0];
    char* qtArgv[] = {appName, nullptr};

    ntrt::app = new QApplication(qtArgc, qtArgv);

    return runQtApp();
}

int runQtApp()
{
    initQtApp();
    initQtStyles();
    initQtIcon();

    ntrt::mainWindow = new trtui::MainWindow();
    ntrt::mainWindow->show();

    int exitCode = QApplication::exec();
    terminateHook();
    cleanMem();
    return exitCode;
}

void ntrt::quitApp()
{
    QApplication::quit();
}

void cleanMem()
{
    delete ntrt::mainWindow;
    ntrt::mainWindow = nullptr;

    delete ntrt::app;
    ntrt::app = nullptr;
}

void terminateHook()
{
    // do something
}

void initQtApp()
{
    QApplication::setApplicationName("NTRT");
    QApplication::setOrganizationName("NTRT");
}

void initQtStyles()
{
    QApplication::setStyle("Fusion");

    QByteArray bytes = trtutils::readAllBytes("./res/style.qss", QIODevice::Text);

    ntrt::app->setStyleSheet(QString::fromUtf8(bytes));
}

void initQtIcon()
{
    QIcon icon = QIcon("./res/icon.ico");
    QApplication::setWindowIcon(icon);
}
