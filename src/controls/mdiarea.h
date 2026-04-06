#ifndef MDIAREA_H
#define MDIAREA_H

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTabBar>
#include <QFrame>
#include <QPointer>
#include "mditabbar.h"

class MdiArea : public QMdiArea
{
    Q_OBJECT
public:
    explicit MdiArea(QWidget* parent = nullptr);
    ~MdiArea() override;

    QMdiSubWindow* addSubWindow(QWidget* widget, Qt::WindowFlags flags = Qt::WindowFlags());
    void removeSubWindow(QWidget* widget);
    QList<QMdiSubWindow*> localSubWindowList(WindowOrder order = CreationOrder) const;
    void setActiveSubWindow(QMdiSubWindow* wnd);

    void setViewMode(ViewMode mode);

    QTabBar* tabBar() const
    {
        return _tabBar;
    }

    bool tabsExpanding() const
    {
        return _tabsExpanding;
    }
    void setTabsExpanding(bool expanding);

    bool documentMode() const
    {
        return QMdiArea::documentMode();
    }
    void setDocumentMode(bool enabled);

    bool tabsClosable() const
    {
        return QMdiArea::tabsClosable();
    }
    void setTabsClosable(bool closable);

    bool tabsMovable() const
    {
        return QMdiArea::tabsMovable();
    }
    void setTabsMovable(bool movable);

    void setTabBarTrailingInset(int inset);
    void moveTabToPosition(QMdiSubWindow* subWnd, QPoint globalPos);
    int tabBarTrailingInset() const
    {
        return _tabBarTrailingInset;
    }

signals:
    void tabBarLayoutChanged();
    void tabsReordered();
    void lastTabAboutToClose();
    void tabContextMenuRequested(QMdiSubWindow* subWnd, QPoint globalPos);
    void tabDraggedOutside(QMdiSubWindow* subWnd, QPoint globalPos);

public:
    void setVisible(bool visible) override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void on_tabBarClicked(int index);
    void on_tabBarContextMenu(const QPoint& pos);
    void on_currentTabChanged(int index);
    void on_closeTab(int index);
    void on_moveTab(int from, int to);
    void on_tabBarDestroyed();
    void on_subWindowActivated(QMdiSubWindow* wnd);

private:
    void setupTabbedMode();
    void refreshTabBar();
    void updateTabBarGeometry();
    void updateViewportBaseLine();
    void syncNativeTabBarSelection(QMdiSubWindow* wnd);
    void updateTabbedEnabledState(QMdiSubWindow* activeWnd);
    void enforceTabbedSubWindowState(QMdiSubWindow* wnd);
    QMdiSubWindow* subWindowAtIndex(int index) const;

private:
    bool _destroying = false;
    bool _tabsExpanding = false;
    bool _updatingTabBarGeometry = false;
    int _tabBarTrailingInset = 0;
    QPointer<QTabBar> _nativeTabBar;
    MdiTabBar* _tabBar = nullptr;
    QFrame* _tabBarBaseLine = nullptr;
    QPointer<QMdiSubWindow> _lastActivatedSubWindow;
    QPointer<QMdiSubWindow> _requestedActivation;
    QList<QPointer<QMdiSubWindow>> _tabHistory;
};

#endif // MDIAREA_H
