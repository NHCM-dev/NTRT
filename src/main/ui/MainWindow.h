#pragma once
#include <QMainWindow>
#include <QMenuBar>

namespace ntrt
{
    class MainWindow final : public QMainWindow
    {
    public:
        QMenuBar* menuBar = nullptr;
        QMenu* fileMenu = nullptr;
        QMenu* aboutMenu = nullptr;

        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override = default;

    private:
        void initMenuBar();
    };
}
