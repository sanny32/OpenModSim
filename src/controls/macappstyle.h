#ifndef MACAPPSTYLE_H
#define MACAPPSTYLE_H

#ifdef Q_OS_MAC

#include "appstyle.h"

///
/// \brief The MacAppStyle class
///
class MacAppStyle : public AppStyle
{
public:
    using AppStyle::AppStyle;

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                       QPainter* painter, const QWidget* widget) const override;
};

#endif // Q_OS_MAC
#endif // MACAPPSTYLE_H
