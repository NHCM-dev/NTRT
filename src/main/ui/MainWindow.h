#pragma once
#include <QMainWindow>

namespace ntrt
{
    class MainWindow final : public QMainWindow
    {
    public:
        explicit MainWindow(QWidget* parent = nullptr);
    };
}
