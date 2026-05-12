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
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override;
};

#endif // APPSTYLE_H
