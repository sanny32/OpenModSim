#ifdef Q_OS_MAC

#include "macappstyle.h"

#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QStyleOptionTab>
#include <QTabBar>
#include <QWidget>

///
/// \brief MacAppStyle::MacAppStyle
/// \param parent
///
MacAppStyle::MacAppStyle(QObject* parent)
    : QlementineStyle(parent)
{
}

///
/// \brief MacAppStyle::drawControl
/// \param element
/// \param option
/// \param painter
/// \param widget
///
void MacAppStyle::drawControl(ControlElement element, const QStyleOption* option,
                              QPainter* painter, const QWidget* widget) const
{
    QlementineStyle::drawControl(element, option, painter, widget);

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
/// \brief MacAppStyle::polish
/// \param widget
///
void MacAppStyle::polish(QWidget* widget)
{
    QlementineStyle::polish(widget);

    if (auto* label = qobject_cast<QLabel*>(widget))
        label->setForegroundRole(QPalette::WindowText);
}

///
/// \brief MacAppStyle::sizeFromContents
/// \param type
/// \param option
/// \param contentsSize
/// \param widget
/// \return
///
QSize MacAppStyle::sizeFromContents(ContentsType type, const QStyleOption* option,
                                    const QSize& contentsSize, const QWidget* widget) const
{
    QSize size = QlementineStyle::sizeFromContents(type, option, contentsSize, widget);

    if (type == CT_LineEdit && qobject_cast<const QLineEdit*>(widget))
        size.setWidth(qMax(size.width(), contentsSize.width()));

    return size;
}

#endif // Q_OS_MAC
