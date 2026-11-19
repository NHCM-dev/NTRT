#ifndef NTRT_NUMERICWIDGETS_H
#define NTRT_NUMERICWIDGETS_H

#include <QDoubleSpinBox>
#include <QLabel>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <optional>

namespace trtui
{
    class DecimalSpinBox final : public QDoubleSpinBox
    {
        Q_OBJECT
        QString textFromValue(double value) const override;
    public:
        explicit DecimalSpinBox(QWidget* parent = nullptr);
    };

    class PriceLabel final : public QLabel
    {
        Q_OBJECT
        qsizetype highlightStart = -1;
        qsizetype highlightLength = 0;
        double difference = 0;

        void paintEvent(QPaintEvent* event) override;
    public:
        explicit PriceLabel(QWidget* parent = nullptr);
        void setPriceText(const QString& prefix, const QString& price, const QString& suffix,
                          double movement = 0, qint64 age = -1, bool flashEnabled = false);
    };

    class ForegroundItemDelegate final : public QStyledItemDelegate
    {
        void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
    public:
        explicit ForegroundItemDelegate(QObject* parent = nullptr);
    };

    class ValueBarDelegate final : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        static constexpr int FractionRole = Qt::UserRole + 10;
        static constexpr int RisingRole = Qt::UserRole + 11;

        explicit ValueBarDelegate(QObject* parent = nullptr);
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    };

    class RatioBar final : public QWidget
    {
        Q_OBJECT
        std::optional<double> share;
        QString leftLabel;
        QString rightLabel;
        void paintEvent(QPaintEvent* event) override;
    public:
        RatioBar(const QString& left, const QString& right, QWidget* parent = nullptr);
        void setShare(const std::optional<double>& value);
        const std::optional<double>& getShare() const;
    };
}
#endif
