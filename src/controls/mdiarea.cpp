#include "mdiarea.h"
#include <QPainter>
#include <QScrollBar>
#include <QScopedValueRollback>
#include <QSplitter>
#include <QStyle>
#include <QStyleOptionTabBarBase>

///
/// \brief The TabBarBaseLineWidget class
///
class TabBarBaseLineWidget : public QFrame
{
public:
    explicit TabBarBaseLineWidget(MdiArea* owner)
        : QFrame(owner)
        , _owner(owner)
    {
        setFrameShape(QFrame::NoFrame);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, false);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        if (!_owner)
            return;

        auto* tab = _owner->tabBar();
        if (!tab || _owner->viewMode() != QMdiArea::TabbedView || !tab->isVisible())
            return;

        QStyleOptionTabBarBase option;
        option.initFrom(this);
        option.shape = tab->shape();
        option.documentMode = tab->documentMode();
        option.tabBarRect = rect();

        QPainter painter(this);
        style()->drawPrimitive(QStyle::PE_FrameTabBarBase, &option, &painter, this);
    }

private:
    MdiArea* _owner;
};

///
/// \brief MdiArea::MdiArea
/// \param parent
///
MdiArea::MdiArea(QWidget* parent)
    : QMdiArea(parent)
{
    setViewMode(QMdiArea::TabbedView);
    connect(this, &QMdiArea::subWindowActivated, this, &MdiArea::on_subWindowActivated);
}

///
/// \brief MdiArea::~MdiArea
///
MdiArea::~MdiArea()
{
    _destroying = true;
}

///
/// \brief MdiArea::addSubWindow
/// \param widget
/// \param flags
/// \return
///
QMdiSubWindow* MdiArea::addSubWindow(QWidget* widget, Qt::WindowFlags flags)
{
    auto* wnd = QMdiArea::addSubWindow(widget, flags);
    if (!wnd)
        return nullptr;

    wnd->installEventFilter(this);
    enforceTabbedSubWindowState(wnd);
    if (_tabBar)
        _tabBar->addSubWindow(wnd);

    updateTabBarGeometry();
    return wnd;
}

///
/// \brief MdiArea::removeSubWindow
/// \param widget
///
void MdiArea::removeSubWindow(QWidget* widget)
{
    if (_tabBar) {
        auto* wnd = qobject_cast<QMdiSubWindow*>(widget);
        if (!wnd) {
            for (auto* candidate : QMdiArea::subWindowList()) {
                if (candidate && candidate->widget() == widget) {
                    wnd = candidate;
                    break;
                }
            }
        }

        if (wnd)
            _tabBar->removeSubWindow(wnd);
    }

    QMdiArea::removeSubWindow(widget);
    updateTabBarGeometry();
}

///
/// \brief MdiArea::localSubWindowList
/// \param order
/// \return
///
QList<QMdiSubWindow*> MdiArea::localSubWindowList(WindowOrder order) const
{
    return QMdiArea::subWindowList(order);
}

///
/// \brief MdiArea::setViewMode
/// \param mode
///
void MdiArea::setViewMode(ViewMode mode)
{
    QMdiArea::setViewMode(mode);

    if (mode == QMdiArea::TabbedView) {
        setupTabbedMode();
    } else {
        setViewportMargins(0, 0, 0, 0);

        if (_tabBar)
            _tabBar->hide();
        if (_tabBarBaseLine)
            _tabBarBaseLine->hide();
    }

    emit tabBarLayoutChanged();
}

///
/// \brief MdiArea::setTabsExpanding
/// \param expanding
///
void MdiArea::setTabsExpanding(bool expanding)
{
    if (_tabsExpanding == expanding)
        return;

    _tabsExpanding = expanding;
    refreshTabBar();
}

///
/// \brief MdiArea::setDocumentMode
/// \param enabled
///
void MdiArea::setDocumentMode(bool enabled)
{
    QMdiArea::setDocumentMode(enabled);
    refreshTabBar();
}

///
/// \brief MdiArea::setTabsClosable
/// \param closable
///
void MdiArea::setTabsClosable(bool closable)
{
    QMdiArea::setTabsClosable(closable);
    refreshTabBar();
}

///
/// \brief MdiArea::setTabsMovable
/// \param movable
///
void MdiArea::setTabsMovable(bool movable)
{
    QMdiArea::setTabsMovable(movable);
    refreshTabBar();
}

