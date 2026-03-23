#include <QApplication>
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLayout>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QStyle>
#include <QTimer>
#include "mdiareaex.h"
#include "mditabbar.h"

///
/// \brief setEqualSplitterSizes
/// \param splitter
/// \return
///
static bool setEqualSplitterSizes(QSplitter* splitter)
{
    if (!splitter)
        return false;

    if (splitter->count() < 2)
        return false;

    const int total = splitter->orientation() == Qt::Horizontal
                          ? splitter->size().width()
                          : splitter->size().height();

    if (total <= 1)
        return false;

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    const int handleWidth = qMax(0, splitter->handleWidth());
    const int available = qMax(2, total - handleWidth);
    const int first = available / 2;
    splitter->setSizes({first, available - first});
    return true;
}

///
/// \brief MdiAreaEx::MdiAreaEx
/// \param parent
///
MdiAreaEx::MdiAreaEx(QWidget* parent)
    : QWidget(parent)
{
    auto* hostLayout = new QHBoxLayout(this);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->setSpacing(0);

    _primaryArea = new MdiArea(this);
    _primaryArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    _primaryArea->setMinimumSize(0, 0);
    hostLayout->addWidget(_primaryArea);

    _activePanel = _primaryArea;

    connectPanel(_primaryArea);
    createSplitButton();
    syncSplitButtonState();
    updateSplitButtonGeometry();

    connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget* now) {
        if (!now || !_secondaryArea)
            return;
        if (_secondaryArea->isAncestorOf(now)) {
            _activePanel = _secondaryArea;
        }
        else if (_primaryArea->isAncestorOf(now)) {
            _activePanel = _primaryArea;
        }
    });
}

///
/// \brief MdiAreaEx::~MdiAreaEx
///
MdiAreaEx::~MdiAreaEx()
{
    _destroying = true;
}

///
/// \brief MdiAreaEx::addSubWindow
/// \param widget
/// \param flags
/// \return
///
QMdiSubWindow* MdiAreaEx::addSubWindow(QWidget* widget, Qt::WindowFlags flags)
{
    if (!_primaryArea)
        return nullptr;

    auto* wnd = activePanel()->addSubWindow(widget, flags);
    syncSplitButtonState();
    updateSplitButtonGeometry();
    return wnd;
}

