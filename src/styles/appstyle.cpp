#include "appstyle.h"

#include <QPainter>
#include <QStyleOptionToolButton>
#include <QStyleOptionTab>
#include <QTabBar>

///
/// \brief AppStyle::drawControl
/// \param element
/// \param option
/// \param painter
/// \param widget
///
void AppStyle::drawControl(ControlElement element, const QStyleOption* option,
                            QPainter* painter, const QWidget* widget) const
{
    QProxyStyle::drawControl(element, option, painter, widget);

    if (element != CE_TabBarTab || !widget)
        return;

    const auto* tabBar = qobject_cast<const QTabBar*>(widget);
    const auto* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);
    if (!tabBar || !tabOption)
        return;

    if (!(tabOption->state & State_Selected))
        return;

    if (!widget->property("mdiIndicatorActive").toBool())
        return;

    const QRect r = tabOption->rect;
    const QColor color = widget->palette().highlight().color();
    constexpr int thickness = 2;

    switch (tabBar->shape()) {
        case QTabBar::RoundedSouth:
        case QTabBar::TriangularSouth:
            painter->fillRect(r.left(), r.bottom() - thickness + 1, r.width(), thickness, color);
            break;
        case QTabBar::RoundedWest:
        case QTabBar::TriangularWest:
            painter->fillRect(r.left(), r.top(), thickness, r.height(), color);
            break;
        case QTabBar::RoundedEast:
        case QTabBar::TriangularEast:
            painter->fillRect(r.right() - thickness + 1, r.top(), thickness, r.height(), color);
            break;
        case QTabBar::RoundedNorth:
        case QTabBar::TriangularNorth:
        default:
            painter->fillRect(r.left(), r.top(), r.width(), thickness, color);
            break;
    }
}

///
/// \brief AppStyle::pixelMetric
/// \param metric
/// \param option
/// \param widget
/// \return
///
int AppStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
    if (widget && widget->property("preservePressedIconAlignment").toBool()) {
        switch (metric) {
            case PM_ButtonShiftHorizontal:
            case PM_ButtonShiftVertical:
                return 0;
            default:
                break;
        }
    }

    return QProxyStyle::pixelMetric(metric, option, widget);
}
