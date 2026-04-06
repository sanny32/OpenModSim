#ifndef UIUTILS_H
#define UIUTILS_H

#include <QColor>
#include <QPainter>
#include <QPushButton>

///
/// \brief recolorPushButtonIcon
/// \param btn
/// \param color
///
inline void recolorPushButtonIcon(QPushButton* btn, const QColor& color)
{
    if (!btn) return;

    QIcon origIcon = btn->icon();
    if (origIcon.isNull()) return;

    QSize iconSize = btn->iconSize();
    if (!iconSize.isValid())
        iconSize = btn->size();

    QPixmap pixmap = origIcon.pixmap(iconSize);
    if (pixmap.isNull()) return;

    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    painter.end();

    btn->setIcon(QIcon(pixmap));
}

///
/// \brief crossFadeWindowIcon
/// \param wnd
/// \param from
/// \param to
/// \param size
/// \param msec
///
inline void crossFadeWindowIcon(QWidget* wnd, const QIcon& from, const QIcon& to, int size = 16, int msec = 16)
{
    const int steps = 12;
    int step = 0;

    QPixmap p1 = from.pixmap(size, size);
    QPixmap p2 = to.pixmap(size, size);

    QTimer* timer = new QTimer(wnd);

    QObject::connect(timer, &QTimer::timeout, wnd, [=]() mutable
                     {
                         qreal t = qreal(step) / steps;

                         QPixmap result(size, size);
                         result.fill(Qt::transparent);

                         QPainter painter(&result);
                         painter.setRenderHint(QPainter::SmoothPixmapTransform);
                         painter.setOpacity(1.0 - t);
                         painter.drawPixmap(0, 0, p1);
                         painter.setOpacity(t);
                         painter.drawPixmap(0, 0, p2);

                         wnd->setWindowIcon(QIcon(result));

                         step++;
                         if (step > steps)
                         {
                             timer->stop();
                             timer->deleteLater();
                         }
                     });

    timer->start(msec);
}

#endif // UIUTILS_H

