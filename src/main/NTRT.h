//
// Created by NHCM on 2026/11/6.
//

#ifndef NTRT_NTRT_H
#define NTRT_NTRT_H
#include <string>
#include <vector>
#include <QApplication>

#include "data/DataManager.h"
#include "ui/MainWindow.h"

namespace ntrt
{
    inline bool configLoaded = false;

    inline QApplication* app = nullptr;
    inline trtui::MainWindow* mainWindow = nullptr;
    inline std::vector<std::string> args;

    inline trtdata::DataManager* dataManager = nullptr;

    void quitApp();
}

int main(int argc, char* argv[]);

int runQtApp();

void cleanMem();

void startupHook();
void terminateHook();

void initQtApp();
void initQtStyles();
void initQtIcon();

#endif //NTRT_NTRT_H
