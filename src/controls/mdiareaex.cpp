#include "mdiareaex.h"
#include <QApplication>
#include <QBoxLayout>
#include <QLayout>
#include <QPainter>
#include <QPointer>
#include <QScrollBar>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QStyleOptionTabBarBase>
#include <QTimer>

///
/// \brief The TabBarBaseLineWidget class
///
class TabBarBaseLineWidget : public QFrame
{
public:
    explicit TabBarBaseLineWidget(MdiAreaEx* owner)
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
    MdiAreaEx* _owner;
};

static void setEqualSplitterSizes(QSplitter* splitter)
{
    if (!splitter)
        return;

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    const int total = splitter->orientation() == Qt::Horizontal
                          ? splitter->size().width()
                          : splitter->size().height();

    if (total > 1) {
        const int first = total / 2;
        splitter->setSizes({first, total - first});
    } else {
        QPointer<QSplitter> deferredSplitter(splitter);
        QTimer::singleShot(0, splitter, [deferredSplitter]() {
            if (deferredSplitter)
                setEqualSplitterSizes(deferredSplitter);
        });
    }
}

///
/// \brief MdiAreaEx::MdiAreaEx
/// \param parent
///
MdiAreaEx::MdiAreaEx(QWidget* parent, bool splitPanel)
    : QMdiArea(parent)
    , _isSecondaryPanel(splitPanel)
    , _tabBar(nullptr)
    , _splitButton(nullptr)
{
    _lastActiveArea = this;
    setViewMode(QMdiArea::TabbedView);
    connect(this, &QMdiArea::subWindowActivated, this, &MdiAreaEx::on_subWindowActivated);
}

///
/// \brief MdiAreaEx::~MdiAreaEx
///
MdiAreaEx::~MdiAreaEx()
{
    _destroying = true;
}

///
/// \brief MdiAreaEx::isSplitView
/// \return
///
bool MdiAreaEx::isSplitView() const
{
    return !_isSecondaryPanel && _splitter && _secondaryArea;
}

///
/// \brief MdiAreaEx::secondaryArea
/// \return
///
MdiAreaEx* MdiAreaEx::secondaryArea() const
{
    return _isSecondaryPanel ? nullptr : _secondaryArea;
}

///
/// \brief MdiAreaEx::toggleVerticalSplit
///
void MdiAreaEx::toggleVerticalSplit()
{
    setSplitViewEnabled(!isSplitView());
}

///
/// \brief MdiAreaEx::areaForSubWindow
/// \param wnd
/// \return
///
MdiAreaEx* MdiAreaEx::areaForSubWindow(QMdiSubWindow* wnd) const
{
    if (!wnd)
        return nullptr;

    if (QMdiArea::subWindowList().contains(wnd))
        return const_cast<MdiAreaEx*>(this);

    if (!_isSecondaryPanel && _secondaryArea && _secondaryArea->QMdiArea::subWindowList().contains(wnd))
        return _secondaryArea;

    return nullptr;
}

///
/// \brief MdiAreaEx::activePanel
/// \return
///
MdiAreaEx* MdiAreaEx::activePanel() const
{
    if (_isSecondaryPanel || !_secondaryArea)
        return const_cast<MdiAreaEx*>(this);

    QWidget* focus = QApplication::focusWidget();
    if (focus) {
        if (_secondaryArea->isAncestorOf(focus))
            return _secondaryArea;
        if (isAncestorOf(focus))
            return const_cast<MdiAreaEx*>(this);
    }

    if (_lastActiveArea == _secondaryArea || _lastActiveArea == this)
        return _lastActiveArea;

    return const_cast<MdiAreaEx*>(this);
}

///
/// \brief MdiAreaEx::eventFilter
/// \param obj
/// \param event
/// \return
///
bool MdiAreaEx::eventFilter(QObject* obj, QEvent* event)
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
        auto wnd = qobject_cast<QMdiSubWindow*>(obj);
        if (_tabBar)
            _tabBar->removeSubWindow(wnd);
    }
    return QMdiArea::eventFilter(obj, event);
}

