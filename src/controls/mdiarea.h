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
    int tabBarTrailingInset() const
    {
        return _tabBarTrailingInset;
    }

signals:
    void tabBarLayoutChanged();
    void tabsReordered();
    void lastTabAboutToClose();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void setVisible(bool visible) override;

private slots:
    void on_tabBarClicked(int index);
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
    void enforceTabbedSubWindowState(QMdiSubWindow* wnd);
    QMdiSubWindow* subWindowAtIndex(int index) const;

private:
    bool _destroying = false;
    bool _tabsExpanding = false;
    bool _updatingTabBarGeometry = false;
    int _tabBarTrailingInset = 0;
    MdiTabBar* _tabBar = nullptr;
    QFrame* _tabBarBaseLine = nullptr;
    QPointer<QMdiSubWindow> _lastActivatedSubWindow;
};

#endif // MDIAREA_H
