#include "WorkspaceWindow.h"
#include "main/util/PaintUtil.h"
#include "main/util/QtUtil.h"
#include <QPainter>
#include <QAction>
#include <QMenu>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QHideEvent>
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

    WorkspaceWindow::WorkspaceWindow(QWidget* content, const QString& title, const QString& id, QWidget* parent)
        : QMdiSubWindow(parent)
    {
        initAppearance();
        initContent(content, title, id);
    }

    void WorkspaceWindow::initContent(QWidget* content, const QString& title, const QString& id)
    {
        setObjectName(id);
        setWindowTitle(title);
        setWidget(content);
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

    void WorkspaceWindow::addWindowAction(QMenu* menu, QAction* before)
    {
        if (windowAction)
            return;
        windowAction = new QAction(windowTitle(), this);
        windowAction->setCheckable(true);
        menu->insertAction(before, windowAction);
        updateWindowAction();
        connect(this, &QWidget::windowTitleChanged, windowAction, &QAction::setText);
        connect(windowAction, &QAction::toggled, this, [this](bool visible)
        {
            if (visible)
                showWindow();
            else
                hide();
        });
    }

    void WorkspaceWindow::showWindow()
    {
        if (isMinimized())
            showNormal();
        else
            show();
        if (mdiArea())
            mdiArea()->setActiveSubWindow(this);
        raise();
    }

    void WorkspaceWindow::updateWindowAction()
    {
        if (!windowAction)
            return;
        const QSignalBlocker blocker(windowAction);
        windowAction->setChecked(!isHidden());
    }

    QRect WorkspaceWindow::getNormalGeometry() const
    {
        return isMinimized() || isMaximized() ? normalGeometry : geometry();
    }

    void WorkspaceWindow::setNormalGeometry(const QRect& geometry)
    {
        normalGeometry = geometry;
    }

    void WorkspaceWindow::showEvent(QShowEvent* event)
    {
        QMdiSubWindow::showEvent(event);
        if (parentWidget())
            parentWidget()->installEventFilter(this);
        scheduleGeometryConstraint();
        updateWindowAction();
    }

    void WorkspaceWindow::hideEvent(QHideEvent* event)
    {
        resizeEdges = {};
        QMdiSubWindow::hideEvent(event);
        updateWindowAction();
    }

    bool WorkspaceWindow::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched == parentWidget() && event->type() == QEvent::Resize)
            scheduleGeometryConstraint();
        return QMdiSubWindow::eventFilter(watched, event);
    }

    void WorkspaceWindow::scheduleGeometryConstraint()
    {
        if (constraintPending || constraining)
            return;
        constraintPending = true;
        // MDI and main-window restoration can resize the viewport several times in one layout pass.
        QTimer::singleShot(0, this, [this]
        {
            constraintPending = false;
            constrainGeometry();
        });
    }

    void WorkspaceWindow::constrainGeometry()
    {
        if (constraining || !parentWidget() || isMaximized() || isMinimized() || !isVisible())
            return;
        // Keep the requested rectangle across viewport changes, including maximized startup.
        const QRect target = requestedGeometry.isValid() ? requestedGeometry : geometry();
        const QRect bounded = trtutils::boundedWidgetGeometry(target, parentWidget()->rect());
        if (bounded == geometry())
            return;
        constraining = true;
        setGeometry(bounded);
        constraining = false;
    }

    Qt::Edges WorkspaceWindow::getResizeEdges(const QPoint& position) const
    {
        Qt::Edges edges;
        if (isMaximized() || isMinimized())
            return edges;
        const int frame = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, nullptr, this);
        if (position.x() < frame)
            edges |= Qt::LeftEdge;
        else if (position.x() >= width() - frame)
            edges |= Qt::RightEdge;
        if (position.y() < frame)
            edges |= Qt::TopEdge;
        else if (position.y() >= height() - frame)
            edges |= Qt::BottomEdge;
        return edges;
    }

    void WorkspaceWindow::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            resizeEdges = getResizeEdges(event->position().toPoint());
            if (resizeEdges)
            {
                resizeGeometry = geometry();
                resizeOrigin = event->globalPosition().toPoint();
                mdiArea()->setActiveSubWindow(this);
                raise();
                event->accept();
                return;
            }
        }
        QMdiSubWindow::mousePressEvent(event);
    }

    void WorkspaceWindow::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton && resizeEdges)
        {
            resizeEdges = {};
            event->accept();
            return;
        }
        QMdiSubWindow::mouseReleaseEvent(event);
    }

    void WorkspaceWindow::moveEvent(QMoveEvent* event)
    {
        QMdiSubWindow::moveEvent(event);
        if (!constraining && !isMaximized() && !isMinimized())
            requestedGeometry = geometry();
        scheduleGeometryConstraint();
        if (!isMaximized() && !isMinimized())
            normalGeometry = geometry();
        updateExposedFrame();
    }

    void WorkspaceWindow::resizeEvent(QResizeEvent* event)
    {
        QMdiSubWindow::resizeEvent(event);
        if (!constraining && !isMaximized() && !isMinimized())
            requestedGeometry = geometry();
        scheduleGeometryConstraint();
        if (!isMaximized() && !isMinimized())
            normalGeometry = geometry();
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
        if (resizeEdges && parentWidget())
        {
            const QPoint delta = event->globalPosition().toPoint() - resizeOrigin;
            const QSize minimum = minimumSizeHint().expandedTo(minimumSize());
            setGeometry(trtutils::resizedWidgetGeometry(resizeGeometry, delta, resizeEdges,
                parentWidget()->rect(), minimum));
            event->accept();
            return;
        }
        QMdiSubWindow::mouseMoveEvent(event);
        constrainGeometry();
        requestedGeometry = geometry();
    }

    void WorkspaceWindow::leaveEvent(QEvent* event)
    {
        setProperty("titleHovered", false);
        update();
        QMdiSubWindow::leaveEvent(event);
    }

}
