#ifndef MDITABBAR_H
#define MDITABBAR_H

#include <QTabBar>

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
    bool _indicatorActive = true;
};

#endif // MDITABBAR_H
