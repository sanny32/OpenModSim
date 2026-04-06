#include "mdiarea.h"
#include <QApplication>
#include <QPainter>
#include <QScrollBar>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStyle>
#include <QStyleOptionTabBarBase>
#include <QTimer>
#include "../apptrace.h"

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

static bool isInterestingTraceEvent(QEvent::Type type)
{
    switch (type) {
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::ShortcutOverride:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::Close:
            return true;
        default:
            return false;
    }
}

static QString areaTag(const MdiArea* area)
{
    return QStringLiteral("%1 state={%2}")
        .arg(AppTrace::objectTag(area))
        .arg(AppTrace::mdiAreaState(area));
}

static QMdiSubWindow* mdiOwnerOf(QWidget* widget)
{
    for (QWidget* it = widget; it; it = it->parentWidget()) {
        if (auto* sub = qobject_cast<QMdiSubWindow*>(it))
            return sub;
    }
    return nullptr;
}

///
/// \brief MdiArea::MdiArea
/// \param parent
///
MdiArea::MdiArea(QWidget* parent)
    : QMdiArea(parent)
{
    setViewMode(QMdiArea::TabbedView);
    connect(this, &QMdiArea::subWindowActivated, this, &MdiArea::on_subWindowActivated);
    AppTrace::log("MdiArea::MdiArea",
                  QStringLiteral("constructed %1").arg(areaTag(this)));
}

///
/// \brief MdiArea::~MdiArea
///
MdiArea::~MdiArea()
{
    _destroying = true;
    AppTrace::log("MdiArea::~MdiArea",
                  QStringLiteral("destroying %1").arg(areaTag(this)));
}

///
/// \brief MdiArea::addSubWindow
/// \param widget
/// \param flags
/// \return
///
QMdiSubWindow* MdiArea::addSubWindow(QWidget* widget, Qt::WindowFlags flags)
{
    AppTrace::log("MdiArea::addSubWindow",
                  QStringLiteral("%1 request widget=%2 flags=0x%3")
                      .arg(AppTrace::objectTag(this))
                      .arg(AppTrace::widgetTag(widget))
                      .arg(QString::number(quint32(flags), 16)));

    auto* wnd = QMdiArea::addSubWindow(widget, flags);
    if (!wnd)
        return nullptr;

    wnd->installEventFilter(this);
    enforceTabbedSubWindowState(wnd);
    if (_tabBar) {
        QScopedValueRollback<QPointer<QMdiSubWindow>> requestedActivationGuard(_requestedActivation, wnd);
        _tabBar->addSubWindow(wnd);
    }

    updateTabBarGeometry();

    AppTrace::log("MdiArea::addSubWindow",
                  QStringLiteral("%1 added=%2 after=%3")
                      .arg(AppTrace::objectTag(this))
                      .arg(AppTrace::subWindowTag(wnd))
                      .arg(AppTrace::mdiAreaState(this)));
    return wnd;
}

