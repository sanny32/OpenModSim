#ifdef Q_OS_MAC

#include "macappstyle.h"

#include <QMainWindow>
#include <QPainter>
#include <QStyleOption>

///
/// \brief MacAppStyle::drawPrimitive
/// \param element
/// \param option
/// \param painter
/// \param widget
///
void MacAppStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                                 QPainter* painter, const QWidget* widget) const
{
    if (element == PE_PanelToolBar && widget &&
        !qobject_cast<const QMainWindow*>(widget->parentWidget()))
    {
        painter->fillRect(option->rect, option->palette.window());
        return;
    }
    AppStyle::drawPrimitive(element, option, painter, widget);
}

#endif // Q_OS_MAC