///
/// \brief MdiAreaEx::removeSubWindow
/// \param widget
///
void MdiAreaEx::removeSubWindow(QWidget* widget)
{
    if (!_primaryArea || !widget)
        return;

    QMdiSubWindow* wnd = qobject_cast<QMdiSubWindow*>(widget);
    if (!wnd) {
        for (auto* candidate : _primaryArea->localSubWindowList()) {
            if (candidate && candidate->widget() == widget) {
                wnd = candidate;
                break;
            }
        }

        if (!wnd && _secondaryArea) {
            for (auto* candidate : _secondaryArea->localSubWindowList()) {
                if (candidate && candidate->widget() == widget) {
                    wnd = candidate;
                    break;
                }
            }
        }
    }

    auto* owner = areaForSubWindow(wnd);
    if (!owner)
        owner = _primaryArea;

    owner->removeSubWindow(widget);
    syncSplitButtonState();
    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::subWindowList
/// \param order
/// \return
///
QList<QMdiSubWindow*> MdiAreaEx::subWindowList(QMdiArea::WindowOrder order) const
{
    QList<QMdiSubWindow*> list;

    if (_primaryArea)
        list = _primaryArea->localSubWindowList(order);

    if (_secondaryArea)
        list.append(_secondaryArea->localSubWindowList(order));

    return list;
}

///
/// \brief MdiAreaEx::localSubWindowList
/// \param order
/// \return
///
QList<QMdiSubWindow*> MdiAreaEx::localSubWindowList(QMdiArea::WindowOrder order) const
{
    return _primaryArea ? _primaryArea->localSubWindowList(order) : QList<QMdiSubWindow*>();
}

///
/// \brief MdiAreaEx::currentSubWindow
/// \return
///
QMdiSubWindow* MdiAreaEx::currentSubWindow() const
{
    if (!_primaryArea)
        return nullptr;

    if (!_secondaryArea)
        return _primaryArea->currentSubWindow();

    auto* panel = activePanel();
    if (panel == _secondaryArea) {
        if (auto* wnd = _secondaryArea->currentSubWindow())
            return wnd;
    }

    if (auto* wnd = _primaryArea->currentSubWindow())
        return wnd;

    return _secondaryArea->currentSubWindow();
}

///
/// \brief MdiAreaEx::activeSubWindow
/// \return
///
QMdiSubWindow* MdiAreaEx::activeSubWindow() const
{
    if (!_primaryArea)
        return nullptr;

    if (!_secondaryArea)
        return _primaryArea->activeSubWindow();

    auto* panel = activePanel();
    if (panel == _secondaryArea) {
        if (auto* wnd = _secondaryArea->activeSubWindow())
            return wnd;
    }

    if (auto* wnd = _primaryArea->activeSubWindow())
        return wnd;

    return _secondaryArea->activeSubWindow();
}

///
/// \brief MdiAreaEx::activePrimarySubWindow
/// Returns the best available active subwindow of the primary panel, trying
/// multiple sources in order: activeSubWindow, currentSubWindow, MdiTabBar,
/// and finally the window saved before the last split operation.
/// \return
///
QMdiSubWindow* MdiAreaEx::activePrimarySubWindow() const
{
    if (!_primaryArea)
        return nullptr;

    if (auto* wnd = _primaryArea->activeSubWindow())
        return wnd;
    if (auto* wnd = _primaryArea->currentSubWindow())
        return wnd;
    if (auto* tb = qobject_cast<MdiTabBar*>(_primaryArea->tabBar()))
        if (auto* wnd = tb->currentSubWindow())
            return wnd;

    return _preSplitActiveWindow;
}

///
/// \brief MdiAreaEx::setActiveSubWindow
/// \param wnd
///
void MdiAreaEx::setActiveSubWindow(QMdiSubWindow* wnd)
{
    if (!_primaryArea)
        return;

    if (!wnd) {
        _primaryArea->setActiveSubWindow(nullptr);
        if (_secondaryArea)
            _secondaryArea->setActiveSubWindow(nullptr);
        return;
    }

    if (auto* owner = areaForSubWindow(wnd)) {
        owner->setActiveSubWindow(wnd);
        owner->setFocus(Qt::OtherFocusReason);
        _activePanel = owner;
        return;
    }

    _primaryArea->setActiveSubWindow(wnd);
    _primaryArea->setFocus(Qt::OtherFocusReason);
    _activePanel = _primaryArea;
}

///
/// \brief MdiAreaEx::closeAllSubWindows
///
void MdiAreaEx::closeAllSubWindows()
{
    if (_primaryArea)
        _primaryArea->closeAllSubWindows();

    if (_secondaryArea)
        _secondaryArea->closeAllSubWindows();

    syncSplitButtonState();
    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::setViewMode
/// \param mode
///
void MdiAreaEx::setViewMode(QMdiArea::ViewMode mode)
{
    if (!_primaryArea)
        return;

    if (mode != QMdiArea::TabbedView && _secondaryArea)
        setSplitViewEnabled(false);

    _primaryArea->setViewMode(mode);

    if (_secondaryArea)
        _secondaryArea->setViewMode(mode);

    syncSplitButtonState();
    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::viewMode
/// \return
///
QMdiArea::ViewMode MdiAreaEx::viewMode() const
{
    return _primaryArea ? _primaryArea->viewMode() : QMdiArea::TabbedView;
}

///
/// \brief MdiAreaEx::toggleVerticalSplit
///
void MdiAreaEx::toggleVerticalSplit()
{
    setSplitViewEnabled(!isSplitView());
}

///
/// \brief MdiAreaEx::isSplitView
/// \return
///
bool MdiAreaEx::isSplitView() const
{
    return _splitter && _secondaryArea;
}

///
/// \brief MdiAreaEx::primaryArea
/// \return
///
MdiArea* MdiAreaEx::primaryArea() const
{
    return _primaryArea;
}

///
/// \brief MdiAreaEx::secondaryArea
/// \return
///
MdiArea* MdiAreaEx::secondaryArea() const
{
    return _secondaryArea;
}

///
/// \brief MdiAreaEx::tabBar
/// \return
///
QTabBar* MdiAreaEx::tabBar() const
{
    return _primaryArea ? _primaryArea->tabBar() : nullptr;
}

///
/// \brief MdiAreaEx::documentMode
/// \return
///
bool MdiAreaEx::documentMode() const
{
    return _primaryArea ? _primaryArea->documentMode() : false;
}

///
/// \brief MdiAreaEx::setDocumentMode
/// \param enabled
///
void MdiAreaEx::setDocumentMode(bool enabled)
{
    if (_primaryArea)
        _primaryArea->setDocumentMode(enabled);

    if (_secondaryArea)
        _secondaryArea->setDocumentMode(enabled);

    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::tabsClosable
/// \return
///
bool MdiAreaEx::tabsClosable() const
{
    return _primaryArea ? _primaryArea->tabsClosable() : false;
}

///
/// \brief MdiAreaEx::setTabsClosable
/// \param closable
///
void MdiAreaEx::setTabsClosable(bool closable)
{
    if (_primaryArea)
        _primaryArea->setTabsClosable(closable);

    if (_secondaryArea)
        _secondaryArea->setTabsClosable(closable);

    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::tabsMovable
/// \return
///
bool MdiAreaEx::tabsMovable() const
{
    return _primaryArea ? _primaryArea->tabsMovable() : false;
}

///
/// \brief MdiAreaEx::setTabsMovable
/// \param movable
///
void MdiAreaEx::setTabsMovable(bool movable)
{
    if (_primaryArea)
        _primaryArea->setTabsMovable(movable);

    if (_secondaryArea)
        _secondaryArea->setTabsMovable(movable);

    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::tabsExpanding
/// \return
///
bool MdiAreaEx::tabsExpanding() const
{
    return _primaryArea ? _primaryArea->tabsExpanding() : false;
}

///
/// \brief MdiAreaEx::setTabsExpanding
/// \param expanding
///
void MdiAreaEx::setTabsExpanding(bool expanding)
{
    if (_primaryArea)
        _primaryArea->setTabsExpanding(expanding);

    if (_secondaryArea)
        _secondaryArea->setTabsExpanding(expanding);

    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::setActivationOrder
/// \param order
///
void MdiAreaEx::setActivationOrder(QMdiArea::WindowOrder order)
{
    if (_primaryArea)
        _primaryArea->setActivationOrder(order);

    if (_secondaryArea)
        _secondaryArea->setActivationOrder(order);
}

///
/// \brief MdiAreaEx::activationOrder
/// \return
///
QMdiArea::WindowOrder MdiAreaEx::activationOrder() const
{
    return _primaryArea ? _primaryArea->activationOrder() : QMdiArea::ActivationHistoryOrder;
}

///
/// \brief MdiAreaEx::setBackground
/// \param background
///
void MdiAreaEx::setBackground(const QBrush& background)
{
    if (_primaryArea)
        _primaryArea->setBackground(background);

    if (_secondaryArea)
        _secondaryArea->setBackground(background);
}

///
/// \brief MdiAreaEx::background
/// \return
///
QBrush MdiAreaEx::background() const
{
    return _primaryArea ? _primaryArea->background() : QBrush();
}

///
/// \brief MdiAreaEx::setOption
/// \param option
/// \param on
///
void MdiAreaEx::setOption(QMdiArea::AreaOption option, bool on)
{
    if (_primaryArea)
        _primaryArea->setOption(option, on);

    if (_secondaryArea)
        _secondaryArea->setOption(option, on);
}

///
/// \brief MdiAreaEx::testOption
/// \param option
/// \return
///
bool MdiAreaEx::testOption(QMdiArea::AreaOption option) const
{
    return _primaryArea && _primaryArea->testOption(option);
}

///
/// \brief MdiAreaEx::setTabPosition
/// \param position
///
void MdiAreaEx::setTabPosition(QTabWidget::TabPosition position)
{
    if (_primaryArea)
        _primaryArea->setTabPosition(position);

    if (_secondaryArea)
        _secondaryArea->setTabPosition(position);

    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::tabPosition
/// \return
///
QTabWidget::TabPosition MdiAreaEx::tabPosition() const
{
    return _primaryArea ? _primaryArea->tabPosition() : QTabWidget::North;
}

///
/// \brief MdiAreaEx::setTabShape
/// \param shape
///
void MdiAreaEx::setTabShape(QTabWidget::TabShape shape)
{
    if (_primaryArea)
        _primaryArea->setTabShape(shape);

    if (_secondaryArea)
        _secondaryArea->setTabShape(shape);

    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::tabShape
/// \return
///
QTabWidget::TabShape MdiAreaEx::tabShape() const
{
    return _primaryArea ? _primaryArea->tabShape() : QTabWidget::Rounded;
}

///
/// \brief MdiAreaEx::cascadeSubWindows
///
void MdiAreaEx::cascadeSubWindows()
{
    if (_primaryArea)
        _primaryArea->cascadeSubWindows();

    if (_secondaryArea)
        _secondaryArea->cascadeSubWindows();
}

///
/// \brief MdiAreaEx::tileSubWindows
///
void MdiAreaEx::tileSubWindows()
{
    if (_primaryArea)
        _primaryArea->tileSubWindows();

    if (_secondaryArea)
        _secondaryArea->tileSubWindows();
}

///
/// \brief MdiAreaEx::eventFilter
/// \param obj
/// \param event
/// \return
///
bool MdiAreaEx::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == _splitter) {
        switch (event->type()) {
            case QEvent::Show:
            case QEvent::Resize:
            case QEvent::LayoutRequest:
                tryEqualizeSplitterSizes();
                updateSplitButtonGeometry();
                break;
            default:
                break;
        }
    }

    return QWidget::eventFilter(obj, event);
}

///
/// \brief MdiAreaEx::resizeEvent
/// \param event
///
void MdiAreaEx::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    tryEqualizeSplitterSizes();
    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::setVisible
/// \param visible
///
void MdiAreaEx::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (_splitButton)
        _splitButton->setVisible(visible && shouldShowSplitButton());
    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::on_panelSubWindowActivated
/// \param wnd
///
void MdiAreaEx::on_panelSubWindowActivated(QMdiSubWindow* wnd)
{
    if (_isSplitInProgress)
        return;

    if (wnd) {
        if (auto* owner = areaForSubWindow(wnd))
            _activePanel = owner;
    }

    emit subWindowActivated(wnd);
    syncSplitButtonState();
    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::on_panelTabBarLayoutChanged
///
void MdiAreaEx::on_panelTabBarLayoutChanged()
{
    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::areaForSubWindow
/// \param wnd
/// \return
///
MdiArea* MdiAreaEx::areaForSubWindow(QMdiSubWindow* wnd) const
{
    if (!wnd)
        return nullptr;

    if (_primaryArea && _primaryArea->localSubWindowList().contains(wnd))
        return _primaryArea;

    if (_secondaryArea && _secondaryArea->localSubWindowList().contains(wnd))
        return _secondaryArea;

    return nullptr;
}

///
/// \brief MdiAreaEx::activePanel
/// \return
///
MdiArea* MdiAreaEx::activePanel() const
{
    return _activePanel ? _activePanel : _primaryArea;
}

///
/// \brief MdiAreaEx::connectPanel
/// \param area
///
void MdiAreaEx::connectPanel(MdiArea* area)
{
    if (!area)
        return;

    connect(area, &QMdiArea::subWindowActivated, this, &MdiAreaEx::on_panelSubWindowActivated, Qt::UniqueConnection);
    connect(area, &MdiArea::tabBarLayoutChanged, this, &MdiAreaEx::on_panelTabBarLayoutChanged, Qt::UniqueConnection);
    connect(area, &MdiArea::tabsReordered, this, &MdiAreaEx::tabsReordered, Qt::UniqueConnection);
}

///
/// \brief MdiAreaEx::syncPanelOptions
/// \param area
///
void MdiAreaEx::syncPanelOptions(MdiArea* area) const
{
    if (!area || !_primaryArea)
        return;

    area->setActivationOrder(_primaryArea->activationOrder());
    area->setBackground(_primaryArea->background());
    area->setDocumentMode(_primaryArea->documentMode());
    area->setTabsClosable(_primaryArea->tabsClosable());
    area->setTabsMovable(_primaryArea->tabsMovable());
    area->setTabsExpanding(_primaryArea->tabsExpanding());
    area->setTabPosition(_primaryArea->tabPosition());
    area->setTabShape(_primaryArea->tabShape());
    area->setOption(QMdiArea::DontMaximizeSubWindowOnActivation,
                    _primaryArea->testOption(QMdiArea::DontMaximizeSubWindowOnActivation));
    area->setTabBarTrailingInset(0);
}

///
/// \brief MdiAreaEx::createSplitButton
///
void MdiAreaEx::createSplitButton()
{
    if (_splitButton) {
        _splitButton->raise();
        syncSplitButtonState();
        return;
    }

    _splitButton = new QToolButton(this);
    _splitButton->setAutoRaise(true);
    _splitButton->setIcon(QIcon(":/res/actionSplitView.png"));
    _splitButton->setToolTip(QStringLiteral("Split view"));
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
    return _primaryArea &&
           _primaryArea->viewMode() == QMdiArea::TabbedView &&
           !subWindowList().isEmpty();
}

///
/// \brief MdiAreaEx::syncSplitButtonState
///
void MdiAreaEx::syncSplitButtonState()
{
    if (!_splitButton)
        return;

    const bool visible = shouldShowSplitButton() && isVisible();
    _splitButton->setVisible(visible);

    QSignalBlocker blocker(_splitButton);
    _splitButton->setChecked(isSplitView());
}

///
/// \brief MdiAreaEx::updateSplitButtonGeometry
///
void MdiAreaEx::updateSplitButtonGeometry()
{
    if (!_splitButton || !_primaryArea || _updatingSplitButtonGeometry)
        return;

    QScopedValueRollback<bool> guard(_updatingSplitButtonGeometry, true);

    auto clearTabBarInsets = [this]() {
        if (_primaryArea)
            _primaryArea->setTabBarTrailingInset(0);
        if (_secondaryArea)
            _secondaryArea->setTabBarTrailingInset(0);
    };

    const bool showSplitButton = shouldShowSplitButton() && isVisible();
    if (!showSplitButton) {
        clearTabBarInsets();
        _splitButton->hide();
        return;
    }

    if (_primaryArea->viewMode() != QMdiArea::TabbedView) {
        clearTabBarInsets();
        _splitButton->hide();
        return;
    }

    auto* primaryTabBar = _primaryArea->tabBar();

    MdiArea* placementArea = _primaryArea;
    if (isSplitView() && _secondaryArea)
        placementArea = _secondaryArea;

    auto* placementTabBar = placementArea ? placementArea->tabBar() : nullptr;
    const bool placementTabBarVisible = placementTabBar && placementTabBar->isVisible();

    QTabBar* anchorTabBar = placementTabBarVisible
                                ? placementTabBar
                                : ((primaryTabBar && primaryTabBar->isVisible()) ? primaryTabBar : nullptr);
    if (!anchorTabBar) {
        clearTabBarInsets();
        _splitButton->hide();
        return;
    }

    QWidget* splitButtonHost = _primaryArea;
    if (isSplitView() && _splitter && _splitter->parentWidget())
        splitButtonHost = _splitter->parentWidget();

    if (_splitButton->parentWidget() != splitButtonHost)
        _splitButton->setParent(splitButtonHost);

    int styleGap = -1;
    if (_splitButton->style())
        styleGap = _splitButton->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing, nullptr, _splitButton);
    if (styleGap <= 0 && splitButtonHost && splitButtonHost->style())
        styleGap = splitButtonHost->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing, nullptr, splitButtonHost);
    if (styleGap <= 0)
        styleGap = 4;

    const int splitWidth = _splitButton->width() + styleGap;

    MdiArea* insetOwner = placementTabBarVisible ? placementArea : nullptr;
    clearTabBarInsets();
    if (insetOwner)
        insetOwner->setTabBarTrailingInset(splitWidth);

    QRect buttonRect;
    QRect reserveRect;
    if (insetOwner) {
        auto* insetTabBar = insetOwner->tabBar();
        if (insetTabBar && insetTabBar->isVisible()) {
            const QPoint topLeftOnHost = insetTabBar->mapTo(splitButtonHost, QPoint(0, 0));
            const QRect tabBarRectOnHost = QRect(topLeftOnHost, insetTabBar->size());
            reserveRect = QRect(tabBarRectOnHost.right() + 1, tabBarRectOnHost.y(), splitWidth, tabBarRectOnHost.height());
        }
    } else if (placementArea) {
        const QPoint areaTopLeftOnHost = placementArea->mapTo(splitButtonHost, QPoint(0, 0));
        const QPoint anchorTopLeftOnHost = anchorTabBar->mapTo(splitButtonHost, QPoint(0, 0));
        reserveRect = QRect(areaTopLeftOnHost.x() + qMax(0, placementArea->width() - splitWidth),
                            anchorTopLeftOnHost.y(),
                            splitWidth,
                            anchorTabBar->height());
    }

    if (reserveRect.isValid() && !reserveRect.isEmpty()) {
        const int x = reserveRect.x() + qMax(0, (reserveRect.width() - _splitButton->width()) / 2);
        const int y = reserveRect.y() + (reserveRect.height() - _splitButton->height()) / 2;
        buttonRect = QRect(x, y, _splitButton->width(), _splitButton->height());
    }

    if (buttonRect.isValid() && !buttonRect.isEmpty()) {
        _splitButton->setGeometry(buttonRect);
        _splitButton->show();
        _splitButton->raise();
    } else {
        _splitButton->hide();
    }
}

///
/// \brief MdiAreaEx::setSplitViewEnabled
/// \param enabled
///
void MdiAreaEx::setSplitViewEnabled(bool enabled)
{
    if (!_primaryArea || _primaryArea->viewMode() != QMdiArea::TabbedView)
        return;

    const bool wasSplit = isSplitView();

    if (enabled) {
        // Save the active window before ensureSplitArea reparents _primaryArea,
        // which resets QMdiArea's internal active/current window state.
        _preSplitActiveWindow = activePrimarySubWindow();

        ensureSplitArea(Qt::Horizontal);
        if (!_secondaryArea)
            return;

        requestEqualSplitterSizes();
    } else {
        emit splitViewAboutToDisable();
        mergeSplitArea();
    }

    syncSplitButtonState();
    updateSplitButtonGeometry();

    if (wasSplit != isSplitView())
        emit splitViewToggled(isSplitView());
}

///
/// \brief MdiAreaEx::ensureSplitArea
/// \param orientation
///
void MdiAreaEx::ensureSplitArea(Qt::Orientation orientation)
{
    if (!_primaryArea)
        return;

    if (_splitter && _secondaryArea) {
        _splitter->setOrientation(orientation);
        requestEqualSplitterSizes();
        updateSplitButtonGeometry();
        return;
    }

    auto* hostLayout = layout();
    if (!hostLayout)
        return;

    const int hostIndex = hostLayout->indexOf(_primaryArea);

    _splitter = new QSplitter(orientation, this);
    _splitter->setChildrenCollapsible(false);
    _splitter->installEventFilter(this);

    hostLayout->removeWidget(_primaryArea);
    _primaryArea->setParent(_splitter);
    _primaryArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    _primaryArea->setMinimumSize(0, 0);
    _splitter->addWidget(_primaryArea);

    _secondaryArea = new MdiArea(_splitter);
    _secondaryArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    _secondaryArea->setMinimumSize(0, 0);
    syncPanelOptions(_secondaryArea);
    _secondaryArea->setViewMode(viewMode());
    _splitter->addWidget(_secondaryArea);
    _splitter->setStretchFactor(0, 1);
    _splitter->setStretchFactor(1, 1);
    setEqualSplitterSizes(_splitter);

    if (auto* boxLayout = qobject_cast<QBoxLayout*>(hostLayout)) {
        if (hostIndex >= 0)
            boxLayout->insertWidget(hostIndex, _splitter);
        else
            boxLayout->addWidget(_splitter);
    } else {
        hostLayout->addWidget(_splitter);
    }

    connectPanel(_secondaryArea);
    _activePanel = _primaryArea;
    requestEqualSplitterSizes();
    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::mergeSplitArea
///
void MdiAreaEx::mergeSplitArea()
{
    if (!_splitter || !_secondaryArea || !_primaryArea)
        return;

    _isSplitInProgress = true;

    const auto secondaryWindows = _secondaryArea->localSubWindowList();
    for (auto* wnd : secondaryWindows) {
        _secondaryArea->removeSubWindow(wnd);
        _primaryArea->addSubWindow(wnd, Qt::WindowFlags());
    }

    auto* hostLayout = layout();
    if (hostLayout) {
        const int hostIndex = hostLayout->indexOf(_splitter);
        hostLayout->removeWidget(_splitter);

        _primaryArea->setParent(this);

        if (auto* boxLayout = qobject_cast<QBoxLayout*>(hostLayout)) {
            if (hostIndex >= 0)
                boxLayout->insertWidget(hostIndex, _primaryArea);
            else
                boxLayout->addWidget(_primaryArea);
        } else {
            hostLayout->addWidget(_primaryArea);
        }
    }

    _secondaryArea->deleteLater();
    _secondaryArea = nullptr;

    _splitter->deleteLater();
    _splitter = nullptr;
    _pendingSplitterEqualize = false;
    _pendingSplitterEqualizePasses = 0;

    _activePanel = _primaryArea;
    _isSplitInProgress = false;
    syncSplitButtonState();
    updateSplitButtonGeometry();
}

///
/// \brief MdiAreaEx::requestEqualSplitterSizes
///
void MdiAreaEx::requestEqualSplitterSizes()
{
    if (!_splitter)
        return;

    _pendingSplitterEqualize = true;
    _pendingSplitterEqualizePasses = 8;
    tryEqualizeSplitterSizes();
///
/// \brief QTimer::singleShot
///
    QTimer::singleShot(0, this, [this]() {
        tryEqualizeSplitterSizes();
    });
///
/// \brief QTimer::singleShot
///
    QTimer::singleShot(16, this, [this]() {
        tryEqualizeSplitterSizes();
    });
///
/// \brief QTimer::singleShot
///
    QTimer::singleShot(33, this, [this]() {
        tryEqualizeSplitterSizes();
    });
}

///
/// \brief MdiAreaEx::tryEqualizeSplitterSizes
///
void MdiAreaEx::tryEqualizeSplitterSizes()
{
    if (!_pendingSplitterEqualize || !_splitter)
        return;

    if (!setEqualSplitterSizes(_splitter))
        return;

    if (_pendingSplitterEqualizePasses > 0)
        --_pendingSplitterEqualizePasses;

    if (_pendingSplitterEqualizePasses <= 0)
        _pendingSplitterEqualize = false;
}