///
/// \brief MdiArea::setTabBarTrailingInset
/// \param inset
///
void MdiArea::setTabBarTrailingInset(int inset)
{
    const int normalizedInset = qMax(0, inset);
    if (_tabBarTrailingInset == normalizedInset)
        return;

    _tabBarTrailingInset = normalizedInset;
    updateTabBarGeometry();
    emit tabBarLayoutChanged();
}

///
/// \brief MdiArea::eventFilter
/// \param obj
/// \param event
/// \return
///
bool MdiArea::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == _tabBar) {
        switch (event->type()) {
            case QEvent::Paint:
            case QEvent::Resize:
            case QEvent::Move:
            case QEvent::Show:
            case QEvent::Hide:
                updateViewportBaseLine();
                break;
            default:
                break;
        }
    }

    if (event->type() == QEvent::Close) {
        auto* wnd = qobject_cast<QMdiSubWindow*>(obj);
        if (_tabBar)
            _tabBar->removeSubWindow(wnd);
    }

    return QMdiArea::eventFilter(obj, event);
}

///
/// \brief MdiArea::resizeEvent
/// \param event
///
void MdiArea::resizeEvent(QResizeEvent* event)
{
    QMdiArea::resizeEvent(event);
    updateTabBarGeometry();
}

///
/// \brief MdiArea::setVisible
/// \param visible
///
void MdiArea::setVisible(bool visible)
{
    QMdiArea::setVisible(visible);
    if (visible) {
        setupTabbedMode();
        for (auto* wnd : QMdiArea::subWindowList())
            enforceTabbedSubWindowState(wnd);
    }
    if (_tabBar)
        _tabBar->setVisible(visible && viewMode() == QMdiArea::TabbedView);
    updateViewportBaseLine();
    emit tabBarLayoutChanged();
}

///
/// \brief MdiArea::on_tabBarClicked
/// \param index
///
void MdiArea::on_tabBarClicked(int index)
{
    if (!_tabBar)
        return;

    auto* wnd = subWindowAtIndex(index);
    if (wnd)
        setActiveSubWindow(wnd);

    setFocus(Qt::OtherFocusReason);
}

///
/// \brief MdiArea::on_currentTabChanged
/// \param index
///
void MdiArea::on_currentTabChanged(int index)
{
    if (!_tabBar || !_tabBar->isVisible())
        return;

    auto* wnd = subWindowAtIndex(index);
    if (wnd)
        setActiveSubWindow(wnd);
}

///
/// \brief MdiArea::on_closeTab
/// \param index
///
void MdiArea::on_closeTab(int index)
{
    if (!_tabBar)
        return;

    auto* wnd = subWindowAtIndex(index);
    if (!wnd)
        return;

    if (_tabBar->count() == 1)
        emit lastTabAboutToClose();

    wnd->close();
}

///
/// \brief MdiArea::on_moveTab
/// \param from
/// \param to
///
void MdiArea::on_moveTab(int from, int to)
{
    if(from == to)
        return;

    emit tabsReordered();
}

///
/// \brief MdiArea::on_tabBarDestroyed
///
void MdiArea::on_tabBarDestroyed()
{
    if (_tabBar) {
        _tabBar->deleteLater();
        _tabBar = nullptr;
    }

    if (_tabBarBaseLine)
        _tabBarBaseLine->hide();

    updateViewportBaseLine();
    emit tabBarLayoutChanged();
}

///
/// \brief MdiArea::on_subWindowActivated
/// \param wnd
///
void MdiArea::on_subWindowActivated(QMdiSubWindow* wnd)
{
    _lastActivatedSubWindow = wnd;
    enforceTabbedSubWindowState(wnd);

    if (!_tabBar)
        return;

    _tabBar->setCurrentSubWindow(wnd);
    updateTabBarGeometry();
}

