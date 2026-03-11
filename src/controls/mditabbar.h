#ifndef MDITABBAR_H
#define MDITABBAR_H

#include <QTabBar>
#include "tabbaroverlay.h"

///
/// \brief The MdiTabBar class
///
class MdiTabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit MdiTabBar(QWidget* parent = nullptr);

    void addSubWindow(QMdiSubWindow* wnd);
    void removeSubWindow(QMdiSubWindow* wnd);

    QMdiSubWindow* currentSubWindow() const;
    void setCurrentSubWindow(QMdiSubWindow* wnd);

private:
    void createSplitButton();

private:
    TabBarOverlay* _mainTabOverlay;
    QToolButton* _splitButton;
};

#endif // MDITABBAR_H
