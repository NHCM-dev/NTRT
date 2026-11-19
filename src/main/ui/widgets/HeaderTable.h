#ifndef NTRT_HEADERTABLE_H
#define NTRT_HEADERTABLE_H
#include <QTableWidget>
#include <QPaintEvent>
#include <QStringList>
namespace trtui
{
    class HeaderTable final : public QTableWidget
    {
        void paintEvent(QPaintEvent* event) override;
    public:
        explicit HeaderTable(QWidget* parent = nullptr, const QStringList& headers = {});
    };
}
#endif