///
/// \brief MdiArea::setupTabbedMode
///
void MdiArea::setupTabbedMode()
{
    if (_tabBar) {
        refreshTabBar();
        if (isVisible())
            _tabBar->show();
        updateTabBarGeometry();
        updateViewportBaseLine();
        return;
    }

    QTabBar* nativeTabBar = nullptr;
    const auto tabBars = findChildren<QTabBar*>();
    for (auto* candidate : tabBars) {
        if (candidate && !qobject_cast<MdiTabBar*>(candidate)) {
            nativeTabBar = candidate;
            break;
        }
    }

    if (!nativeTabBar)
        return;

    nativeTabBar->hide();
    nativeTabBar->lower();
    nativeTabBar->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    _tabBar = new MdiTabBar(this);
    _tabBar->installEventFilter(this);
    _tabBar->setDocumentMode(documentMode());
    _tabBar->setTabsClosable(tabsClosable());
    _tabBar->setExpanding(tabsExpanding());
    _tabBar->setMovable(tabsMovable());

    for (auto* wnd : QMdiArea::subWindowList()) {
        _tabBar->addSubWindow(wnd);
        enforceTabbedSubWindowState(wnd);
    }

    QMdiSubWindow* preferredCurrent = nullptr;
    if (_lastActivatedSubWindow && QMdiArea::subWindowList().contains(_lastActivatedSubWindow.data()))
        preferredCurrent = _lastActivatedSubWindow.data();
    if (!preferredCurrent)
        preferredCurrent = QMdiArea::activeSubWindow();
    if (!preferredCurrent)
        preferredCurrent = QMdiArea::currentSubWindow();
    if (preferredCurrent)
        _tabBar->setCurrentSubWindow(preferredCurrent);

    if (isVisible())
        _tabBar->show();

    updateTabBarGeometry();
    updateViewportBaseLine();

    connect(nativeTabBar, &QObject::destroyed, this, [this]() {
        if (_destroying)
            return;
        on_tabBarDestroyed();
    });
    connect(_tabBar, &QTabBar::tabBarClicked, this, &MdiArea::on_tabBarClicked, Qt::UniqueConnection);
    connect(_tabBar, &QTabBar::currentChanged, this, &MdiArea::on_currentTabChanged, Qt::UniqueConnection);
    connect(_tabBar, &QTabBar::tabCloseRequested, this, &MdiArea::on_closeTab, Qt::UniqueConnection);
    connect(_tabBar, &QTabBar::tabMoved, this, &MdiArea::on_moveTab, Qt::UniqueConnection);

    _tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_tabBar, &QTabBar::customContextMenuRequested,
            this, &MdiArea::on_tabBarContextMenu, Qt::UniqueConnection);
    connect(_tabBar, &MdiTabBar::tabDraggedOutside,
            this, &MdiArea::tabDraggedOutside, Qt::UniqueConnection);
}

///
/// \brief MdiArea::refreshTabBar
///
void MdiArea::refreshTabBar()
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
/// \brief MdiArea::updateTabBarGeometry
///
void MdiArea::updateTabBarGeometry()
{
    if (!_tabBar || _updatingTabBarGeometry)
        return;

    QScopedValueRollback<bool> updatingGuard(_updatingTabBarGeometry, true);

    const QSize tabBarSizeHint = _tabBar->sizeHint();

    int areaHeight = height();
    if (horizontalScrollBar() && horizontalScrollBar()->isVisible())
        areaHeight -= horizontalScrollBar()->height();

    int areaWidth = width();
    if (verticalScrollBar() && verticalScrollBar()->isVisible())
        areaWidth -= verticalScrollBar()->width();

    const int trailingInset = (tabPosition() == QTabWidget::North || tabPosition() == QTabWidget::South)
                                  ? _tabBarTrailingInset
                                  : 0;

    QRect tabBarRect;
    QMargins desiredMargins = viewportMargins();

    switch (tabPosition()) {
        case QTabWidget::North:
            desiredMargins = QMargins(0, tabBarSizeHint.height(), 0, 0);
            tabBarRect = QRect(0, 0, areaWidth - trailingInset, tabBarSizeHint.height());
            break;
        case QTabWidget::South:
            desiredMargins = QMargins(0, 0, 0, tabBarSizeHint.height());
            tabBarRect = QRect(0, areaHeight - tabBarSizeHint.height(), areaWidth - trailingInset, tabBarSizeHint.height());
            break;
        case QTabWidget::East:
            if (layoutDirection() == Qt::LeftToRight)
                desiredMargins = QMargins(0, 0, tabBarSizeHint.width(), 0);
            else
                desiredMargins = QMargins(tabBarSizeHint.width(), 0, 0, 0);
            tabBarRect = QRect(areaWidth - tabBarSizeHint.width(), 0, tabBarSizeHint.width(), areaHeight);
            break;
        case QTabWidget::West:
            if (layoutDirection() == Qt::LeftToRight)
                desiredMargins = QMargins(tabBarSizeHint.width(), 0, 0, 0);
            else
                desiredMargins = QMargins(0, 0, tabBarSizeHint.width(), 0);
            tabBarRect = QRect(0, 0, tabBarSizeHint.width(), areaHeight);
            break;
        default:
            break;
    }

    if (viewportMargins() != desiredMargins)
        setViewportMargins(desiredMargins);

    _tabBar->setGeometry(QStyle::visualRect(layoutDirection(), contentsRect(), tabBarRect));
    updateViewportBaseLine();
    emit tabBarLayoutChanged();
}