///
/// \brief MdiAreaEx::addSubWindowLocal
/// \param widget
/// \param flags
/// \return
///
QMdiSubWindow* MdiAreaEx::addSubWindowLocal(QWidget* widget, Qt::WindowFlags flags)
{
    auto wnd = QMdiArea::addSubWindow(widget, flags);
    if (!wnd)
        return nullptr;

    wnd->installEventFilter(this);
    if (_tabBar)
        _tabBar->addSubWindow(wnd);

    updateTabBarGeometry();
    return wnd;
}

///
/// \brief MdiAreaEx::addSubWindow
/// \param widget
/// \param flags
/// \return
///
QMdiSubWindow* MdiAreaEx::addSubWindow(QWidget* widget, Qt::WindowFlags flags)
{
    if (!_isSecondaryPanel && _secondaryArea) {
        MdiAreaEx* target = activePanel();
        if (target && target != this)
            return target->addSubWindowLocal(widget, flags);
    }

    return addSubWindowLocal(widget, flags);
}

///
/// \brief MdiAreaEx::addSubWindowDirect
/// \param widget
/// \param flags
/// \return
///
QMdiSubWindow* MdiAreaEx::addSubWindowDirect(QWidget* widget, Qt::WindowFlags flags)
{
    return addSubWindowLocal(widget, flags);
}

