#ifndef MDIAREAEX_H
#define MDIAREAEX_H

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTabBar>
#include <QToolButton>
#include <QSplitter>
#include <QFrame>
#include "mditabbar.h"

///
/// \brief The MdiAreaEx class
///
class MdiAreaEx : public QMdiArea
{
    Q_OBJECT
public:
    explicit MdiAreaEx(QWidget* parent = nullptr, bool splitPanel = false);
    ~MdiAreaEx();

    QMdiSubWindow *addSubWindow(QWidget *widget, Qt::WindowFlags flags = Qt::WindowFlags());
    QMdiSubWindow *addSubWindowDirect(QWidget *widget, Qt::WindowFlags flags = Qt::WindowFlags());
    void removeSubWindow(QWidget *widget);
    QList<QMdiSubWindow*> subWindowList(WindowOrder order = CreationOrder) const;
    QList<QMdiSubWindow*> localSubWindowList(WindowOrder order = CreationOrder) const;
    QMdiSubWindow* currentSubWindow() const;
    QMdiSubWindow* activeSubWindow() const;
    void setActiveSubWindow(QMdiSubWindow* wnd);
    void closeAllSubWindows();

    void setViewMode(ViewMode mode);
    void toggleVerticalSplit();
    bool isSplitView() const;
    MdiAreaEx* secondaryArea() const;

    QTabBar* tabBar() {
        return _tabBar;
    }

    bool documentMode() const {
        return QMdiArea::documentMode();
    }
    void setDocumentMode(bool enabled) {
        QMdiArea::setDocumentMode(enabled);
        refreshTabBar();
    }

    bool tabsClosable() const {
        return QMdiArea::tabsClosable();
    }
    void setTabsClosable(bool closable){
        QMdiArea::setTabsClosable(closable);
        refreshTabBar();
    }

    bool tabsMovable() const{
        return QMdiArea::tabsMovable();
    }
    void setTabsMovable(bool movable){
        QMdiArea::setTabsMovable(movable);
        refreshTabBar();
    }

    bool tabsExpanding() const {
        return _tabsExpanding;
    }
    void setTabsExpanding(bool expanding) {
        _tabsExpanding = expanding;
        refreshTabBar();
    }

signals:
    void splitViewAboutToDisable();
    void splitViewToggled(bool enabled);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void setVisible(bool visible) override;

private slots:
    void on_currentTabChanged(int index);
    void on_closeTab(int index);
    void on_moveTab(int from, int to);
    void on_tabBarDestroyed();
    void on_subWindowActivated(QMdiSubWindow* wnd);

private:
    QMdiSubWindow* addSubWindowLocal(QWidget* widget, Qt::WindowFlags flags);
    void removeSubWindowLocal(QWidget* widget);
    MdiAreaEx* areaForSubWindow(QMdiSubWindow* wnd) const;
    MdiAreaEx* activePanel() const;

    void setupTabbedMode();
    void createSplitButton();
    void updateViewportBaseLine();
    void syncSplitButtonState();
    void splitTab(int index, Qt::Orientation orientation);
    void setSplitViewEnabled(bool enabled);
    void ensureSplitArea(Qt::Orientation orientation);
    void mergeSplitArea();

    void refreshTabBar();
    void updateTabBarGeometry();

    QMdiSubWindow* subWindowAtIndex(int index) const;

private:
    bool _isSecondaryPanel = false;
    bool _isSplitInProgress = false;
    bool _tabsExpanding = false;
    MdiTabBar* _tabBar;
    QToolButton* _splitButton;
    QFrame* _tabBarBaseLine = nullptr;
    QSplitter* _splitter = nullptr;
    MdiAreaEx* _secondaryArea = nullptr;
    MdiAreaEx* _lastActiveArea = nullptr;
};

#endif // MDIAREAEX_H
