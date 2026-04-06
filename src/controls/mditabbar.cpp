#include "mditabbar.h"
#include <QApplication>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleOptionTab>
#include <QVariant>

static inline QPoint mouseEventPos(const QMouseEvent* e)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position().toPoint();
#else
    return e->pos();
#endif
}

static inline QPoint mouseEventGlobalPos(const QMouseEvent* e)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->globalPosition().toPoint();
#else
    return e->globalPos();
#endif
}

///
/// \brief The DragTabOverlay class renders a semi-transparent tab image
/// that follows the cursor when a tab is dragged outside its panel.
///
class DragTabOverlay : public QWidget
{
public:
    explicit DragTabOverlay(const QPixmap& px)
        : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
        , _px(px)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        resize(px.size());
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setOpacity(0.75);
        p.drawPixmap(0, 0, _px);
    }

private:
    QPixmap _px;
};

class MdiTabBarStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override
    {
        QProxyStyle::drawControl(element, option, painter, widget);

        if(element != CE_TabBarTab || !widget)
            return;

        const auto* tabBar = qobject_cast<const QTabBar*>(widget);
        const auto* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);
        if(!tabBar || !tabOption)
            return;

        if(!(tabOption->state & State_Selected))
            return;

        if(!widget->property("mdiIndicatorActive").toBool())
            return;

        const QRect r = tabOption->rect;
        const QColor color = widget->palette().highlight().color();
        constexpr int thickness = 2;

        switch (tabBar->shape()) {
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                painter->fillRect(r.left(), r.bottom() - thickness + 1, r.width(), thickness, color);
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                painter->fillRect(r.left(), r.top(), thickness, r.height(), color);
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                painter->fillRect(r.right() - thickness + 1, r.top(), thickness, r.height(), color);
                break;
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            default:
                painter->fillRect(r.left(), r.top(), r.width(), thickness, color);
                break;
        }
    }
};

///
/// \brief setWindowTitleHelper
/// \param title
/// \param widget
/// \return
///
static inline QString setWindowTitleHelper(const QString &title, const QWidget *widget)
{
    Q_ASSERT(widget);
    QString cap = title;
    if (cap.isEmpty())
        return cap;
    QLatin1String placeHolder("[*]");
    int index = cap.indexOf(placeHolder);
    // here the magic begins
    while (index != -1) {
        index += placeHolder.size();
        int count = 1;
        while (cap.indexOf(placeHolder, index) == index) {
            ++count;
            index += placeHolder.size();
        }
        if (count%2) { // odd number of [*] -> replace last one
            int lastIndex = cap.lastIndexOf(placeHolder, index - 1);
            if (widget->isWindowModified()
                && widget->style()->styleHint(QStyle::SH_TitleBar_ModifyNotification, nullptr, widget))
                cap.replace(lastIndex, 3, QWidget::tr("*"));
            else
                cap.remove(lastIndex, 3);
        }
        index = cap.indexOf(placeHolder, index);
    }
    cap.replace(QLatin1String("[*][*]"), placeHolder);
    return cap;
}

///
/// \brief tabTextForWindow
/// \param subWindow
/// \return
///
static inline QString tabTextForWindow(QMdiSubWindow *subWindow)
{
    if (!subWindow)
        return QString();

    QString title = subWindow->windowTitle();
    if (subWindow->isWindowModified()) {
        title.replace(QLatin1String("[*]"), QLatin1String("*"));
    } else {
        title = setWindowTitleHelper(title, subWindow);
    }

    return title.isEmpty() ? QMdiArea::tr("(Untitled)") : title;
}

///
/// \brief MdiTabBar::MdiTabBar
/// \param parent
///
MdiTabBar::MdiTabBar(QWidget* parent)
    : QTabBar(parent)
{
    setStyle(new MdiTabBarStyle(style()));

    setUsesScrollButtons(true);
    setAutoFillBackground(true);
    setProperty("mdiIndicatorActive", true);

    connect(qApp, &QApplication::focusChanged, this, [this, parent](QWidget* /*old*/, QWidget* now) {
        if (!now || !parent)
            return;
        const Qt::WindowType wtype = now->window()->windowType();
        if (wtype == Qt::Popup || wtype == Qt::ToolTip)
            return;
        const bool active = (now == parent) || parent->isAncestorOf(now);
        if (active == _indicatorActive)
            return;
        _indicatorActive = active;
        setProperty("mdiIndicatorActive", active);
        update();
    });
}

///
/// \brief MdiTabBar::addSubWindow
/// \param wnd
///
void MdiTabBar::addSubWindow(QMdiSubWindow* wnd)
{
    if(!wnd) return;

    const int existingIndex = indexOfSubWindow(wnd);
    if(existingIndex != -1) {
        updateSubWindowState(wnd);
        setCurrentIndex(existingIndex);
        return;
    }

    const int index = addTab(wnd->windowIcon(), tabTextForWindow(wnd));
    setTabData(index, QVariant::fromValue(wnd));

    connect(wnd, &QWidget::windowTitleChanged, this, [this, wnd](const QString&) {
        updateSubWindowState(wnd);
    });
    connect(wnd, &QWidget::windowIconChanged, this, [this, wnd](const QIcon&) {
        updateSubWindowState(wnd);
    });
    connect(wnd, &QObject::destroyed, this, [this, wnd]() {
        removeSubWindow(wnd);
    });

    setCurrentIndex(index);
}

