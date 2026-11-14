//
// Created by NHCM on 2026/11/6.
//

#ifndef NTRT_MAINWINDOW_H
#define NTRT_MAINWINDOW_H

#include <qboxlayout.h>
#include <QButtonGroup>
#include <QMainWindow>
#include <QMenuBar>
#include <QStackedWidget>
#include <memory>
#include <unordered_map>
#include "AbstractPage.h"
#include <QTimer>

namespace trtui
{

    enum class Navigation : int
    {
        Market = 1,
        Data = 2,
        Trade = 3,
        Account = 4,
        Research = 5,
        AI = 6,
        Settings = 7
    };

    class MainWindow final : public QMainWindow
    {
    public:
        QMenuBar* menuBar = nullptr;
        QMenu* fileMenu = nullptr;
        QMenu* aboutMenu = nullptr;

        QWidget* root = nullptr;
        QHBoxLayout* rootLayout = nullptr;

        QStackedWidget* mainPages = nullptr;
        QWidget* navigationBar = nullptr;
        QButtonGroup* navigationGroup = nullptr;

        std::unordered_map<Navigation, std::unique_ptr<AbstractPage>> widgets;

        QTimer* timer200ms = new QTimer(this);

        explicit MainWindow(QWidget* parent = nullptr);

    private:
        void initLayouts();
        void initWindowGeometry();
        void initMenuBar();
        void initNavigation();
        void initStatusBar();
        void initWidgets();
        void initPages();
        void init200msSchedules();

        void updateData200ms();
    };
}

#endif //NTRT_MAINWINDOW_H