///
/// \brief MdiAreaEx::removeSubWindowLocal
/// \param widget
///
void MdiAreaEx::removeSubWindowLocal(QWidget* widget)
{
    if (_tabBar) {
        auto wnd = qobject_cast<QMdiSubWindow*>(widget);
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
/// \brief MdiAreaEx::removeSubWindow
/// \param widget
///
void MdiAreaEx::removeSubWindow(QWidget* widget)
{
    if (!_isSecondaryPanel && _secondaryArea) {
        QMdiSubWindow* wnd = qobject_cast<QMdiSubWindow*>(widget);
        if (!wnd) {
            for (auto* candidate : QMdiArea::subWindowList()) {
                if (candidate && candidate->widget() == widget) {
                    wnd = candidate;
                    break;
                }
            }

            if (!wnd) {
                for (auto* candidate : _secondaryArea->QMdiArea::subWindowList()) {
                    if (candidate && candidate->widget() == widget) {
                        wnd = candidate;
                        break;
                    }
                }
            }
        }

        auto* owner = areaForSubWindow(wnd);
        if (owner && owner != this) {
            owner->removeSubWindowLocal(widget);
            return;
        }
    }

    removeSubWindowLocal(widget);
}

///
/// \brief MdiAreaEx::subWindowList
/// \param order
/// \return
///
QList<QMdiSubWindow*> MdiAreaEx::subWindowList(WindowOrder order) const
{
    auto list = QMdiArea::subWindowList(order);
    if (!_isSecondaryPanel && _secondaryArea)
        list.append(_secondaryArea->QMdiArea::subWindowList(order));

    return list;
}

///
/// \brief MdiAreaEx::localSubWindowList
/// \param order
/// \return
///
QList<QMdiSubWindow*> MdiAreaEx::localSubWindowList(WindowOrder order) const
{
    return QMdiArea::subWindowList(order);
}

///
/// \brief MdiAreaEx::currentSubWindow
/// \return
///
QMdiSubWindow* MdiAreaEx::currentSubWindow() const
{
    if (_isSecondaryPanel || !_secondaryArea)
        return QMdiArea::currentSubWindow();

    auto* panel = activePanel();
    if (panel == _secondaryArea) {
        if (auto* wnd = _secondaryArea->QMdiArea::currentSubWindow())
            return wnd;
    }

    if (auto* wnd = QMdiArea::currentSubWindow())
        return wnd;

    return _secondaryArea->QMdiArea::currentSubWindow();
}

///
/// \brief MdiAreaEx::activeSubWindow
/// \return
///
QMdiSubWindow* MdiAreaEx::activeSubWindow() const
{
    if (_isSecondaryPanel || !_secondaryArea)
        return QMdiArea::activeSubWindow();

    auto* panel = activePanel();
    if (panel == _secondaryArea) {
        if (auto* wnd = _secondaryArea->QMdiArea::activeSubWindow())
            return wnd;
    }

    if (auto* wnd = QMdiArea::activeSubWindow())
        return wnd;

    return _secondaryArea->QMdiArea::activeSubWindow();
}

///
/// \brief MdiAreaEx::setActiveSubWindow
/// \param wnd
///
void MdiAreaEx::setActiveSubWindow(QMdiSubWindow* wnd)
{
    if (!wnd) {
        QMdiArea::setActiveSubWindow(nullptr);
        if (!_isSecondaryPanel && _secondaryArea)
            _secondaryArea->QMdiArea::setActiveSubWindow(nullptr);
        return;
    }

    auto* owner = areaForSubWindow(wnd);
    if (owner && owner != this) {
        owner->QMdiArea::setActiveSubWindow(wnd);
        owner->setFocus(Qt::OtherFocusReason);
        _lastActiveArea = owner;
        return;
    }

    QMdiArea::setActiveSubWindow(wnd);
    _lastActiveArea = this;
}

///
/// \brief MdiAreaEx::closeAllSubWindows
///
void MdiAreaEx::closeAllSubWindows()
{
    QMdiArea::closeAllSubWindows();

    if (!_isSecondaryPanel && _secondaryArea)
        _secondaryArea->closeAllSubWindows();
}

///
/// \brief MdiAreaEx::setViewMode
/// \param mode
///
void MdiAreaEx::setViewMode(ViewMode mode)
{
    if (!_isSecondaryPanel && mode != QMdiArea::TabbedView && _secondaryArea)
        setSplitViewEnabled(false);

    QMdiArea::setViewMode(mode);

    if (mode == QMdiArea::TabbedView) {
        setupTabbedMode();
    } else {
        setViewportMargins(0, 0, 0, 0);

        if (_tabBar)
            _tabBar->hide();
        if (_splitButton)
            _splitButton->hide();
        if (_tabBarBaseLine)
            _tabBarBaseLine->hide();
    }

    if (!_isSecondaryPanel && _secondaryArea)
        _secondaryArea->setViewMode(mode);

    syncSplitButtonState();
}

///
/// \brief MdiAreaEx::setupTabbedMode
///
void MdiAreaEx::setupTabbedMode()
{
    if (_tabBar) {
        refreshTabBar();
        if (isVisible())
            _tabBar->show();
        createSplitButton();
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

    foreach (QMdiSubWindow* wnd, QMdiArea::subWindowList())
        _tabBar->addSubWindow(wnd);

    QMdiSubWindow* current = QMdiArea::currentSubWindow();
    if (current) {
        _tabBar->setCurrentSubWindow(current);

        if (current->isMaximized())
            current->showNormal();

        if (!testOption(QMdiArea::DontMaximizeSubWindowOnActivation))
            current->showMaximized();
    }

    if (isVisible())
        _tabBar->show();

    createSplitButton();
    updateTabBarGeometry();
    updateViewportBaseLine();

    connect(nativeTabBar, &QObject::destroyed, this, [this]() {
        if (_destroying)
            return;
        on_tabBarDestroyed();
    });
    connect(_tabBar, &QTabBar::tabBarClicked, this, &MdiAreaEx::on_tabBarClicked, Qt::UniqueConnection);
    connect(_tabBar, &QTabBar::currentChanged, this, &MdiAreaEx::on_currentTabChanged, Qt::UniqueConnection);
    connect(_tabBar, &QTabBar::tabCloseRequested, this, &MdiAreaEx::on_closeTab, Qt::UniqueConnection);
    connect(_tabBar, &QTabBar::tabMoved, this, &MdiAreaEx::on_moveTab, Qt::UniqueConnection);
}

///
/// \brief MdiTabBar::createSplitButton
///
void MdiAreaEx::createSplitButton()
{
    if (_isSecondaryPanel)
        return;

    if (_splitButton) {
        _splitButton->raise();
        syncSplitButtonState();
        return;
    }

    _splitButton = new QToolButton(this);
    _splitButton->setAutoRaise(true);
    _splitButton->setIcon(QIcon(":/res/actionSplitView.png"));
    _splitButton->setToolTip(tr("Split view"));
    _splitButton->setCheckable(true);

    const QSize sh = _splitButton->sizeHint();
    const int btnSize = qMax(20, qMin(sh.width(), sh.height()));
    _splitButton->setFixedSize(btnSize, btnSize);

    connect(_splitButton, &QToolButton::toggled, this, [this](bool checked) {
        if (_isSplitInProgress)
            return;
        setSplitViewEnabled(checked);
    });

    _splitButton->raise();
    syncSplitButtonState();
}

///
/// \brief MdiAreaEx::shouldShowSplitButton
/// \return
///
bool MdiAreaEx::shouldShowSplitButton() const
{
    return !_isSecondaryPanel &&
           viewMode() == QMdiArea::TabbedView &&
           !subWindowList().isEmpty();
}

///
/// \brief MdiAreaEx::syncSplitButtonState
///
void MdiAreaEx::syncSplitButtonState()
{
    if (!_splitButton)
        return;

    const bool visible = shouldShowSplitButton();
    _splitButton->setVisible(visible);

    QSignalBlocker blocker(_splitButton);
    _splitButton->setChecked(isSplitView());
}

///
/// \brief MdiAreaEx::ensureSplitArea
/// \param orientation
///
void MdiAreaEx::ensureSplitArea(Qt::Orientation orientation)
{
    if (_isSecondaryPanel)
        return;

    if (_splitter && _secondaryArea) {
        _splitter->setOrientation(orientation);
        setEqualSplitterSizes(_splitter);
        return;
    }

    QWidget* hostParent = parentWidget();
    auto* hostLayout = hostParent ? hostParent->layout() : nullptr;
    if (!hostParent || !hostLayout)
        return;

    const int hostIndex = hostLayout->indexOf(this);

    _splitter = new QSplitter(orientation, hostParent);
    _splitter->setChildrenCollapsible(false);

    hostLayout->removeWidget(this);
    setParent(_splitter);
    _splitter->addWidget(this);

    _secondaryArea = new MdiAreaEx(_splitter, true);
    _secondaryArea->setActivationOrder(activationOrder());
    _secondaryArea->setBackground(background());
    _secondaryArea->_tabsExpanding = _tabsExpanding;
    _secondaryArea->QMdiArea::setDocumentMode(documentMode());
    _secondaryArea->QMdiArea::setTabsClosable(tabsClosable());
    _secondaryArea->QMdiArea::setTabsMovable(tabsMovable());
    _secondaryArea->QMdiArea::setTabPosition(tabPosition());
    _secondaryArea->QMdiArea::setTabShape(tabShape());
    _secondaryArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation,
                              testOption(QMdiArea::DontMaximizeSubWindowOnActivation));
    _secondaryArea->setViewMode(viewMode());

    _splitter->addWidget(_secondaryArea);

    if (auto* boxLayout = qobject_cast<QBoxLayout*>(hostLayout)) {
        if (hostIndex >= 0)
            boxLayout->insertWidget(hostIndex, _splitter);
        else
            boxLayout->addWidget(_splitter);
    } else {
        hostLayout->addWidget(_splitter);
    }

    setEqualSplitterSizes(_splitter);
    QTimer::singleShot(0, _splitter, [this]() { setEqualSplitterSizes(_splitter); });

    connect(_secondaryArea, &QMdiArea::subWindowActivated, this, [this](QMdiSubWindow* wnd) {
        if (_isSplitInProgress)
            return;

        _lastActiveArea = _secondaryArea;
        emit subWindowActivated(wnd);
    });
}

///
/// \brief MdiAreaEx::mergeSplitArea
///
void MdiAreaEx::mergeSplitArea()
{
    if (_isSecondaryPanel || !_splitter || !_secondaryArea)
        return;

    _isSplitInProgress = true;
    const auto secondaryWindows = _secondaryArea->QMdiArea::subWindowList();
    for (auto* wnd : secondaryWindows) {
        _secondaryArea->removeSubWindowLocal(wnd);
        addSubWindowLocal(wnd, Qt::WindowFlags());
    }

    QWidget* hostParent = _splitter->parentWidget();
    auto* hostLayout = hostParent ? hostParent->layout() : nullptr;
    if (hostParent) {
        const int hostIndex = hostLayout ? hostLayout->indexOf(_splitter) : -1;
        if (hostLayout)
            hostLayout->removeWidget(_splitter);

        setParent(hostParent);

        if (auto* boxLayout = qobject_cast<QBoxLayout*>(hostLayout)) {
            if (hostIndex >= 0)
                boxLayout->insertWidget(hostIndex, this);
            else
                boxLayout->addWidget(this);
        } else if (hostLayout) {
            hostLayout->addWidget(this);
        } else {
            setGeometry(_splitter->geometry());
            show();
        }
    }

    _secondaryArea->deleteLater();
    _secondaryArea = nullptr;

    _splitter->deleteLater();
    _splitter = nullptr;

    _lastActiveArea = this;
    _isSplitInProgress = false;
    syncSplitButtonState();
    updateTabBarGeometry();
}

///
/// \brief MdiAreaEx::setSplitViewEnabled
/// \param enabled
///
void MdiAreaEx::setSplitViewEnabled(bool enabled)
{
    if (_isSecondaryPanel || viewMode() != QMdiArea::TabbedView)
        return;

    const bool wasSplit = isSplitView();

    if (enabled) {
        ensureSplitArea(Qt::Horizontal);
        if (!_secondaryArea)
            return;

        setEqualSplitterSizes(_splitter);
        QTimer::singleShot(0, _splitter, [this]() { setEqualSplitterSizes(_splitter); });
    } else {
        emit splitViewAboutToDisable();
        mergeSplitArea();
    }

    syncSplitButtonState();

    if (wasSplit != isSplitView())
        emit splitViewToggled(isSplitView());
}

///
/// \brief MdiAreaEx::on_tabBarClicked
/// \param index
///
void MdiAreaEx::on_tabBarClicked(int index)
{
    if (!_tabBar)
        return;

    auto wnd = _tabBar->subWindowAt(index);
    if (wnd)
        setActiveSubWindow(wnd);

    setFocus(Qt::OtherFocusReason);
}

///
/// \brief MdiAreaEx::on_currentTabChanged
/// \param index
///
void MdiAreaEx::on_currentTabChanged(int index)
{
    if (!_tabBar)
        return;

    auto wnd = _tabBar->subWindowAt(index);
    if (wnd)
        setActiveSubWindow(wnd);
}

///
/// \brief MdiAreaEx::on_closeTab
/// \param index
///
void MdiAreaEx::on_closeTab(int index)
{
    if (!_tabBar)
        return;

    auto wnd = _tabBar->subWindowAt(index);
    if (wnd)
        wnd->close();
}

///
/// \brief MdiAreaEx::on_moveTab
/// \param from
/// \param to
///
void MdiAreaEx::on_moveTab(int from, int to)
{
    Q_UNUSED(from)
    Q_UNUSED(to)
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

    if (_splitButton)
        _splitButton->hide();

    if (_tabBarBaseLine)
        _tabBarBaseLine->hide();

    updateViewportBaseLine();
}

///
/// \brief MdiAreaEx::on_subWindowActivated
///
void MdiAreaEx::on_subWindowActivated(QMdiSubWindow* wnd)
{
    if (!_tabBar)
        return;

    _tabBar->setCurrentSubWindow(wnd);

    if (!_isSecondaryPanel && wnd) {
        if (auto* owner = areaForSubWindow(wnd))
            _lastActiveArea = owner;
    }

    updateTabBarGeometry();
}

///
/// \brief MdiAreaEx::refreshTabBar
///
void MdiAreaEx::refreshTabBar()
{
    if (_tabBar) {
        _tabBar->setDocumentMode(documentMode());
        _tabBar->setTabsClosable(tabsClosable());
        _tabBar->setExpanding(tabsExpanding());
        _tabBar->setMovable(tabsMovable());
        updateTabBarGeometry();
    }

    if (!_isSecondaryPanel && _secondaryArea) {
        _secondaryArea->_tabsExpanding = _tabsExpanding;
        _secondaryArea->QMdiArea::setDocumentMode(documentMode());
        _secondaryArea->QMdiArea::setTabsClosable(tabsClosable());
        _secondaryArea->QMdiArea::setTabsMovable(tabsMovable());
        _secondaryArea->refreshTabBar();
    }
}

///
/// \brief MdiAreaEx::updateTabBarGeometry
///
void MdiAreaEx::updateTabBarGeometry()
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

    const bool showSplitButton = _splitButton && shouldShowSplitButton();
    QWidget* splitButtonHost = this;
    if (showSplitButton && isSplitView() && _splitter && _splitter->parentWidget())
        splitButtonHost = _splitter->parentWidget();

    if (showSplitButton && _splitButton->parentWidget() != splitButtonHost)
        _splitButton->setParent(splitButtonHost);

    const bool splitButtonInPrimaryArea = showSplitButton && _splitButton->parentWidget() == this;
    const int splitWidth = splitButtonInPrimaryArea ? (_splitButton->width() + 4) : 0;

    QRect tabBarRect;
    QRect buttonRect;
    QMargins desiredMargins = viewportMargins();

    switch (tabPosition()) {
        case QTabWidget::North:
            desiredMargins = QMargins(0, tabBarSizeHint.height(), 0, 0);
            tabBarRect = QRect(0, 0, areaWidth - splitWidth, tabBarSizeHint.height());
            if (splitButtonInPrimaryArea) {
                buttonRect = QRect(areaWidth - splitWidth,
                                   (tabBarSizeHint.height() - _splitButton->height()) / 2,
                                   _splitButton->width(),
                                   _splitButton->height());
            }
            break;
        case QTabWidget::South:
            desiredMargins = QMargins(0, 0, 0, tabBarSizeHint.height());
            tabBarRect = QRect(0, areaHeight - tabBarSizeHint.height(), areaWidth - splitWidth, tabBarSizeHint.height());
            if (splitButtonInPrimaryArea) {
                buttonRect = QRect(areaWidth - splitWidth,
                                   areaHeight - tabBarSizeHint.height() +
                                       (tabBarSizeHint.height() - _splitButton->height()) / 2,
                                   _splitButton->width(),
                                   _splitButton->height());
            }
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

    if (_splitButton) {
        _splitButton->setVisible(showSplitButton);
        if (showSplitButton) {
            if (!splitButtonInPrimaryArea && _splitter && splitButtonHost == _splitter->parentWidget()) {
                const QPoint topLeftOnHost = mapTo(splitButtonHost, QPoint(0, 0));
                const QRect splitterRectOnHost = _splitter->geometry();
                const int rightX = splitterRectOnHost.right() - (_splitButton->width() + 3);

                if (tabPosition() == QTabWidget::South) {
                    const int y = topLeftOnHost.y() + areaHeight - tabBarSizeHint.height() +
                                  (tabBarSizeHint.height() - _splitButton->height()) / 2;
                    buttonRect = QRect(rightX, y, _splitButton->width(), _splitButton->height());
                } else {
                    const int y = topLeftOnHost.y() + (tabBarSizeHint.height() - _splitButton->height()) / 2;
                    buttonRect = QRect(rightX, y, _splitButton->width(), _splitButton->height());
                }
            }

            _splitButton->setGeometry(buttonRect);
            _splitButton->raise();
        }
    }

    updateViewportBaseLine();
}

///
/// \brief MdiAreaEx::updateViewportBaseLine
///
void MdiAreaEx::updateViewportBaseLine()
{
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
    if (_tabBar)
        _tabBar->setVisible(visible && viewMode() == QMdiArea::TabbedView);
    if (_splitButton)
        _splitButton->setVisible(visible && shouldShowSplitButton());
    updateViewportBaseLine();
}

///
/// \brief MdiAreaEx::subWindowAtIndex
/// \param index
/// \return
///
QMdiSubWindow* MdiAreaEx::subWindowAtIndex(int index) const
{
    return _tabBar ? _tabBar->subWindowAt(index) : nullptr;
}

///
/// \brief MdiAreaEx::splitTab
/// \param index
/// \param orientation
///
void MdiAreaEx::splitTab(int index, Qt::Orientation orientation)
{
    if (_isSecondaryPanel || viewMode() != QMdiArea::TabbedView)
        return;

    QMdiSubWindow* sw = subWindowAtIndex(index);
    if (!sw)
        return;

    ensureSplitArea(orientation);
    if (!_secondaryArea)
        return;

    auto* owner = areaForSubWindow(sw);
    if (owner && owner != _secondaryArea) {
        owner->removeSubWindowLocal(sw);
        _secondaryArea->addSubWindowLocal(sw, Qt::WindowFlags());
    }

    _secondaryArea->QMdiArea::setActiveSubWindow(sw);
    _secondaryArea->setFocus(Qt::OtherFocusReason);
    _lastActiveArea = _secondaryArea;
    syncSplitButtonState();
}
