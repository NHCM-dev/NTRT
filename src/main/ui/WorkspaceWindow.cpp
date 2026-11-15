#include "WorkspaceWindow.h"
#include "main/util/PaintUtil.h"
#include "main/util/QtUtil.h"
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
#include <QPaintEvent>
#include <QTimer>
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
            trtutils::clipToParentRect(*painter, widget);
            painter->fillRect(option->rect, option->palette.brush(QPalette::Window));
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

            const QPalette palette = widget ? widget->window()->palette() : title->palette;
            painter->save();
            trtutils::clipToParentRect(*painter, widget);
            painter->setClipRect(title->rect, Qt::IntersectClip);
            painter->fillRect(title->rect, palette.brush(QPalette::Window));
            painter->setPen(palette.color(QPalette::Mid));
            painter->drawLine(title->rect.bottomLeft(), title->rect.bottomRight());
            QRect label = subControlRect(CC_TitleBar, title, SC_TitleBarLabel, widget).adjusted(8, 0, -6, 0);
            if (widget->property("titleHovered").toBool() && label.contains(widget->mapFromGlobal(QCursor::pos())))
                painter->fillRect(
                    QRect(title->rect.left(), title->rect.top(), label.right() - title->rect.left() + 1,
                          title->rect.height()), palette.brush(QPalette::Mid));
            painter->setFont(widget->font());
            painter->setPen(palette.color(QPalette::WindowText));
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
                    painter->fillRect(rect, palette.brush(QPalette::Mid));
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

    void WorkspaceWindow::paintEvent(QPaintEvent* event)
    {
        QMdiSubWindow::paintEvent(event);
        const bool active = mdiArea() && mdiArea()->activeSubWindow() == this;
        const QRegion border = trtutils::frameRegion(rect(), active ? 2 : 1);
        QPainter painter(this);
        painter.setClipRegion(event->region().intersected(border));
        trtutils::clipToParentRect(painter, this);
        painter.fillRect(rect(), palette().brush(active ? QPalette::Highlight : QPalette::Mid));
    }

    void WorkspaceWindow::constrainPosition()
    {
        if (!parentWidget() || isMaximized() || isMinimized())
            return;
        const QRect area = parentWidget()->rect();
        const QPoint bounded(std::clamp(x(), 0, std::max(0, area.width() - width())),
            std::clamp(y(), 0, std::max(0, area.height() - height())));
        if (bounded == pos())
            return;
        move(bounded);
    }

    void WorkspaceWindow::moveEvent(QMoveEvent* event)
    {
        QMdiSubWindow::moveEvent(event);
        updateExposedFrame();
    }

    void WorkspaceWindow::resizeEvent(QResizeEvent* event)
    {
        QMdiSubWindow::resizeEvent(event);
        updateExposedFrame();
    }

    void WorkspaceWindow::updateExposedFrame()
    {
        const QRect oldGeometry = previousGeometry;
        previousGeometry = geometry();
        QWidget* desktop = parentWidget();
        if (!desktop || !isVisible() || !oldGeometry.isValid() || oldGeometry == previousGeometry)
            return;
        // Include the fractional-DPI fringe around the old two-pixel outline.
        const QRegion outline = trtutils::frameRegion(oldGeometry.adjusted(-1, -1, 1, 1), 3);
        const QRegion exposed = outline.subtracted(QRegion(previousGeometry));
        // The one-pixel fringe can cross the viewport and touch adjacent chrome.
        QWidget* surface = desktop->window();
        trtutils::updateWidgetTreeRegion(surface, exposed.translated(desktop->mapTo(surface, QPoint())));
    }

    void WorkspaceWindow::changeEvent(QEvent* event)
    {
        QMdiSubWindow::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        {
            QTimer::singleShot(0, this, [this]
            {
                update();
            });
        }
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
        constrainPosition();
    }

    void WorkspaceWindow::leaveEvent(QEvent* event)
    {
        setProperty("titleHovered", false);
        update();
        QMdiSubWindow::leaveEvent(event);
    }

}
