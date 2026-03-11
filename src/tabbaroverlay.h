#ifndef TABBAROVERLAY_H
#define TABBAROVERLAY_H

#include <QWidget>
#include <QTabBar>
#include <QPainter>

///
/// \brief The TabBarOverlay class
/// Draws a VSCode-style top indicator bar on the active tab without stylesheets.
/// Lives as a child of the QTabBar it decorates.
///
class TabBarOverlay : public QWidget
{
public:
    explicit TabBarOverlay(QTabBar* tabBar, bool active = true)
        : QWidget(tabBar)
        , _tabBar(tabBar)
        , _active(active)
    {
        setObjectName("TabBarOverlay");
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        resize(tabBar->size());
        connect(tabBar, &QTabBar::currentChanged, this, [this](int) { update(); });
        tabBar->installEventFilter(this);
        raise();
        setVisible(true);
    }

    void setActive(bool active)
    {
        if(_active == active) return;
        _active = active;
        update();
    }

protected:
    bool eventFilter(QObject* obj, QEvent* e) override
    {
        if(obj == _tabBar && e->type() == QEvent::Resize)
            resize(static_cast<QResizeEvent*>(e)->size());
        return false;
    }

    void paintEvent(QPaintEvent*) override
    {
        if(!_active) return;
        const int idx = _tabBar->currentIndex();
        if(idx < 0) return;

        const QRect r = _tabBar->tabRect(idx);
        QPainter p(this);
        p.fillRect(r.left(), 0, r.width(), 2, palette().highlight().color());
    }

private:
    QTabBar* _tabBar;
    bool _active;
};

#endif // TABBAROVERLAY_H