///
/// \brief MdiArea::removeSubWindow
/// \param widget
///
void MdiArea::removeSubWindow(QWidget* widget)
{
    AppTrace::log("MdiArea::removeSubWindow",
                  QStringLiteral("%1 request widget=%2")
                      .arg(AppTrace::objectTag(this))
                      .arg(AppTrace::widgetTag(widget)));

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

    AppTrace::log("MdiArea::removeSubWindow",
                  QStringLiteral("%1 after=%2")
                      .arg(AppTrace::objectTag(this))
                      .arg(AppTrace::mdiAreaState(this)));
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
/// \brief MdiArea::setActiveSubWindow
/// \param wnd
///
void MdiArea::setActiveSubWindow(QMdiSubWindow* wnd)
{
    QScopedValueRollback<QPointer<QMdiSubWindow>> requestedActivationGuard(_requestedActivation, wnd);

    const auto beforeState = AppTrace::mdiAreaState(this);
    AppTrace::log("MdiArea::setActiveSubWindow",
                  QStringLiteral("%1 request=%2 focus=%3 before=%4")
                      .arg(AppTrace::objectTag(this))
                      .arg(AppTrace::subWindowTag(wnd))
                      .arg(AppTrace::widgetTag(QApplication::focusWidget()))
                      .arg(beforeState));

    if (wnd && !wnd->isEnabled())
        wnd->setEnabled(true);

    QMdiArea::setActiveSubWindow(wnd);

    // Some Qt versions can keep the previous active page during early startup.
    // Retry with an explicit show/focus to force page stack activation.
    if (wnd && viewMode() == QMdiArea::TabbedView && QMdiArea::activeSubWindow() != wnd) {
        enforceTabbedSubWindowState(wnd);
        wnd->show();
        wnd->raise();
        wnd->setFocus(Qt::OtherFocusReason);
        QMdiArea::setActiveSubWindow(wnd);
    }

    if (viewMode() != QMdiArea::TabbedView)
        return;

    const auto windows = QMdiArea::subWindowList();
    QMdiSubWindow* actual = QMdiArea::activeSubWindow();
    if (!actual)
        actual = QMdiArea::currentSubWindow();
    if (!actual || !windows.contains(actual))
        actual = (wnd && windows.contains(wnd)) ? wnd : nullptr;

    if (actual) {
        _lastActivatedSubWindow = actual;

        if (_tabBar && _tabBar->currentSubWindow() != actual) {
            const QSignalBlocker blocker(_tabBar);
            _tabBar->setCurrentSubWindow(actual);
        }

        syncNativeTabBarSelection(actual);
        updateTabbedEnabledState();
    } else {
        QMdiSubWindow* keepEnabled = _lastActivatedSubWindow.data();
        if (!keepEnabled && _tabBar)
            keepEnabled = _tabBar->currentSubWindow();
        if (!keepEnabled)
            keepEnabled = QMdiArea::currentSubWindow();
        updateTabbedEnabledState();
    }

    const QMdiSubWindow* tabCurrent = _tabBar ? _tabBar->currentSubWindow() : nullptr;
    AppTrace::log("MdiArea::setActiveSubWindow",
                  QStringLiteral("%1 done tabCurrent=%2 lastActivated=%3 after=%4")
                      .arg(AppTrace::objectTag(this))
                      .arg(AppTrace::subWindowTag(tabCurrent))
                      .arg(AppTrace::subWindowTag(_lastActivatedSubWindow.data()))
                      .arg(AppTrace::mdiAreaState(this)));
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

    updateTabbedEnabledState();
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
    if (AppTrace::isEnabled() && event && isInterestingTraceEvent(event->type())) {
        bool related = (obj == this || obj == _tabBar || obj == _nativeTabBar || obj == viewport());
        if (!related) {
            if (const auto* widget = qobject_cast<const QWidget*>(obj))
                related = (widget == this) || isAncestorOf(widget);
        }

        if (related) {
            AppTrace::log("MdiArea::eventFilter",
                          QStringLiteral("%1 obj=%2 event=%3 focus=%4 state=%5")
                              .arg(AppTrace::objectTag(this))
                              .arg(AppTrace::objectTag(obj))
                              .arg(AppTrace::eventTypeName(event->type()))
                              .arg(AppTrace::widgetTag(QApplication::focusWidget()))
                              .arg(AppTrace::mdiAreaState(this)));
        }
    }

    if (event && event->type() == QEvent::FocusIn && viewMode() == QMdiArea::TabbedView && _tabBar) {
        auto* widget = qobject_cast<QWidget*>(obj);
        auto* owner = mdiOwnerOf(widget);
        const auto windows = QMdiArea::subWindowList();

        if (owner && windows.contains(owner)) {
            QMdiSubWindow* tabCurrent = _tabBar->currentSubWindow();
            if ((!tabCurrent || !windows.contains(tabCurrent)) && _tabBar->currentIndex() >= 0)
                tabCurrent = subWindowAtIndex(_tabBar->currentIndex());
            if (tabCurrent && !windows.contains(tabCurrent))
                tabCurrent = nullptr;

            const bool activationRequested = _requestedActivation && _requestedActivation == owner;
            if (!activationRequested && tabCurrent && owner != tabCurrent) {
                AppTrace::log("MdiArea::eventFilter",
                              QStringLiteral("%1 blocked foreign FocusIn obj=%2 owner=%3 tabCurrent=%4")
                                  .arg(AppTrace::objectTag(this))
                                  .arg(AppTrace::objectTag(obj))
                                  .arg(AppTrace::subWindowTag(owner))
                                  .arg(AppTrace::subWindowTag(tabCurrent)));

                QPointer<QMdiSubWindow> stable = tabCurrent;
                QTimer::singleShot(0, this, [this, stable]() {
                    if (!stable)
                        return;
                    if (!QMdiArea::subWindowList().contains(stable.data()))
                        return;
                    setActiveSubWindow(stable.data());
                });
                return true;
            }
        }
    }

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
    AppTrace::log("MdiArea::setVisible",
                  QStringLiteral("%1 visible=%2 before=%3")
                      .arg(AppTrace::objectTag(this))
                      .arg(visible)
                      .arg(AppTrace::mdiAreaState(this)));

    QMdiArea::setVisible(visible);
    if (visible) {
        setupTabbedMode();
        for (auto* wnd : QMdiArea::subWindowList())
            enforceTabbedSubWindowState(wnd);

        if (viewMode() == QMdiArea::TabbedView && _tabBar) {
            if (auto* current = subWindowAtIndex(_tabBar->currentIndex()))
                setActiveSubWindow(current);

            // Run once after pending startup events to eliminate tab/content drift.
            QTimer::singleShot(0, this, [this]() {
                if (viewMode() != QMdiArea::TabbedView || !_tabBar)
                    return;
                if (auto* current = subWindowAtIndex(_tabBar->currentIndex()))
                    setActiveSubWindow(current);
            });
        }
    }
    if (_tabBar)
        _tabBar->setVisible(visible && viewMode() == QMdiArea::TabbedView);
    updateViewportBaseLine();
    emit tabBarLayoutChanged();

    AppTrace::log("MdiArea::setVisible",
                  QStringLiteral("%1 done visible=%2 after=%3")
                      .arg(AppTrace::objectTag(this))
                      .arg(visible)
                      .arg(AppTrace::mdiAreaState(this)));
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
    AppTrace::log("MdiArea::on_tabBarClicked",
                  QStringLiteral("%1 index=%2 wnd=%3")
                      .arg(AppTrace::objectTag(this))
                      .arg(index)
                      .arg(AppTrace::subWindowTag(wnd)));
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
    const bool activationRequested = _requestedActivation && _requestedActivation == wnd;
    const auto windows = QMdiArea::subWindowList();
    QWidget* focus = QApplication::focusWidget();
    QMdiSubWindow* focusOwner = mdiOwnerOf(focus);
    const bool focusInsideMdi = focus && (focus == this || isAncestorOf(focus));
    const bool focusInsideWnd = wnd && focus && (focus == wnd || wnd->isAncestorOf(focus));
    const bool focusInsideOtherWnd = wnd && focusOwner && focusOwner != wnd;
    AppTrace::log("MdiArea::on_currentTabChanged",
                  QStringLiteral("%1 index=%2 wnd=%3 requested=%4 focus=%5 focusOwner=%6 focusInsideMdi=%7 focusInsideWnd=%8 focusInsideOtherWnd=%9")
                       .arg(AppTrace::objectTag(this))
                       .arg(index)
                       .arg(AppTrace::subWindowTag(wnd))
                       .arg(activationRequested)
                       .arg(AppTrace::widgetTag(focus))
                       .arg(AppTrace::subWindowTag(focusOwner))
                       .arg(focusInsideMdi)
                       .arg(focusInsideWnd)
                       .arg(focusInsideOtherWnd));

    // Ignore transient tab changes coming from outside MDI focus context
    // (for example, when external tab widgets switch pages) and from cases
    // where focus still belongs to another MDI subwindow.
    if (!activationRequested && wnd && ((!focusInsideMdi && !focusInsideWnd) || focusInsideOtherWnd)) {
        QMdiSubWindow* stable = focusOwner;
        if (!stable || !windows.contains(stable))
            stable = _lastActivatedSubWindow.data();
        if (!stable || !windows.contains(stable))
            stable = QMdiArea::activeSubWindow();
        if (!stable || !windows.contains(stable))
            stable = QMdiArea::currentSubWindow();

        if (stable && stable != wnd) {
            const QSignalBlocker blocker(_tabBar);
            _tabBar->setCurrentSubWindow(stable);
            syncNativeTabBarSelection(stable);
            AppTrace::log("MdiArea::on_currentTabChanged",
                          QStringLiteral("%1 blocked transient change wnd=%2 stable=%3 focusOwner=%4")
                              .arg(AppTrace::objectTag(this))
                              .arg(AppTrace::subWindowTag(wnd))
                              .arg(AppTrace::subWindowTag(stable))
                              .arg(AppTrace::subWindowTag(focusOwner)));
            return;
        }
    }

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
    AppTrace::log("MdiArea::on_closeTab",
                  QStringLiteral("%1 index=%2 wnd=%3")
                      .arg(AppTrace::objectTag(this))
                      .arg(index)
                      .arg(AppTrace::subWindowTag(wnd)));
    if (!wnd)
        return;

    if (_tabBar->count() == 1)
        emit lastTabAboutToClose();

    // Find the previously active window using Qt's built-in activation history.
    QMdiSubWindow* prev = nullptr;
    for (auto* c : QMdiArea::subWindowList(ActivationHistoryOrder))
        if (c && c != wnd) { prev = c; break; }

    wnd->close();

    if (prev)
        setActiveSubWindow(prev);
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
    QWidget* focus = QApplication::focusWidget();
    QMdiSubWindow* focusOwner = mdiOwnerOf(focus);

    AppTrace::log("MdiArea::on_subWindowActivated",
                  QStringLiteral("%1 signal wnd=%2 focus=%3 state=%4")
                      .arg(AppTrace::objectTag(this))
                      .arg(AppTrace::subWindowTag(wnd))
                      .arg(AppTrace::widgetTag(focus))
                      .arg(AppTrace::mdiAreaState(this)));

    if (!wnd) {
        QMdiSubWindow* keepEnabled = _lastActivatedSubWindow.data();
        if (!keepEnabled && _tabBar)
            keepEnabled = _tabBar->currentSubWindow();
        if (!keepEnabled)
            keepEnabled = QMdiArea::currentSubWindow();

        updateTabbedEnabledState();
        // Keep Qt's hidden native tabbar on the last known document. This
        // prevents fallback activation to the first tab when focus leaves MDI.
        syncNativeTabBarSelection(keepEnabled);

        if (_tabBar)
            updateTabBarGeometry();

        AppTrace::log("MdiArea::on_subWindowActivated",
                      QStringLiteral("%1 null-activation keepEnabled=%2 after=%3")
                          .arg(AppTrace::objectTag(this))
                          .arg(AppTrace::subWindowTag(keepEnabled))
                          .arg(AppTrace::mdiAreaState(this)));
        return;
    }

    const auto windows = QMdiArea::subWindowList();
    const bool activationRequested = _requestedActivation && _requestedActivation == wnd;
    const bool focusInsideMdi = focus && (focus == this || isAncestorOf(focus));
    const bool focusInsideWnd = focus && (focus == wnd || wnd->isAncestorOf(focus));
    const bool focusInsideOtherWnd = focusOwner && focusOwner != wnd;

    QMdiSubWindow* tabCurrent = _tabBar ? subWindowAtIndex(_tabBar->currentIndex()) : nullptr;

    QMdiSubWindow* stable = tabCurrent;
    if (!stable && _lastActivatedSubWindow && windows.contains(_lastActivatedSubWindow.data()))
        stable = _lastActivatedSubWindow.data();
    if (!stable && focusOwner && windows.contains(focusOwner))
        stable = focusOwner;

    const bool transientActivation =
        !activationRequested &&
        stable &&
        stable != wnd &&
        !focusInsideWnd &&
        ((tabCurrent && tabCurrent != wnd) || !focus || focusInsideOtherWnd || !focusInsideMdi);

    if (transientActivation) {
        AppTrace::log("MdiArea::on_subWindowActivated",
                      QStringLiteral("%1 blocked transient activation wnd=%2 stable=%3 tabCurrent=%4 requested=%5 focusOwner=%6 focusInsideMdi=%7 focusInsideWnd=%8")
                          .arg(AppTrace::objectTag(this))
                          .arg(AppTrace::subWindowTag(wnd))
                          .arg(AppTrace::subWindowTag(stable))
                          .arg(AppTrace::subWindowTag(tabCurrent))
                          .arg(activationRequested)
                          .arg(AppTrace::subWindowTag(focusOwner))
                          .arg(focusInsideMdi)
                          .arg(focusInsideWnd));

        if (_tabBar && _tabBar->currentSubWindow() != stable) {
            const QSignalBlocker blocker(_tabBar);
            _tabBar->setCurrentSubWindow(stable);
        }
        syncNativeTabBarSelection(stable);

        if (QMdiArea::activeSubWindow() != stable || QMdiArea::currentSubWindow() != stable)
            setActiveSubWindow(stable);
        return;
    }

    _lastActivatedSubWindow = wnd;
    updateTabbedEnabledState();
    enforceTabbedSubWindowState(wnd);

    if (!_tabBar)
        return;

    if (_tabBar->currentSubWindow() != wnd) {
        const QSignalBlocker blocker(_tabBar);
        _tabBar->setCurrentSubWindow(wnd);
    }
    syncNativeTabBarSelection(wnd);
    updateTabBarGeometry();

    AppTrace::log("MdiArea::on_subWindowActivated",
                  QStringLiteral("%1 activated=%2 after=%3")
                      .arg(AppTrace::objectTag(this))
                      .arg(AppTrace::subWindowTag(wnd))
                      .arg(AppTrace::mdiAreaState(this)));
}

///
/// \brief MdiArea::setupTabbedMode
///
void MdiArea::setupTabbedMode()
{
    AppTrace::log("MdiArea::setupTabbedMode",
                  QStringLiteral("%1 begin state=%2")
                      .arg(AppTrace::objectTag(this))
                      .arg(AppTrace::mdiAreaState(this)));

    if (_tabBar) {
        refreshTabBar();
        if (isVisible())
            _tabBar->show();
        updateTabBarGeometry();
        updateViewportBaseLine();
        AppTrace::log("MdiArea::setupTabbedMode",
                      QStringLiteral("%1 reuse-existing-tabbar").arg(AppTrace::objectTag(this)));
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

    _nativeTabBar = nativeTabBar;
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

    const auto windows = QMdiArea::subWindowList();

    QMdiSubWindow* preferredCurrent = nullptr;
    if (_lastActivatedSubWindow && windows.contains(_lastActivatedSubWindow.data()))
        preferredCurrent = _lastActivatedSubWindow.data();
    if (!preferredCurrent)
        preferredCurrent = QMdiArea::activeSubWindow();
    if (!preferredCurrent)
        preferredCurrent = QMdiArea::currentSubWindow();
    if (!preferredCurrent && _nativeTabBar) {
        const int nativeIndex = _nativeTabBar->currentIndex();
        if (nativeIndex >= 0 && nativeIndex < windows.size())
            preferredCurrent = windows.at(nativeIndex);
    }
    if (!preferredCurrent && !windows.isEmpty())
        preferredCurrent = windows.first();
    if (preferredCurrent)
        _lastActivatedSubWindow = preferredCurrent;

    if (preferredCurrent) {
        // Keep custom tab bar, native tab stack and QMdiArea active window
        // strictly aligned when tabbed mode is (re)initialized.
        setActiveSubWindow(preferredCurrent);
    } else {
        updateTabbedEnabledState();
    }

    AppTrace::log("MdiArea::setupTabbedMode",
                  QStringLiteral("%1 preferredCurrent=%2 state=%3")
                      .arg(AppTrace::objectTag(this))
                      .arg(AppTrace::subWindowTag(preferredCurrent))
                      .arg(AppTrace::mdiAreaState(this)));

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
/// \brief MdiArea::updateTabbedEnabledState
///
void MdiArea::updateTabbedEnabledState()
{
    // Keep all MDI subwindows enabled. Disabling non-active windows can desync
    // Qt's internal tab stack from our custom tab bar on some startup paths.
    const auto windows = QMdiArea::subWindowList();
    for (auto* wnd : windows) {
        if (wnd && !wnd->isEnabled())
            wnd->setEnabled(true);
    }
}

///
/// \brief MdiArea::syncNativeTabBarSelection
/// \param wnd
///
void MdiArea::syncNativeTabBarSelection(QMdiSubWindow* wnd)
{
    if (!_nativeTabBar || !wnd)
        return;

    const auto windows = QMdiArea::subWindowList();
    const int index = windows.indexOf(wnd);
    if (index < 0 || index >= _nativeTabBar->count())
        return;

    if (_nativeTabBar->currentIndex() == index)
        return;

    const QSignalBlocker blocker(_nativeTabBar);
    _nativeTabBar->setCurrentIndex(index);
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
    if (_tabBar) {
        if (auto* wnd = _tabBar->subWindowAt(index))
            if (QMdiArea::subWindowList().contains(wnd))
                return wnd;
    }
    const auto windows = QMdiArea::subWindowList();
    return (index >= 0 && index < windows.size()) ? windows.at(index) : nullptr;
}

///
/// \brief MdiArea::moveTabToPosition
/// Repositions \a subWnd's tab to the index under \a globalPos.
/// Call this after addSubWindow() to insert at the drop location rather than the end.
///
void MdiArea::moveTabToPosition(QMdiSubWindow* subWnd, QPoint globalPos)
{
    if (!_tabBar || !subWnd || globalPos.isNull())
        return;

    const int currentIndex = _tabBar->indexOfSubWindow(subWnd);
    if (currentIndex < 0)
        return;

    const QPoint localPos = _tabBar->mapFromGlobal(globalPos);
    const int targetIndex = _tabBar->tabAt(localPos);
    if (targetIndex < 0 || targetIndex == currentIndex)
        return;

    _tabBar->moveTab(currentIndex, targetIndex);
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

