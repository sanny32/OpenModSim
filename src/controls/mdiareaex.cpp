#include "mdiareaex.h"

///
/// \brief MdiAreaEx::MdiAreaEx
/// \param parent
///
MdiAreaEx::MdiAreaEx(QWidget* parent)
    : QMdiArea(parent)
{
    setViewMode(QMdiArea::TabbedView);
    connect(this, &QMdiArea::subWindowActivated, this, &MdiAreaEx::on_subWindowActivated);
}

///
/// \brief MdiAreaEx::eventFilter
/// \param obj
/// \param event
/// \return
///
bool MdiAreaEx::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Close) {
        auto wnd = qobject_cast<QMdiSubWindow*>(obj);
        if(_tabBar) _tabBar->removeSubWindow(wnd);
    }
    return QMdiArea::eventFilter(obj, event);
}

///
/// \brief MdiAreaEx::addSubWindow
/// \param widget
/// \param flags
/// \return
///
QMdiSubWindow* MdiAreaEx::addSubWindow(QWidget* widget, Qt::WindowFlags flags)
{
    auto wnd = QMdiArea::addSubWindow(widget, flags);

    if(_tabBar) {
        _tabBar->addSubWindow(wnd);
    }
    updateTabBarGeometry();

    return wnd;
}

///
/// \brief MdiAreaEx::removeSubWindow
/// \param widget
///
void MdiAreaEx::removeSubWindow(QWidget* widget)
{
    QMdiArea::removeSubWindow(widget);
    updateTabBarGeometry();
}

///
/// \brief MdiAreaEx::setViewMode
/// \param mode
///
void MdiAreaEx::setViewMode(ViewMode mode)
{
    QMdiArea::setViewMode(mode);

    if (mode == QMdiArea::TabbedView) {
        setupTabbedMode();
    }
}

///
/// \brief MdiAreaEx::setupTabbedMode
///
void MdiAreaEx::setupTabbedMode()
{
    auto tabBar = findChild<QTabBar*>();
    if (!tabBar) return;
    tabBar->setVisible(false);

    _tabBar = new MdiTabBar(this);
    _tabBar->setAutoFillBackground(true);
    _tabBar->setDocumentMode(documentMode());
    _tabBar->setTabsClosable(tabsClosable());
    _tabBar->setExpanding(tabsExpanding());
    _tabBar->setMovable(tabsMovable());

    foreach (QMdiSubWindow* wnd, subWindowList())
        _tabBar->addSubWindow(wnd);

    QMdiSubWindow* current = currentSubWindow();
    if (current) {
        _tabBar->setCurrentSubWindow(current);

        if (current->isMaximized())
            current->showNormal();

         if (!testOption(QMdiArea::DontMaximizeSubWindowOnActivation)) {
            current->showMaximized();
        }
    }

    if(isVisible()) {
        _tabBar->show();
    }

    updateTabBarGeometry();

    connect(tabBar, &QObject::destroyed, this, &MdiAreaEx::on_tabBarDestroyed);
    connect(_tabBar, &QTabBar::currentChanged, this, &MdiAreaEx::on_currentTabChanged);
    connect(_tabBar, &QTabBar::tabCloseRequested, this, &MdiAreaEx::on_closeTab);
    connect(_tabBar, &QTabBar::tabMoved, this, &MdiAreaEx::on_moveTab);
}

///
/// \brief MdiAreaEx::on_currentTabChanged
/// \param index
///
void MdiAreaEx::on_currentTabChanged(int index)
{
    auto wnd = subWindowAtIndex(index);
    if(wnd) setActiveSubWindow(wnd);
}

///
/// \brief MdiAreaEx::on_closeTab
/// \param index
///
void MdiAreaEx::on_closeTab(int index)
{
    auto wnd = subWindowAtIndex(index);
    if(wnd) wnd->close();
}

///
/// \brief MdiAreaEx::on_moveTab
/// \param from
/// \param to
///
void MdiAreaEx::on_moveTab(int from, int to)
{

}

///
/// \brief MdiAreaEx::on_tabBarDestroyed
///
void MdiAreaEx::on_tabBarDestroyed()
{
    if (_tabBar) {
        _tabBar->deleteLater();
        _tabBar = nullptr;
    }
}

