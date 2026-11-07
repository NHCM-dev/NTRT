//
// Created by NHCM on 2026/11/6.
//

#include "NTRT.h"
#include "ui/MainWindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QString>

using namespace ntrt;

int main(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++)
    {
        args.emplace_back(argv[i]);
    }

    int qtArgc = 1;
    char* qtArgv[] = {argv[0], nullptr};
    app = new QApplication(qtArgc, qtArgv);

    QApplication::setApplicationName("NTRT");
    QApplication::setOrganizationName("NTRT");

    initQtStyles();

    mainWindow = new MainWindow();
    mainWindow->show();
    return QApplication::exec();
}

void initQtStyles()
{
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QFile file("./res/style.qss");

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        app->setStyleSheet(QString::fromUtf8(file.readAll()));
    }
}