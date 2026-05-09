#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QProxyStyle>

///
/// \brief The AppStyle class
///
class AppStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    void drawControl(ControlElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget) const override;
};

#endif // APPSTYLE_H
