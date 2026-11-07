//
// Created by NHCM on 2026/11/6.
//

#ifndef NTRT_NTRT_H
#define NTRT_NTRT_H
#include <string>
#include <vector>
#include <QApplication>

#include "ui/MainWindow.h"

namespace ntrt
{
    static inline QApplication* app = nullptr;
    static inline MainWindow* mainWindow = nullptr;
    static inline std::vector<std::string> args;
}

int main(int argc, char* argv[]);

void initQtStyles();

#endif //NTRT_NTRT_H