///
/// \brief MdiAreaEx::on_subWindowActivated
///
void MdiAreaEx::on_subWindowActivated(QMdiSubWindow* wnd)
{
    if(!_tabBar)
        return;

    _tabBar->setCurrentSubWindow(wnd);
}

///
/// \brief MdiAreaEx::refreshTabBar
///
void MdiAreaEx::refreshTabBar()
{
    if (!_tabBar)
        return;

    _tabBar->setDocumentMode(documentMode());
    _tabBar->setTabsClosable(tabsClosable());
    _tabBar->setExpanding(tabsExpanding());
    _tabBar->setMovable(tabsMovable());

    updateTabBarGeometry();
}

///
/// \brief MdiAreaEx::updateTabBarGeometry
///
void MdiAreaEx::updateTabBarGeometry()
{
    if (!_tabBar)
        return;

    const QSize tabBarSizeHint = _tabBar->sizeHint();

    int areaHeight = height();
    if (horizontalScrollBar() && horizontalScrollBar()->isVisible())
        areaHeight -= horizontalScrollBar()->height();

    int areaWidth = width();
    if (verticalScrollBar() && verticalScrollBar()->isVisible())
        areaWidth -= verticalScrollBar()->width();

    QRect tabBarRect;
    switch (tabPosition()) {
        case QTabWidget::North:
            setViewportMargins(0, tabBarSizeHint.height(), 0, 0);
            tabBarRect = QRect(0, 0, areaWidth, tabBarSizeHint.height());
            break;
        case QTabWidget::South:
            setViewportMargins(0, 0, 0, tabBarSizeHint.height());
            tabBarRect = QRect(0, areaHeight - tabBarSizeHint.height(), areaWidth, tabBarSizeHint.height());
            break;
        case QTabWidget::East:
            if (layoutDirection() == Qt::LeftToRight)
                setViewportMargins(0, 0, tabBarSizeHint.width(), 0);
            else
                setViewportMargins(tabBarSizeHint.width(), 0, 0, 0);
            tabBarRect = QRect(areaWidth - tabBarSizeHint.width(), 0, tabBarSizeHint.width(), areaHeight);
            break;
        case QTabWidget::West:
            if (layoutDirection() == Qt::LeftToRight)
                setViewportMargins(tabBarSizeHint.width(), 0, 0, 0);
            else
                setViewportMargins(0, 0, tabBarSizeHint.width(), 0);
            tabBarRect = QRect(0, 0, tabBarSizeHint.width(), areaHeight);
            break;
        default:
            break;
    }

    _tabBar->setGeometry(QStyle::visualRect(layoutDirection(), contentsRect(), tabBarRect));
}

///
/// \brief MdiAreaEx::resizeEvent
/// \param event
///
void MdiAreaEx::resizeEvent(QResizeEvent* event)
{
    QMdiArea::resizeEvent(event);
    updateTabBarGeometry();
}

///
/// \brief MdiAreaEx::setVisible
/// \param visible
///
void MdiAreaEx::setVisible(bool visible)
{
    QMdiArea::setVisible(visible);
    if(_tabBar) _tabBar->setVisible(visible);
}

///
/// \brief MdiAreaEx::subWindowAtIndex
/// \param index
/// \return
///
QMdiSubWindow* MdiAreaEx::subWindowAtIndex(int index) const
{
    return (index >= 0 && index < subWindowList().size()) ? subWindowList().at(index) : nullptr;
}

///
/// \brief MdiAreaEx::splitTab
/// \param index
/// \param orientation
///
void MdiAreaEx::splitTab(int index, Qt::Orientation orientation)
{
    QMdiSubWindow* sw = subWindowAtIndex(index);
    if (!sw) return;

    QWidget* w = sw->widget();
    sw->hide();
    removeSubWindow(sw);

    QSplitter* splitter = new QSplitter(orientation, parentWidget());

    this->setParent(splitter);
    splitter->addWidget(this);

    w->setParent(splitter);
    splitter->addWidget(w);

    if (auto layout = parentWidget()->layout())
        layout->addWidget(splitter);
    else
        splitter->setParent(parentWidget());
}
