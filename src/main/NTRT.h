//
// Created by NHCM on 2026/11/6.
//

#ifndef NTRT_NTRT_H
#define NTRT_NTRT_H
#include <string>
#include <vector>
#include <QApplication>

#include "data/ConfigManager.h"
#include "monitor/DataMonitor.h"
#include "ui/MainWindow.h"

namespace ntrt
{
    inline bool configLoaded = false;

    inline std::shared_ptr<QApplication> app = nullptr;
    inline std::shared_ptr<trtui::MainWindow> mainWindow = nullptr;
    inline std::vector<std::string> args;

    inline std::shared_ptr<trtdata::ConfigManager> configManager = nullptr;
    inline std::shared_ptr<trtcore::DataMonitor> dataMonitor = nullptr;

    void quitApp();
    void displayAbout(QWidget* parent);
}

int main(int argc, char* argv[]);

int runQtApp();

void startupHook();
void terminateHook();

void initQtApp();
void initQtStyles();
void initQtIcon();

#endif //NTRT_NTRT_H
