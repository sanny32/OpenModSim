#ifndef MACAPPSTYLE_H
#define MACAPPSTYLE_H

#ifdef Q_OS_MAC

#include <oclero/qlementine/style/QlementineStyle.hpp>

class QWidget;

///
/// \brief The MacAppStyle class
///
class MacAppStyle : public oclero::qlementine::QlementineStyle
{
public:
    explicit MacAppStyle(QObject* parent = nullptr);

    void drawControl(ControlElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget = nullptr) const override;
    void polish(QWidget* widget) override;
    QSize sizeFromContents(ContentsType type, const QStyleOption* option,
                           const QSize& contentsSize, const QWidget* widget = nullptr) const override;
};

#endif // Q_OS_MAC
#endif // MACAPPSTYLE_H
