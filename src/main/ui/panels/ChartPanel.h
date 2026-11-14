#ifndef NTRT_CHARTPANEL_H
#define NTRT_CHARTPANEL_H
#include <QFrame>
namespace trtui
{
    class ChartPanel final : public QFrame
    {
        void initContent();
    public:
        explicit ChartPanel(QWidget* parent = nullptr);
    };
}
#endif //NTRT_CHARTPANEL_H
