#include "NumericWidgets.h"
#include "main/util/NumberUtil.h"
#include "main/util/PaintUtil.h"
#include "main/util/PaletteUtil.h"
#include <QApplication>
#include <QPainter>
#include <QStyleOption>
#include <algorithm>
#include <cmath>

namespace trtui
{
    DecimalSpinBox::DecimalSpinBox(QWidget* parent) : QDoubleSpinBox(parent)
    {}

    QString DecimalSpinBox::textFromValue(double value) const
    {
        QString text = trtutils::formatDecimal(value, decimals());
        text.replace(QString("."), locale().decimalPoint());
        return text;
    }

    PriceLabel::PriceLabel(QWidget* parent) : QLabel(parent)
    {
        setTextFormat(Qt::PlainText);
        setAlignment(Qt::AlignCenter);
    }

    void PriceLabel::setPriceText(const QString& prefix, const QString& price, const QString& suffix,
                                 double movement, qint64 age, bool flashEnabled)
    {
        const qsizetype offset = trtutils::priceFlashOffset(price, movement, age, flashEnabled);
        highlightStart = offset < 0 ? -1 : prefix.size() + offset;
        highlightLength = offset < 0 ? 0 : price.size() - offset;
        difference = movement;
        setText(prefix + price + suffix);
        update();
    }

    void PriceLabel::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);
        QPainter painter(this);
        QStyleOption option;
        option.initFrom(this);
        style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
        const QColor color = difference > 0 ? trtutils::risingColor(palette()) : trtutils::fallingColor(palette());
        trtutils::drawHighlightedText(painter, contentsRect(), text(), font(), palette().color(QPalette::WindowText),
                                      alignment(), highlightStart, highlightLength, color);
    }

    ValueBarDelegate::ValueBarDelegate(QObject* parent) : QStyledItemDelegate(parent)
    {}

    void ValueBarDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyleOptionViewItem styled(option);
        initStyleOption(&styled, index);
        QStyle* style = styled.widget ? styled.widget->style() : QApplication::style();
        const QString value = styled.text;
        styled.text.clear();
        style->drawControl(QStyle::CE_ItemViewItem, &styled, painter, styled.widget);
        const bool rising = index.data(RisingRole).toBool();
        const QColor color = rising ? trtutils::risingColor(styled.palette) : trtutils::fallingColor(styled.palette);
        painter->save();
        painter->setClipRect(option.rect, Qt::IntersectClip);
        trtutils::drawValueBar(*painter, option.rect, index.data(FractionRole).toDouble(), color);
        const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &styled, styled.widget) + 1;
        const QRect area = style->subElementRect(QStyle::SE_ItemViewItemText, &styled, styled.widget).adjusted(margin, 0, -margin, 0);
        const QString elided = styled.fontMetrics.elidedText(value, styled.textElideMode, area.width());
        style->drawItemText(painter, area, styled.displayAlignment, styled.palette, true, elided, QPalette::Text);
        painter->restore();
    }

    RatioBar::RatioBar(const QString& left, const QString& right, QWidget* parent)
        : QWidget(parent), leftLabel(left), rightLabel(right)
    {
        setMinimumHeight(26);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void RatioBar::setShare(const std::optional<double>& value)
    {
        share = value && std::isfinite(*value) ? std::optional<double>(std::clamp(*value, 0.0, 1.0)) : std::nullopt;
        update();
    }

    const std::optional<double>& RatioBar::getShare() const
    {
        return share;
    }

    void RatioBar::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), palette().brush(QPalette::Base));
        const QColor rising = trtutils::risingColor(palette());
        const QColor falling = trtutils::fallingColor(palette());
        if (share)
        {
            const double split = width() * *share;
            QColor buy = rising;
            QColor sell = falling;
            buy.setAlphaF(0.22);
            sell.setAlphaF(0.22);
            const double bevel = std::min({5.0, split, width() - split});
            const QPolygonF polygon{QPointF(0, 0), QPointF(split + bevel, 0),
                                     QPointF(split - bevel, height()), QPointF(0, height())};
            painter.setPen(Qt::NoPen);
            painter.setBrush(buy);
            painter.drawPolygon(polygon);
            const QPolygonF other{QPointF(split + bevel, 0), QPointF(width(), 0),
                                   QPointF(width(), height()), QPointF(split - bevel, height())};
            painter.setBrush(sell);
            painter.drawPolygon(other);
        }
        const double buyPercent = share ? std::round(*share * 10000) / 100 : 0;
        const QString left = leftLabel + " " + (share ? QString::number(buyPercent, 'f', 2) + "%" : QString("—"));
        const QString right = (share ? QString::number(100 - buyPercent, 'f', 2) + "% " : QString("— ")) + rightLabel;
        painter.setPen(share ? rising : palette().color(QPalette::PlaceholderText));
        painter.drawText(rect().adjusted(7, 0, -7, 0), Qt::AlignLeft | Qt::AlignVCenter, left);
        painter.setPen(share ? falling : palette().color(QPalette::PlaceholderText));
        painter.drawText(rect().adjusted(7, 0, -7, 0), Qt::AlignRight | Qt::AlignVCenter, right);
    }
}
