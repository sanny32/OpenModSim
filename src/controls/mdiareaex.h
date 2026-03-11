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
    explicit MdiAreaEx(QWidget* parent = nullptr);
    ~MdiAreaEx();

    QMdiSubWindow *addSubWindow(QWidget *widget, Qt::WindowFlags flags = Qt::WindowFlags());
    void removeSubWindow(QWidget *widget);

    void setViewMode(ViewMode mode);

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
    void setupTabbedMode();
    void createSplitButton();
    void updateViewportBaseLine();

    void refreshTabBar();
    void updateTabBarGeometry();

    QMdiSubWindow* subWindowAtIndex(int index) const;
    void splitTab(int index, Qt::Orientation orientation);

private:
    bool _tabsExpanding = false;
    MdiTabBar* _tabBar;
    QToolButton* _splitButton;
    QFrame* _tabBarBaseLine = nullptr;
};

#endif // MDIAREAEX_H
