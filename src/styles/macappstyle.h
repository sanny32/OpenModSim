#ifndef MACAPPSTYLE_H
#define MACAPPSTYLE_H

#ifdef Q_OS_MAC

#include <oclero/qlementine/style/QlementineStyle.hpp>

class QWidget;
class QModelIndex;

///
/// \brief The MacAppStyle class
///
class MacAppStyle : public oclero::qlementine::QlementineStyle
{
public:
    explicit MacAppStyle(QObject* parent = nullptr);

    QColor const& buttonBackgroundColor(oclero::qlementine::MouseState mouse,
                                        oclero::qlementine::ColorRole role,
                                        const QWidget* widget = nullptr) const override;
    QColor const& buttonForegroundColor(oclero::qlementine::MouseState mouse,
                                        oclero::qlementine::ColorRole role,
                                        const QWidget* widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget = nullptr) const override;
    QColor const& iconForegroundColor(oclero::qlementine::MouseState mouse,
                                      oclero::qlementine::ColorRole role) const override;
    QColor listItemBackgroundColor(oclero::qlementine::MouseState mouse,
                                   oclero::qlementine::SelectionState selected,
                                   oclero::qlementine::FocusState focus,
                                   oclero::qlementine::ActiveState active,
                                   const QModelIndex& index,
                                   const QWidget* widget = nullptr) const override;
    QColor const& listItemForegroundColor(oclero::qlementine::MouseState mouse,
                                          oclero::qlementine::SelectionState selected,
                                          oclero::qlementine::FocusState focus,
                                          oclero::qlementine::ActiveState active) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override;
    void polish(QWidget* widget) override;
    QSize sizeFromContents(ContentsType type, const QStyleOption* option,
                           const QSize& contentsSize, const QWidget* widget = nullptr) const override;
    QColor const& splitterColor(oclero::qlementine::MouseState mouse) const override;
    int styleHint(StyleHint hint, const QStyleOption* option = nullptr,
                  const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override;
    QColor const& tabBackgroundColor(oclero::qlementine::MouseState mouse,
                                     oclero::qlementine::SelectionState selected) const override;
    QColor const& tabBarBackgroundColor(oclero::qlementine::MouseState mouse) const override;
    QColor const& tabForegroundColor(oclero::qlementine::MouseState mouse,
                                     oclero::qlementine::SelectionState selected) const override;
    QColor const& tableHeaderBgColor(oclero::qlementine::MouseState mouse,
                                     oclero::qlementine::CheckState checked) const override;
    QColor const& tableHeaderFgColor(oclero::qlementine::MouseState mouse,
                                     oclero::qlementine::CheckState checked) const override;
    QColor const& tableLineColor() const override;
    QColor const& textFieldBackgroundColor(oclero::qlementine::MouseState mouse,
                                           oclero::qlementine::Status status) const override;
    QColor const& textFieldBorderColor(oclero::qlementine::MouseState mouse,
                                       oclero::qlementine::FocusState focus,
                                       oclero::qlementine::Status status) const override;
    QColor const& toolBarBackgroundColor() const override;
    QColor const& toolBarBorderColor() const override;
    QColor const& toolButtonBackgroundColor(oclero::qlementine::MouseState mouse,
                                            oclero::qlementine::ColorRole role) const override;
    QColor const& toolButtonForegroundColor(oclero::qlementine::MouseState mouse,
                                            oclero::qlementine::ColorRole role) const override;
};

#endif // Q_OS_MAC
#endif // MACAPPSTYLE_H
