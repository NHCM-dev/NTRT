#include "ChartPanel.h"
#include <QLabel>
#include <QVBoxLayout>
namespace trtui
{
    ChartPanel::ChartPanel(QWidget* parent) : QFrame(parent)
    {
        setMinimumSize(200, 120);
        initContent();
    }

    void ChartPanel::initContent()
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        QLabel* placeholder = new QLabel("Chart coming soon", this);
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setForegroundRole(QPalette::PlaceholderText);
        layout->addWidget(placeholder);
    }
}
