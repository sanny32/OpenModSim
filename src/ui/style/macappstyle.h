// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file macappstyle.h
/// \brief Declares the macappstyle interfaces.
///

#ifndef MACAPPSTYLE_H
#define MACAPPSTYLE_H

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "qlementineappstyle.h"

#include <oclero/qlementine/style/Theme.hpp>

///
/// \brief The MacAppStyle class
///
class MacAppStyle : public QlementineAppStyle
{
    Q_OBJECT

public:
    explicit MacAppStyle(QObject* parent = nullptr);

    void drawControl(ControlElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget = nullptr) const override;
    QColor const& buttonBackgroundColor(oclero::qlementine::MouseState mouse,
                                        oclero::qlementine::ColorRole role,
                                        const QWidget* widget = nullptr) const override;
    QColor const& buttonForegroundColor(oclero::qlementine::MouseState mouse,
                                        oclero::qlementine::ColorRole role,
                                        const QWidget* widget = nullptr) const override;
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
    QColor const& splitterColor(oclero::qlementine::MouseState mouse) const override;
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
    QColor const& toolTipForegroundColor() const override;
    QColor const& toolButtonBackgroundColor(oclero::qlementine::MouseState mouse,
                                            oclero::qlementine::ColorRole role) const override;
    QColor const& toolButtonForegroundColor(oclero::qlementine::MouseState mouse,
                                            oclero::qlementine::ColorRole role) const override;

private:
    void updateTheme();
    bool isDarkMode() const;

    oclero::qlementine::Theme _lightTheme;
    oclero::qlementine::Theme _darkTheme;
};

#endif // HAVE_QLEMENTINE_APP_STYLE
#endif // MACAPPSTYLE_H
