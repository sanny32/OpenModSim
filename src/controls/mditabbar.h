#ifndef MDITABBAR_H
#define MDITABBAR_H

#include <QTabBar>
#include "tabbaroverlay.h"

class QMdiSubWindow;

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

    int indexOfSubWindow(QMdiSubWindow* wnd) const;
    QMdiSubWindow* subWindowAt(int tabIndex) const;

    QMdiSubWindow* currentSubWindow() const;
    void setCurrentSubWindow(QMdiSubWindow* wnd);

private:
    void updateSubWindowState(QMdiSubWindow* wnd);

private:
    TabBarOverlay* _mainTabOverlay;
};

#endif // MDITABBAR_H
