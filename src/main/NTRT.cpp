//
// Created by NHCM on 2026/11/6.
//

#include "NTRT.h"
#include "ui/MainWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QString>
#include <QtNetwork/QNetworkAccessManager>

#include "Ver.h"
#include "network/RestAPITemplate.h"
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

    ntrt::app = std::make_shared<QApplication>(qtArgc, qtArgv);
    ntrt::netManager = new QNetworkAccessManager();
    ntrt::netManager->setTransferTimeout(10000);

    return runQtApp();
}

namespace ntrt
{
    void quitApp()
    {
        QApplication::quit();
    }

    void displayAbout(QWidget* parent)
    {
        std::string aboutString = "About "+ NTRT_NAME + "\n\n" +
            NTRT_DESCRIPTION + "\n" +
            "Version " + NTRT_VERSION + " " + NTRT_RELEASE + " " + NTRT_BUILD + "\n\n" +
            "Made by " + NTRT_AUTHOR + "\n\n" +
                "Build date " + NTRT_DATE + "\n";
        QMessageBox::about(
                parent,
                "About",
                QString::fromStdString(aboutString)
                );
    }
}

int runQtApp()
{
    startupHook();
    initQtApp();
    initQtStyles();
    initQtIcon();

    ntrt::mainWindow = std::make_shared<trtui::MainWindow>();
    ntrt::mainWindow->show();

    int exitCode = QApplication::exec();
    terminateHook();
    return exitCode;
}

void startupHook()
{
    ntrt::configManager = std::make_shared<trtdata::ConfigManager>();
    ntrt::configManager->tryInitOnce();
    ntrt::configLoaded = ntrt::configManager->loadAll();
    ntrt::dataMonitor = std::make_shared<trtcore::DataMonitor>(*ntrt::configManager);
    ntrt::dataMonitor->start();
}

void terminateHook()
{
    if (ntrt::configLoaded)
    {
        ntrt::configManager->saveAll();
    }
    ntrt::dataMonitor->stop();
}

void initQtApp()
{
    QApplication::setApplicationName("NTRT");
    QApplication::setOrganizationName("NTRT");
}

void initQtStyles()
{
    QApplication::setStyle("Fusion");
    const trtdata::AppearanceManager* appearance = ntrt::configManager->getAppearanceManager();
    trtdata::AppearanceManager::applyPalette(appearance->getCurrentTheme());

    QFile styleFile(trtutils::relativePath("res/style.qss"));
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QByteArray bytes = styleFile.readAll();

    ntrt::app->setStyleSheet(QString::fromUtf8(bytes));
}

void initQtIcon()
{
    QIcon icon(trtutils::relativePath("res/icon.ico"));
    QApplication::setWindowIcon(icon);
}
