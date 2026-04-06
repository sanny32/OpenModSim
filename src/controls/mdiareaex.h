#ifndef MDIAREAEX_H
#define MDIAREAEX_H

#include <QPointer>
#include <QWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QBrush>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QSplitter>
#include "mdiarea.h"
#include "ui_mdiareaex.h"

class MdiAreaEx : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool documentMode READ documentMode WRITE setDocumentMode)
    Q_PROPERTY(bool tabsClosable READ tabsClosable WRITE setTabsClosable)
    Q_PROPERTY(bool tabsMovable READ tabsMovable WRITE setTabsMovable)

public:
    explicit MdiAreaEx(QWidget* parent = nullptr);
    ~MdiAreaEx() override;

    QMdiSubWindow* addSubWindow(QWidget* widget, Qt::WindowFlags flags = Qt::WindowFlags());
    void removeSubWindow(QWidget* widget);

    QList<QMdiSubWindow*> subWindowList(QMdiArea::WindowOrder order = QMdiArea::CreationOrder) const;
    QList<QMdiSubWindow*> localSubWindowList(QMdiArea::WindowOrder order = QMdiArea::CreationOrder) const;

    QMdiSubWindow* currentSubWindow() const;
    QMdiSubWindow* activeSubWindow() const;
    void setActiveSubWindow(QMdiSubWindow* wnd);
    void closeAllSubWindows();

    void setViewMode(QMdiArea::ViewMode mode);
    QMdiArea::ViewMode viewMode() const;
    void toggleVerticalSplit();
    bool isSplitView() const;

    MdiArea* primaryArea() const;
    MdiArea* secondaryArea() const;

    QTabBar* tabBar() const;

    bool isEmpty() const
    {
        return subWindowList().isEmpty();
    }

    bool documentMode() const;
    void setDocumentMode(bool enabled);

    bool tabsClosable() const;
    void setTabsClosable(bool closable);

    bool tabsMovable() const;
    void setTabsMovable(bool movable);

    bool tabsExpanding() const;
    void setTabsExpanding(bool expanding);

    void setActivationOrder(QMdiArea::WindowOrder order);
    QMdiArea::WindowOrder activationOrder() const;

    void setBackground(const QBrush& background);
    QBrush background() const;

    void setOption(QMdiArea::AreaOption option, bool on = true);
    bool testOption(QMdiArea::AreaOption option) const;

    void setTabPosition(QTabWidget::TabPosition position);
    QTabWidget::TabPosition tabPosition() const;

    void setTabShape(QTabWidget::TabShape shape);
    QTabWidget::TabShape tabShape() const;

    void cascadeSubWindows();
    void tileSubWindows();

    void setSplitViewEnabled(bool enabled);
    void moveSubWindowToOtherPanel(QMdiSubWindow* subWnd, QPoint globalDropPos = QPoint());
    MdiArea* activePanel() const;

    QMdiSubWindow* activePrimarySubWindow() const;

signals:
    void subWindowActivated(QMdiSubWindow* wnd);
    void splitViewAboutToDisable();
    void splitViewToggled(bool enabled);
    void tabsReordered();
    void tabContextMenuRequested(QMdiSubWindow* subWnd, MdiArea* sourcePanel, QPoint globalPos);
    void moveTabToOtherPanelRequested(QMdiSubWindow* subWnd, QPoint globalDropPos);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void setVisible(bool visible) override;

private slots:
    void on_panelSubWindowActivated(QMdiSubWindow* wnd);
    void on_panelTabBarLayoutChanged();
    void on_panelTabContextMenuRequested(QMdiSubWindow* subWnd, QPoint globalPos);
    void on_panelTabDraggedOutside(QMdiSubWindow* subWnd, QPoint globalPos);

private:
    MdiArea* areaForSubWindow(QMdiSubWindow* wnd) const;
    void connectPanel(MdiArea* area);
    void syncPanelOptions(MdiArea* area) const;

    void createSplitButton();
    bool shouldShowSplitButton() const;
    void syncSplitButtonState();
    void updateSplitButtonGeometry();
    void ensureSplitArea(Qt::Orientation orientation);
    void mergeSplitArea();
    void equalizeSplitterSizes();

private:
    Ui::MdiAreaEx* ui = nullptr;
    bool _isSplitInProgress = false;
    QPointer<QMdiSubWindow> _preSplitActiveWindow;
    bool _destroying = false;
    bool _updatingSplitButtonGeometry = false;
    QToolButton* _splitButton = nullptr;
    QSplitter* _splitter = nullptr;
    MdiArea* _primaryArea = nullptr;
    MdiArea* _secondaryArea = nullptr;
    MdiArea* _activePanel = nullptr;
};

#endif // MDIAREAEX_H
