#ifndef MDITABBAR_H
#define MDITABBAR_H

#include <QPointer>
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

signals:
    void tabDraggedOutside(QMdiSubWindow* subWnd, QPoint globalPos);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void updateSubWindowState(QMdiSubWindow* wnd);

private:
    bool _indicatorActive = true;
    QPointer<QMdiSubWindow> _dragSubWindow;
    QPixmap  _dragPixmap;
    QPoint   _dragHotspot;
    QWidget* _dragOverlay  = nullptr;
    bool     _dragCursorSet = false;
};

#endif // MDITABBAR_H