///
/// \brief MdiArea::updateViewportBaseLine
///
void MdiArea::updateViewportBaseLine()
{
    // In split mode the panel lives inside QSplitter. Keep the baseline hidden
    // only when there is no reserved trailing space for the split button.
    if (qobject_cast<QSplitter*>(parentWidget()) && _tabBarTrailingInset <= 0) {
        if (_tabBarBaseLine)
            _tabBarBaseLine->hide();
        return;
    }

    auto* vp = viewport();
    if (!vp || viewMode() != QMdiArea::TabbedView || !_tabBar || !_tabBar->isVisible()) {
        if (_tabBarBaseLine)
            _tabBarBaseLine->hide();
        return;
    }

    if (!_tabBarBaseLine)
        _tabBarBaseLine = new TabBarBaseLineWidget(this);

    const QRect vpRect = vp->geometry();
    QRect lineRect;
    switch (tabPosition()) {
        case QTabWidget::North:
            lineRect = QRect(vpRect.left(), vpRect.top() - 1, vpRect.width(), 1);
            break;
        case QTabWidget::South:
            lineRect = QRect(vpRect.left(), vpRect.bottom() + 1, vpRect.width(), 1);
            break;
        case QTabWidget::East:
            lineRect = QRect(vpRect.right() + 1, vpRect.top(), 1, vpRect.height());
            break;
        case QTabWidget::West:
            lineRect = QRect(vpRect.left() - 1, vpRect.top(), 1, vpRect.height());
            break;
        default:
            _tabBarBaseLine->hide();
            return;
    }

    if (!lineRect.isValid() || lineRect.isEmpty()) {
        _tabBarBaseLine->hide();
        return;
    }

    _tabBarBaseLine->setGeometry(lineRect);
    _tabBarBaseLine->clearMask();

    const QRect tabBarRect = _tabBar->geometry();
    QRegion mask;

    if (tabPosition() == QTabWidget::North || tabPosition() == QTabWidget::South) {
        const int tabLeft = qMax(0, tabBarRect.left() - lineRect.left());
        const int tabRight = qMin(lineRect.width() - 1, tabBarRect.right() - lineRect.left());

        if (tabLeft > 0)
            mask += QRect(0, 0, tabLeft, 1);
        if (tabRight + 1 < lineRect.width())
            mask += QRect(tabRight + 1, 0, lineRect.width() - (tabRight + 1), 1);
    } else {
        const int tabTop = qMax(0, tabBarRect.top() - lineRect.top());
        const int tabBottom = qMin(lineRect.height() - 1, tabBarRect.bottom() - lineRect.top());

        if (tabTop > 0)
            mask += QRect(0, 0, 1, tabTop);
        if (tabBottom + 1 < lineRect.height())
            mask += QRect(0, tabBottom + 1, 1, lineRect.height() - (tabBottom + 1));
    }

    _tabBarBaseLine->setMask(mask);
    _tabBarBaseLine->update();

    _tabBarBaseLine->show();
    _tabBarBaseLine->raise();
}

///
/// \brief MdiArea::enforceTabbedSubWindowState
/// \param wnd
///
void MdiArea::enforceTabbedSubWindowState(QMdiSubWindow* wnd)
{
    if (!wnd || viewMode() != QMdiArea::TabbedView)
        return;

    if(!wnd->isVisible())
        return;

    if (testOption(QMdiArea::DontMaximizeSubWindowOnActivation))
        return;

    if (wnd->isMinimized())
        wnd->showNormal();

    if (!wnd->isMaximized())
        wnd->showMaximized();
}

///
/// \brief MdiArea::subWindowAtIndex
/// \param index
/// \return
///
QMdiSubWindow* MdiArea::subWindowAtIndex(int index) const
{
    return _tabBar ? _tabBar->subWindowAt(index) : nullptr;
}

///
/// \brief MdiArea::on_tabBarContextMenu
/// \param pos
///
void MdiArea::on_tabBarContextMenu(const QPoint& pos)
{
    if (!_tabBar)
        return;

    auto* subWnd = subWindowAtIndex(_tabBar->tabAt(pos));
    if (!subWnd)
        return;

    emit tabContextMenuRequested(subWnd, _tabBar->mapToGlobal(pos));
}
