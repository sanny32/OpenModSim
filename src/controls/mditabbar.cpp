#include "mditabbar.h"
#include <QApplication>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QVariant>

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
    ,_mainTabOverlay(new TabBarOverlay(this, true))
{
    setUsesScrollButtons(true);
    connect(qApp, &QApplication::focusChanged, this, [this, parent](QWidget* /*old*/, QWidget* now) {
        if(!now) return;
        if(parent && parent->isAncestorOf(now)){
            _mainTabOverlay->update();
        }
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
