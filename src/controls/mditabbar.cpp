#include "mditabbar.h"

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
        extern QString setWindowTitleHelper(const QString&, const QWidget*);
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

    const int index = addTab(wnd->windowIcon(), tabTextForWindow(wnd));
    setTabData(index, QVariant::fromValue(wnd));
}

///
/// \brief MdiTabBar::removeSubWindow
/// \param wnd
///
void MdiTabBar::removeSubWindow(QMdiSubWindow* wnd)
{
    if(!wnd) return;

    for(int i = 0; i < count(); i++) {
        if(tabData(i).value<QMdiSubWindow*>() == wnd) {
            removeTab(i);
            break;
        }
    }
}

///
/// \brief MdiTabBar::currentSubWindow
/// \return
///
QMdiSubWindow* MdiTabBar::currentSubWindow() const
{
    if(currentIndex() == -1)
        return nullptr;

    return tabData(currentIndex()).value<QMdiSubWindow*>();
}

///
/// \brief MdiTabBar::setCurrentSubWindow
/// \param wnd
///
void MdiTabBar::setCurrentSubWindow(QMdiSubWindow* wnd)
{
    for(int i = 0; i < count(); i++) {
        if(tabData(i).value<QMdiSubWindow*>() == wnd) {
            setCurrentIndex(i);
            break;
        }
    }
}

///
/// \brief MdiTabBar::createSplitButton
///
void MdiTabBar::createSplitButton()
{
    if (_splitButton)
        return;

    _splitButton = new QToolButton(this);
    _splitButton->setAutoRaise(true);
    _splitButton->setIcon(QIcon(":/res/actionSplitView.png"));
}
