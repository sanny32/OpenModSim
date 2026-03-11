#include "mdiareaex.h"
#include <QPainter>
#include <QStyleOptionTabBarBase>

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

///
/// \brief MdiAreaEx::MdiAreaEx
/// \param parent
///
MdiAreaEx::MdiAreaEx(QWidget* parent)
    : QMdiArea(parent)
    ,_splitButton(nullptr)
{
    setViewMode(QMdiArea::TabbedView);
    connect(this, &QMdiArea::subWindowActivated, this, &MdiAreaEx::on_subWindowActivated);
}

///
/// \brief MdiAreaEx::~MdiAreaEx
///
MdiAreaEx::~MdiAreaEx()
{
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
    if(!wnd) return nullptr;
    wnd->installEventFilter(this);

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
    if(_tabBar) {
        auto wnd = qobject_cast<QMdiSubWindow*>(widget);
        if(!wnd) {
            for(auto* candidate : subWindowList()) {
                if(candidate && candidate->widget() == widget) {
                    wnd = candidate;
                    break;
                }
            }
        }

        if(wnd)
            _tabBar->removeSubWindow(wnd);
    }

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
    _tabBar->installEventFilter(this);
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

    createSplitButton();
    updateTabBarGeometry();
    updateViewportBaseLine();

    connect(tabBar, &QObject::destroyed, this, &MdiAreaEx::on_tabBarDestroyed);
    connect(_tabBar, &QTabBar::currentChanged, this, &MdiAreaEx::on_currentTabChanged);
    connect(_tabBar, &QTabBar::tabCloseRequested, this, &MdiAreaEx::on_closeTab);
    connect(_tabBar, &QTabBar::tabMoved, this, &MdiAreaEx::on_moveTab);
}

///
/// \brief MdiTabBar::createSplitButton
///
void MdiAreaEx::createSplitButton()
{
    if (_splitButton)
        return;

    _splitButton = new QToolButton(this);
    _splitButton->setAutoRaise(true);
    _splitButton->setIcon(QIcon(":/res/actionSplitView.png"));
    _splitButton->setToolTip(tr("Split view"));

    const QSize sh = _splitButton->sizeHint();
    const int btnSize = qMax(20, qMin(sh.width(), sh.height()));
    _splitButton->setFixedSize(btnSize, btnSize);

    _splitButton->raise();
}


///
/// \brief MdiAreaEx::on_currentTabChanged
/// \param index
///
void MdiAreaEx::on_currentTabChanged(int index)
{
    if(!_tabBar)
        return;

    auto wnd = _tabBar->subWindowAt(index);
    if(wnd) setActiveSubWindow(wnd);
}

///
/// \brief MdiAreaEx::on_closeTab
/// \param index
///
void MdiAreaEx::on_closeTab(int index)
{
    if(!_tabBar)
        return;

    auto wnd = _tabBar->subWindowAt(index);
    if(wnd) wnd->close();
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

    if (_splitButton) {
        _splitButton->deleteLater();
        _splitButton = nullptr;
    }

    if (_tabBarBaseLine)
        _tabBarBaseLine->hide();

    updateViewportBaseLine();
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

    const int splitWidth = _splitButton ? (_splitButton->width() + 4) : 0;

    QRect tabBarRect;
    QRect buttonRect;

    switch (tabPosition()) {
        case QTabWidget::North:
            setViewportMargins(0, tabBarSizeHint.height(), 0, 0);
            tabBarRect = QRect(0, 0, areaWidth - splitWidth, tabBarSizeHint.height());
            if (_splitButton)
            {
                buttonRect = QRect(areaWidth - splitWidth,
                                   (tabBarSizeHint.height() - _splitButton->height())/2,
                                   _splitButton->width(),
                                   _splitButton->height());
            }
            break;
        case QTabWidget::South:
            setViewportMargins(0, 0, 0, tabBarSizeHint.height());
            tabBarRect = QRect(0, areaHeight - tabBarSizeHint.height(), areaWidth - splitWidth, tabBarSizeHint.height());
            if (_splitButton)
            {
                buttonRect = QRect(areaWidth - splitWidth,
                                   areaHeight - tabBarSizeHint.height() +
                                       (tabBarSizeHint.height() - _splitButton->height())/2,
                                   _splitButton->width(),
                                   _splitButton->height());
            }
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

    if (_splitButton) {
        _splitButton->setGeometry(buttonRect);
    }
    updateViewportBaseLine();
}

///
/// \brief MdiAreaEx::updateViewportBaseLine
///
void MdiAreaEx::updateViewportBaseLine()
{
    if (!_tabBarBaseLine) {
        _tabBarBaseLine = new TabBarBaseLineWidget(this);
    }

    auto* vp = viewport();
    if (!vp || viewMode() != QMdiArea::TabbedView || !_tabBar || !_tabBar->isVisible()) {
        _tabBarBaseLine->hide();
        return;
    }

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
    }
    else {
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
    if(_tabBar) _tabBar->setVisible(visible);
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