///
/// \brief MdiTabBar::removeSubWindow
/// \param wnd
///
void MdiTabBar::removeSubWindow(QMdiSubWindow* wnd)
{
    if(!wnd) return;

    const int index = indexOfSubWindow(wnd);
    if(index != -1)
        removeTab(index);
}

///
/// \brief MdiTabBar::indexOfSubWindow
/// \param wnd
/// \return
///
int MdiTabBar::indexOfSubWindow(QMdiSubWindow* wnd) const
{
    if(!wnd) return -1;

    for(int i = 0; i < count(); i++) {
        if(tabData(i).value<QMdiSubWindow*>() == wnd)
            return i;
    }

    return -1;
}

///
/// \brief MdiTabBar::subWindowAt
/// \param tabIndex
/// \return
///
QMdiSubWindow* MdiTabBar::subWindowAt(int tabIndex) const
{
    if(tabIndex < 0 || tabIndex >= count())
        return nullptr;

    return tabData(tabIndex).value<QMdiSubWindow*>();
}

///
/// \brief MdiTabBar::currentSubWindow
/// \return
///
QMdiSubWindow* MdiTabBar::currentSubWindow() const
{
    return subWindowAt(currentIndex());
}

///
/// \brief MdiTabBar::setCurrentSubWindow
/// \param wnd
///
void MdiTabBar::setCurrentSubWindow(QMdiSubWindow* wnd)
{
    const int index = indexOfSubWindow(wnd);
    if(index != -1)
        setCurrentIndex(index);
}

///
/// \brief MdiTabBar::mousePressEvent
/// \param event
///
void MdiTabBar::mousePressEvent(QMouseEvent* event)
{
    // Record subwindow and grab tab image BEFORE Qt's press handler can reorder tabs.
    if (event->button() == Qt::LeftButton) {
        const int idx = tabAt(mouseEventPos(event));
        _dragSubWindow = subWindowAt(idx);
        if (_dragSubWindow) {
            const QRect tr = tabRect(idx);
            _dragPixmap  = grab(tr);
            _dragHotspot = mouseEventPos(event) - tr.topLeft();
        } else {
            _dragPixmap  = QPixmap();
            _dragHotspot = QPoint();
        }
    }
    QTabBar::mousePressEvent(event);
}

///
/// \brief MdiTabBar::mouseMoveEvent
/// \param event
///
void MdiTabBar::mouseMoveEvent(QMouseEvent* event)
{
    if (_dragSubWindow && (event->buttons() & Qt::LeftButton)) {
        auto* par = parentWidget();
        if (par) {
            const QPoint posInParent = mapTo(par, mouseEventPos(event));
            const bool outside = !par->rect().contains(posInParent);

            if (outside) {
                // Create overlay on first exit from the panel.
                if (!_dragOverlay && !_dragPixmap.isNull()) {
                    _dragOverlay = new DragTabOverlay(_dragPixmap);
                }
                if (_dragOverlay) {
                    _dragOverlay->move(mouseEventGlobalPos(event) - _dragHotspot);
                    _dragOverlay->show();
                }
                if (!_dragCursorSet) {
                    QApplication::setOverrideCursor(Qt::DragMoveCursor);
                    _dragCursorSet = true;
                }
            } else {
                // Cursor returned inside the source panel — hide overlay.
                if (_dragOverlay)
                    _dragOverlay->hide();
                if (_dragCursorSet) {
                    QApplication::restoreOverrideCursor();
                    _dragCursorSet = false;
                }
            }
        }
    }
    QTabBar::mouseMoveEvent(event);
}

///
/// \brief MdiTabBar::mouseReleaseEvent
/// \param event
///
void MdiTabBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        delete _dragOverlay;
        _dragOverlay = nullptr;

        if (_dragCursorSet) {
            QApplication::restoreOverrideCursor();
            _dragCursorSet = false;
        }

        if (_dragSubWindow) {
            // Check if the mouse was released outside the parent MdiArea's bounds.
            auto* par = parentWidget();
            if (par) {
                const QPoint posInParent = mapTo(par, mouseEventPos(event));
                if (!par->rect().contains(posInParent)) {
                    QMdiSubWindow* subWnd = _dragSubWindow;
                    _dragSubWindow = nullptr;
                    QTabBar::mouseReleaseEvent(event);
                    emit tabDraggedOutside(subWnd, mouseEventGlobalPos(event));
                    return;
                }
            }
            _dragSubWindow = nullptr;
        }
    }
    QTabBar::mouseReleaseEvent(event);
}

///
/// \brief MdiTabBar::updateSubWindowState
/// \param wnd
///
void MdiTabBar::updateSubWindowState(QMdiSubWindow* wnd)
{
    const int index = indexOfSubWindow(wnd);
    if(index == -1)
        return;

    setTabIcon(index, wnd->windowIcon());
    setTabText(index, tabTextForWindow(wnd));
}

