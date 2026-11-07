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

namespace trtui
{
    enum class Navigation : int
    {
        Market = 1,
        Trade = 2,
        Account = 3,
        Research = 4,
        Settings = 5
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

        explicit MainWindow(QWidget* parent = nullptr);

    private:
        void initLayouts();
        void initMenuBar();
        void initNavigation();
        void initWidgets();
    };
}

#endif //NTRT_MAINWINDOW_H