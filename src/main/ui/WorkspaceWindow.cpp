#include "WorkspaceWindow.h"
#include <QPainter>
#include <QProxyStyle>
#include <QStyleOptionTitleBar>
#include <array>
#include <QMdiArea>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QVariant>
#include <QCursor>
#include <QMoveEvent>
#include <QResizeEvent>
#include <algorithm>

namespace trtui
{
    class WorkspaceWindowStyle final : public QProxyStyle
    {
    public:
        WorkspaceWindowStyle() : QProxyStyle("Fusion")
        {}

        void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                           const QWidget* widget) const override
        {
            if (element == PE_FrameFocusRect)
                return;
            if (element != PE_FrameWindow)
            {
                QProxyStyle::drawPrimitive(element, option, painter, widget);
                return;
            }
            painter->save();
            painter->fillRect(option->rect, option->palette.brush(QPalette::Window));
            const QMdiSubWindow* window = qobject_cast<const QMdiSubWindow*>(widget);
            const bool active = window && window->mdiArea() && window->mdiArea()->activeSubWindow() == window;
            const int thickness = active ? 2 : 1;
            const QRect frame = option->rect;
            const QRegion border = QRegion(frame).subtracted(QRegion(frame.adjusted(thickness, thickness, -thickness, -thickness)));
            painter->setClipRegion(border, Qt::IntersectClip);
            painter->fillRect(frame, option->palette.brush(active ? QPalette::Highlight : QPalette::Mid));
            painter->restore();
        }

        int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget,
                      QStyleHintReturn* data) const override
        {
            if (hint == SH_Workspace_FillSpaceOnMaximize)
                return 1;
            if (hint == SH_WindowFrame_Mask)
                return 0;
            return QProxyStyle::styleHint(hint, option, widget, data);
        }

        void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter,
                                const QWidget* widget) const override
        {
            if (control != CC_TitleBar)
            {
                QProxyStyle::drawComplexControl(control, option, painter, widget);
                return;
            }
            const QStyleOptionTitleBar* title = qstyleoption_cast<const QStyleOptionTitleBar*>(option);
            if (!title)
                return;

            painter->save();
            painter->setClipRect(title->rect);
            painter->fillRect(title->rect, title->palette.brush(QPalette::Window));
            painter->setPen(title->palette.color(QPalette::Mid));
            painter->drawLine(title->rect.bottomLeft(), title->rect.bottomRight());
            QRect label = subControlRect(CC_TitleBar, title, SC_TitleBarLabel, widget).adjusted(8, 0, -6, 0);
            if (widget->property("titleHovered").toBool() && label.contains(widget->mapFromGlobal(QCursor::pos())))
                painter->fillRect(
                    QRect(title->rect.left(), title->rect.top(), label.right() - title->rect.left() + 1,
                          title->rect.height()), title->palette.brush(QPalette::Mid));
            painter->setFont(widget->font());
            painter->setPen(title->palette.color(QPalette::WindowText));
            painter->drawText(label, Qt::AlignLeft | Qt::AlignVCenter,
                              painter->fontMetrics().elidedText(title->text, Qt::ElideRight, label.width()));

            const std::array<SubControl, 4> buttons{
                SC_TitleBarMinButton,
                SC_TitleBarMaxButton,
                SC_TitleBarNormalButton,
                SC_TitleBarCloseButton
            };
            for (SubControl button : buttons)
            {
                if (!title->subControls.testFlag(button))
                    continue;
                const bool minimized = title->titleBarState & Qt::WindowMinimized;
                const bool maximized = title->titleBarState & Qt::WindowMaximized;
                if ((button == SC_TitleBarMinButton && minimized) || (button == SC_TitleBarMaxButton && maximized) ||
                    (button == SC_TitleBarNormalButton && !minimized && !maximized))
                    continue;
                const QRect rect = subControlRect(CC_TitleBar, title, button, widget);
                if (rect.isEmpty())
                    continue;
                if (title->activeSubControls.testFlag(button) && title->state.testFlag(State_MouseOver))
                    painter->fillRect(rect, title->palette.brush(QPalette::Mid));
                const QPoint center = rect.center();
                const QRect glyph(center.x() - 4, center.y() - 4, 8, 8);
                switch (button)
                {
                    case SC_TitleBarMinButton:
                        painter->drawLine(glyph.bottomLeft(), glyph.bottomRight());
                        break;
                    case SC_TitleBarMaxButton:
                        painter->drawRect(glyph);
                        break;
                    case SC_TitleBarNormalButton:
                        painter->drawRect(glyph.adjusted(2, 0, 0, -2));
                        painter->drawRect(glyph.adjusted(0, 2, -2, 0));
                        break;
                    case SC_TitleBarCloseButton:
                        painter->drawLine(glyph.topLeft(), glyph.bottomRight());
                        painter->drawLine(glyph.topRight(), glyph.bottomLeft());
                        break;
                    default:
                        break;
                }
            }
            painter->restore();
        }

        int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override
        {
            if (metric == PM_TitleBarHeight)
                return 30;
            if (metric == PM_MdiSubWindowFrameWidth)
                return 8;
            return QProxyStyle::pixelMetric(metric, option, widget);
        }
    };

    WorkspaceWindow::WorkspaceWindow(QWidget* parent) : QMdiSubWindow(parent)
    {
        initAppearance();
    }

    void WorkspaceWindow::initAppearance()
    {
        setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
            Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
        WorkspaceWindowStyle* appearance = new WorkspaceWindowStyle;
        appearance->setParent(this);
        setStyle(appearance);
        setAttribute(Qt::WA_DeleteOnClose, true);
        setMouseTracking(true);
    }

    void WorkspaceWindow::constrainPosition()
    {
        if (constrainingPosition || !parentWidget() || isMaximized() || isMinimized())
            return;
        const QRect area = parentWidget()->rect();
        const QPoint bounded(std::clamp(x(), 0, std::max(0, area.width() - width())),
            std::clamp(y(), 0, std::max(0, area.height() - height())));
        if (bounded == pos())
            return;
        constrainingPosition = true;
        move(bounded);
        constrainingPosition = false;
    }

    void WorkspaceWindow::moveEvent(QMoveEvent* event)
    {
        constrainPosition();
        QMdiSubWindow::moveEvent(event);
    }

    void WorkspaceWindow::resizeEvent(QResizeEvent* event)
    {
        QMdiSubWindow::resizeEvent(event);
        constrainPosition();
    }

    void WorkspaceWindow::closeEvent(QCloseEvent* event)
    {
        // QWidget close handling destroys this subwindow and its owned content.
        event->accept();
    }

    void WorkspaceWindow::mouseMoveEvent(QMouseEvent* event)
    {
        const int frame = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, nullptr, this);
        const int titleHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight, nullptr, this);
        const bool hovered = event->position().y() >= 0 && event->position().y() < frame + titleHeight;
        if (property("titleHovered").toBool() != hovered)
        {
            setProperty("titleHovered", hovered);
            update();
        }
        update(QRect(0, 0, width(), titleHeight + frame * 2));
        QMdiSubWindow::mouseMoveEvent(event);
    }

    void WorkspaceWindow::leaveEvent(QEvent* event)
    {
        setProperty("titleHovered", false);
        update();
        QMdiSubWindow::leaveEvent(event);
    }

}
