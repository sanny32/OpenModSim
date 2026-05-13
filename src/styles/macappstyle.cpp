#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "macappstyle.h"
#include "apppreferences.h"

#include <QBrush>
#include <QDockWidget>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHash>
#include <QModelIndex>
#include <QPainter>
#include <QStyleHints>
#include <QStyleOptionDockWidget>
#include <QStyleOptionTab>
#include <QTabBar>

namespace {

using oclero::qlementine::ActiveState;
using oclero::qlementine::CheckState;
using oclero::qlementine::ColorRole;
using oclero::qlementine::FocusState;
using oclero::qlementine::MouseState;
using oclero::qlementine::SelectionState;
using oclero::qlementine::Status;
using oclero::qlementine::Theme;

// Light mode macOS colors
namespace Light {
    constexpr QRgb kCanvas         = 0xffffff;
    constexpr QRgb kChrome         = 0xf2f2f7;
    constexpr QRgb kChromeStrong   = 0xe5e5ea;
    constexpr QRgb kChromePressed  = 0xd1d1d6;
    constexpr QRgb kBorder         = 0xd1d1d6;
    constexpr QRgb kBorderActive   = 0xaeaeb2;
    constexpr QRgb kText           = 0x000000;
    constexpr QRgb kMutedText      = 0x8e8e93;
    constexpr QRgb kDisabledText   = 0xc7c7cc;
    constexpr QRgb kBlue           = 0x007aff;
    constexpr QRgb kBlueHover      = 0x1a8aff;
    constexpr QRgb kBluePressed    = 0x0062cc;
    constexpr QRgb kBlueDisabled   = 0xb3d7ff;
    constexpr QRgb kGreen          = 0x34c759;
    constexpr QRgb kIconNormal     = 0x3c3c43;
    constexpr QRgb kIconActive     = 0x0062cc;
}

// Dark mode macOS colors
namespace Dark {
    constexpr QRgb kCanvas         = 0x1c1c1e;
    constexpr QRgb kChrome         = 0x2c2c2e;
    constexpr QRgb kChromeStrong   = 0x3a3a3c;
    constexpr QRgb kChromePressed  = 0x48484a;
    constexpr QRgb kBorder         = 0x38383a;
    constexpr QRgb kBorderActive   = 0x545456;
    constexpr QRgb kText           = 0xffffff;
    constexpr QRgb kMutedText      = 0x8e8e93;
    constexpr QRgb kDisabledText   = 0x48484a;
    constexpr QRgb kBlue           = 0x0a84ff;
    constexpr QRgb kBlueHover      = 0x409cff;
    constexpr QRgb kBluePressed    = 0x0071e3;
    constexpr QRgb kBlueDisabled   = 0x0a3d73;
    constexpr QRgb kGreen          = 0x30d158;
    constexpr QRgb kIconNormal     = 0xebebf5;
    constexpr QRgb kIconActive     = 0x409cff;
}

QColor transparent(QRgb rgb)
{
    QColor color(rgb);
    color.setAlpha(0);
    return color;
}

QColor alpha(QRgb rgb, int value)
{
    QColor color(rgb);
    color.setAlpha(value);
    return color;
}

struct ColorKey { QRgb rgb; int alpha; };

const QColor& colorRef(QRgb rgb)
{
    static QHash<QRgb, QColor> colors;
    auto it = colors.find(rgb);
    if (it == colors.end())
        it = colors.insert(rgb, QColor(rgb));
    return it.value();
}

const QColor& transparentRef(QRgb rgb)
{
    static QHash<QRgb, QColor> colors;
    auto it = colors.find(rgb);
    if (it == colors.end())
        it = colors.insert(rgb, transparent(rgb));
    return it.value();
}

QColor modelBackgroundColor(const QModelIndex& index)
{
    const QVariant background = index.data(Qt::BackgroundRole);
    if (!background.isValid())
        return {};

    const QBrush brush = qvariant_cast<QBrush>(background);
    if (brush.style() != Qt::NoBrush && brush.color().isValid())
        return brush.color();

    const QColor color = qvariant_cast<QColor>(background);
    if (color.isValid())
        return color;

    return {};
}

Theme makeMacLightTheme()
{
    using namespace Light;
    Theme theme;
    theme.meta.name = QStringLiteral("Open ModSim macOS Light");
    theme.meta.version = QStringLiteral("1.0");
    theme.meta.author = QStringLiteral("Open ModSim");

    theme.backgroundColorMain1 = QColor(kCanvas);
    theme.backgroundColorMain2 = QColor(kChrome);
    theme.backgroundColorMain3 = QColor(kChromeStrong);
    theme.backgroundColorMain4 = QColor(kChromePressed);
    theme.backgroundColorMainTransparent = transparent(kCanvas);
    theme.backgroundColorWorkspace = QColor(kCanvas);
    theme.backgroundColorTabBar = QColor(kChrome);

    theme.neutralColor = QColor(kChromeStrong);
    theme.neutralColorHovered = QColor(kChromePressed);
    theme.neutralColorPressed = QColor(0xc7c7cc);
    theme.neutralColorDisabled = QColor(0xf2f2f7);
    theme.neutralColorTransparent = transparent(kChromeStrong);

    theme.primaryColor = QColor(kBlue);
    theme.primaryColorHovered = QColor(kBlueHover);
    theme.primaryColorPressed = QColor(kBluePressed);
    theme.primaryColorDisabled = QColor(kBlueDisabled);
    theme.primaryColorTransparent = transparent(kBlue);

    theme.primaryColorForeground = QColor(kCanvas);
    theme.primaryColorForegroundHovered = QColor(kCanvas);
    theme.primaryColorForegroundPressed = QColor(kCanvas);
    theme.primaryColorForegroundDisabled = QColor(0xebf4fe);
    theme.primaryColorForegroundTransparent = transparent(kCanvas);

    theme.secondaryColor = QColor(kText);
    theme.secondaryColorHovered = QColor(0x1c1c1e);
    theme.secondaryColorPressed = QColor(0x2c2c2e);
    theme.secondaryColorDisabled = QColor(kDisabledText);
    theme.secondaryColorTransparent = transparent(kText);

    theme.secondaryColorForeground = QColor(0x3c3c43);
    theme.secondaryColorForegroundHovered = QColor(kBluePressed);
    theme.secondaryColorForegroundPressed = QColor(0x0051a8);
    theme.secondaryColorForegroundDisabled = QColor(kDisabledText);
    theme.secondaryColorForegroundTransparent = transparent(0x3c3c43);

    theme.secondaryAlternativeColor = QColor(kMutedText);
    theme.secondaryAlternativeColorHovered = QColor(kText);
    theme.secondaryAlternativeColorPressed = QColor(kText);
    theme.secondaryAlternativeColorDisabled = QColor(kDisabledText);
    theme.secondaryAlternativeColorTransparent = transparent(kMutedText);

    theme.statusColorSuccess = QColor(kGreen);
    theme.statusColorSuccessHovered = QColor(0x2fbd54);
    theme.statusColorSuccessPressed = QColor(0x29a94a);
    theme.statusColorSuccessDisabled = QColor(0xccefd6);
    theme.statusColorInfo = QColor(kBlue);
    theme.statusColorInfoHovered = QColor(kBlueHover);
    theme.statusColorInfoPressed = QColor(kBluePressed);
    theme.statusColorInfoDisabled = QColor(kBlueDisabled);

    theme.borderColor = QColor(kBorder);
    theme.borderColorHovered = QColor(kBorderActive);
    theme.borderColorPressed = QColor(0x9e9ea3);
    theme.borderColorDisabled = QColor(0xe5e5ea);
    theme.borderColorTransparent = transparent(kBorder);

    theme.semiTransparentColor1 = alpha(kText, 0);
    theme.semiTransparentColor2 = alpha(kText, 18);
    theme.semiTransparentColor3 = alpha(kText, 26);
    theme.semiTransparentColor4 = alpha(kText, 36);
    theme.semiTransparentColorTransparent = transparent(kText);

    theme.shadowColor1 = alpha(0x000000, 14);
    theme.shadowColor2 = alpha(0x000000, 20);
    theme.shadowColor3 = alpha(0x000000, 32);
    theme.shadowColorTransparent = alpha(0x000000, 0);

    theme.useSystemFonts = true;
    theme.fontSize = 13;
    theme.fontSizeMonospace = 13;
    theme.fontSizeS1 = 11;
    theme.fontRegular = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    theme.fontRegular.setPointSize(theme.fontSize);
    theme.fontBold = theme.fontRegular;
    theme.fontBold.setWeight(QFont::DemiBold);
    theme.fontCaption = theme.fontRegular;
    theme.fontCaption.setPointSize(theme.fontSizeS1);
    theme.fontMonospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    theme.fontMonospace.setPointSize(theme.fontSizeMonospace);
    theme.borderRadius = 6.0;
    theme.checkBoxBorderRadius = 4.0;
    theme.borderWidth = 1;
    theme.controlHeightLarge = 30;
    theme.controlHeightMedium = 26;
    theme.controlHeightSmall = 18;
    theme.controlDefaultWidth = 104;
    theme.iconSize = QSize(16, 16);
    theme.iconSizeMedium = QSize(22, 22);
    theme.iconSizeLarge = QSize(24, 24);
    theme.iconSizeExtraSmall = QSize(12, 12);
    theme.spacing = 8;
    theme.scrollBarThicknessFull = 12;
    theme.scrollBarThicknessSmall = 6;
    theme.tabBarPaddingTop = 2;
    theme.tabBarTabMinWidth = 0;
    theme.tabBarTabMaxWidth = 0;

    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Window, theme.backgroundColorMain2);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Base, theme.backgroundColorMain1);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::AlternateBase, QColor(kChrome));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Button, theme.neutralColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Text, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::WindowText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ButtonText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipBase, QColor(kText));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipText, QColor(kCanvas));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Highlight, theme.primaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::HighlightedText, theme.primaryColorForeground);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Text, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::WindowText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ButtonText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ToolTipText, theme.primaryColorForegroundDisabled);

    return theme;
}

Theme makeMacDarkTheme()
{
    using namespace Dark;
    Theme theme;
    theme.meta.name = QStringLiteral("Open ModSim macOS Dark");
    theme.meta.version = QStringLiteral("1.0");
    theme.meta.author = QStringLiteral("Open ModSim");

    theme.backgroundColorMain1 = QColor(kCanvas);
    theme.backgroundColorMain2 = QColor(kChrome);
    theme.backgroundColorMain3 = QColor(kChromeStrong);
    theme.backgroundColorMain4 = QColor(kChromePressed);
    theme.backgroundColorMainTransparent = transparent(kCanvas);
    theme.backgroundColorWorkspace = QColor(kCanvas);
    theme.backgroundColorTabBar = QColor(kChrome);

    theme.neutralColor = QColor(kChromeStrong);
    theme.neutralColorHovered = QColor(kChromePressed);
    theme.neutralColorPressed = QColor(0x545456);
    theme.neutralColorDisabled = QColor(0x2c2c2e);
    theme.neutralColorTransparent = transparent(kChromeStrong);

    theme.primaryColor = QColor(kBlue);
    theme.primaryColorHovered = QColor(kBlueHover);
    theme.primaryColorPressed = QColor(kBluePressed);
    theme.primaryColorDisabled = QColor(kBlueDisabled);
    theme.primaryColorTransparent = transparent(kBlue);

    theme.primaryColorForeground = QColor(kCanvas);
    theme.primaryColorForegroundHovered = QColor(kCanvas);
    theme.primaryColorForegroundPressed = QColor(kCanvas);
    theme.primaryColorForegroundDisabled = QColor(0x0a2a4a);
    theme.primaryColorForegroundTransparent = transparent(kCanvas);

    theme.secondaryColor = QColor(kText);
    theme.secondaryColorHovered = QColor(0xebebf5);
    theme.secondaryColorPressed = QColor(0xd1d1d6);
    theme.secondaryColorDisabled = QColor(kDisabledText);
    theme.secondaryColorTransparent = transparent(kText);

    theme.secondaryColorForeground = QColor(0xebebf5);
    theme.secondaryColorForegroundHovered = QColor(kBlueHover);
    theme.secondaryColorForegroundPressed = QColor(kBlue);
    theme.secondaryColorForegroundDisabled = QColor(kDisabledText);
    theme.secondaryColorForegroundTransparent = transparent(0xebebf5);

    theme.secondaryAlternativeColor = QColor(kMutedText);
    theme.secondaryAlternativeColorHovered = QColor(kText);
    theme.secondaryAlternativeColorPressed = QColor(kText);
    theme.secondaryAlternativeColorDisabled = QColor(kDisabledText);
    theme.secondaryAlternativeColorTransparent = transparent(kMutedText);

    theme.statusColorSuccess = QColor(kGreen);
    theme.statusColorSuccessHovered = QColor(0x26b84e);
    theme.statusColorSuccessPressed = QColor(0x1fa344);
    theme.statusColorSuccessDisabled = QColor(0x0f3d20);
    theme.statusColorInfo = QColor(kBlue);
    theme.statusColorInfoHovered = QColor(kBlueHover);
    theme.statusColorInfoPressed = QColor(kBluePressed);
    theme.statusColorInfoDisabled = QColor(kBlueDisabled);

    theme.borderColor = QColor(kBorder);
    theme.borderColorHovered = QColor(kBorderActive);
    theme.borderColorPressed = QColor(0x636366);
    theme.borderColorDisabled = QColor(0x2c2c2e);
    theme.borderColorTransparent = transparent(kBorder);

    theme.semiTransparentColor1 = alpha(kText, 0);
    theme.semiTransparentColor2 = alpha(kText, 20);
    theme.semiTransparentColor3 = alpha(kText, 30);
    theme.semiTransparentColor4 = alpha(kText, 45);
    theme.semiTransparentColorTransparent = transparent(kText);

    theme.shadowColor1 = alpha(0x000000, 40);
    theme.shadowColor2 = alpha(0x000000, 55);
    theme.shadowColor3 = alpha(0x000000, 70);
    theme.shadowColorTransparent = alpha(0x000000, 0);

    theme.useSystemFonts = true;
    theme.fontSize = 13;
    theme.fontSizeMonospace = 13;
    theme.fontSizeS1 = 11;
    theme.fontRegular = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    theme.fontRegular.setPointSize(theme.fontSize);
    theme.fontBold = theme.fontRegular;
    theme.fontBold.setWeight(QFont::DemiBold);
    theme.fontCaption = theme.fontRegular;
    theme.fontCaption.setPointSize(theme.fontSizeS1);
    theme.fontMonospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    theme.fontMonospace.setPointSize(theme.fontSizeMonospace);
    theme.borderRadius = 6.0;
    theme.checkBoxBorderRadius = 4.0;
    theme.borderWidth = 1;
    theme.controlHeightLarge = 30;
    theme.controlHeightMedium = 26;
    theme.controlHeightSmall = 18;
    theme.controlDefaultWidth = 104;
    theme.iconSize = QSize(16, 16);
    theme.iconSizeMedium = QSize(22, 22);
    theme.iconSizeLarge = QSize(24, 24);
    theme.iconSizeExtraSmall = QSize(12, 12);
    theme.spacing = 8;
    theme.scrollBarThicknessFull = 12;
    theme.scrollBarThicknessSmall = 6;
    theme.tabBarPaddingTop = 2;
    theme.tabBarTabMinWidth = 0;
    theme.tabBarTabMaxWidth = 0;

    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Window, theme.backgroundColorMain2);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Base, theme.backgroundColorMain1);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::AlternateBase, QColor(kChrome));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Button, theme.neutralColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Text, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::WindowText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ButtonText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipBase, QColor(kChromeStrong));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipText, QColor(kText));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Highlight, theme.primaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::HighlightedText, theme.primaryColorForeground);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Text, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::WindowText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ButtonText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ToolTipText, theme.primaryColorForegroundDisabled);

    return theme;
}

} // namespace

///
/// \brief MacAppStyle::MacAppStyle
/// \param parent
///
MacAppStyle::MacAppStyle(QObject* parent)
    : QlementineAppStyle(parent)
    , _lightTheme(makeMacLightTheme())
    , _darkTheme(makeMacDarkTheme())
{
    updateTheme();

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, [this](Qt::ColorScheme) { updateTheme(); });
    connect(&AppPreferences::instance(), &AppPreferences::settingChanged,
            this, [this](const QString& name, const QString&, const QString&) {
                if (name == QLatin1String("ThemeMode"))
                    updateTheme();
            });
}

void MacAppStyle::updateTheme()
{
    setTheme(isDarkMode() ? _darkTheme : _lightTheme);
}

bool MacAppStyle::isDarkMode() const
{
    switch (AppPreferences::instance().themeMode()) {
        case AppThemeMode::Light:
            return false;
        case AppThemeMode::Dark:
            return true;
        case AppThemeMode::System:
        default:
            return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    }
}

///
/// \brief MacAppStyle::drawControl
///
void MacAppStyle::drawControl(ControlElement element, const QStyleOption* option,
                               QPainter* painter, const QWidget* widget) const
{
    if (element == CE_DockWidgetTitle) {
        const auto* dockOption = qstyleoption_cast<const QStyleOptionDockWidget*>(option);
        if (!dockOption) {
            QlementineAppStyle::drawControl(element, option, painter, widget);
            return;
        }

        const QRect rect = dockOption->rect;
        const QRgb bgColor = isDarkMode() ? Dark::kChrome : Light::kChrome;
        const QRgb borderColor = isDarkMode() ? Dark::kBorder : Light::kBorder;
        const QRgb textColor = isDarkMode() ? Dark::kText : Light::kText;

        painter->save();
        painter->fillRect(rect, colorRef(bgColor));
        painter->setPen(QPen(colorRef(borderColor), 1));
        painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

        QRect textRect = rect.adjusted(6, 0, -42, 0);
        if (textRect.isValid()) {
            QFont font = painter->font();
            font.setPointSize(qMax(11, font.pointSize()));
            painter->setFont(font);
            painter->setPen(colorRef(textColor));
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                              painter->fontMetrics().elidedText(dockOption->title, Qt::ElideRight, textRect.width()));
        }
        painter->restore();
        return;
    }

    QlementineAppStyle::drawControl(element, option, painter, widget);
}

///
/// \brief MacAppStyle::buttonBackgroundColor
///
QColor const& MacAppStyle::buttonBackgroundColor(MouseState mouse, ColorRole role, const QWidget* widget) const
{
    Q_UNUSED(widget)

    if (role == ColorRole::Primary)
        return QlementineAppStyle::buttonBackgroundColor(mouse, role, widget);

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
            case MouseState::Disabled:
                return colorRef(0x2c2c2e);
            case MouseState::Transparent:
                return transparentRef(kChromeStrong);
            case MouseState::Normal:
            default:
                return colorRef(0x3a3a3c);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
            case MouseState::Disabled:
                return colorRef(0xf4f6f8);
            case MouseState::Transparent:
                return transparentRef(kChromeStrong);
            case MouseState::Normal:
            default:
                return colorRef(0xfefefe);
        }
    }
}

///
/// \brief MacAppStyle::buttonForegroundColor
///
QColor const& MacAppStyle::buttonForegroundColor(MouseState mouse, ColorRole role, const QWidget* widget) const
{
    Q_UNUSED(widget)

    if (role == ColorRole::Primary)
        return QlementineAppStyle::buttonForegroundColor(mouse, role, widget);

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    }
}

///
/// \brief MacAppStyle::iconForegroundColor
///
QColor const& MacAppStyle::iconForegroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineAppStyle::iconForegroundColor(mouse, role);

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kIconActive);
            case MouseState::Disabled:
                return colorRef(kDisabledText);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return colorRef(kIconNormal);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kIconActive);
            case MouseState::Disabled:
                return colorRef(kDisabledText);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return colorRef(kIconNormal);
        }
    }
}

///
/// \brief MacAppStyle::listItemBackgroundColor
///
QColor MacAppStyle::listItemBackgroundColor(MouseState mouse, SelectionState selected, FocusState focus,
                                             ActiveState active, const QModelIndex& index,
                                             const QWidget* widget) const
{
    Q_UNUSED(focus)
    Q_UNUSED(active)
    Q_UNUSED(widget)

    const bool isSelected = selected == SelectionState::Selected;
    const QColor rowColor = modelBackgroundColor(index);

    if (isDarkMode()) {
        using namespace Dark;
        if (isSelected)
            return mouse == MouseState::Disabled ? QColor(0x3a3a3c) : QColor(0x0a3d73);
        switch (mouse) {
            case MouseState::Hovered:
                return QColor(0x3a3a3c);
            case MouseState::Pressed:
                return QColor(0x48484a);
            case MouseState::Disabled:
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                if (rowColor.isValid())
                    return rowColor;
                return transparent(kCanvas);
        }
    } else {
        using namespace Light;
        if (isSelected)
            return mouse == MouseState::Disabled ? QColor(0xe5e5ea) : QColor(0xd9eaff);
        switch (mouse) {
            case MouseState::Hovered:
                return QColor(0xf2f2f7);
            case MouseState::Pressed:
                return QColor(0xe5e5ea);
            case MouseState::Disabled:
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                if (rowColor.isValid())
                    return rowColor;
                return transparent(kCanvas);
        }
    }
}

///
/// \brief MacAppStyle::listItemForegroundColor
///
QColor const& MacAppStyle::listItemForegroundColor(MouseState mouse, SelectionState selected,
                                                    FocusState focus, ActiveState active) const
{
    Q_UNUSED(selected)
    Q_UNUSED(focus)
    Q_UNUSED(active)

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    }
}

///
/// \brief MacAppStyle::splitterColor
///
QColor const& MacAppStyle::splitterColor(MouseState mouse) const
{
    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kBorderActive);
            default:
                return colorRef(kBorder);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kBorderActive);
            default:
                return colorRef(kBorder);
        }
    }
}

///
/// \brief MacAppStyle::tabBackgroundColor
///
QColor const& MacAppStyle::tabBackgroundColor(MouseState mouse, SelectionState selected) const
{
    const bool isSelected = selected == SelectionState::Selected;

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Pressed:
                return isSelected ? colorRef(kCanvas) : colorRef(kChromePressed);
            case MouseState::Hovered:
                return isSelected ? colorRef(kCanvas) : colorRef(kChromeStrong);
            case MouseState::Disabled:
                return transparentRef(kChrome);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return isSelected ? colorRef(kCanvas) : transparentRef(kChrome);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Pressed:
                return isSelected ? colorRef(kCanvas) : colorRef(kChromePressed);
            case MouseState::Hovered:
                return isSelected ? colorRef(kCanvas) : colorRef(kChromeStrong);
            case MouseState::Disabled:
                return transparentRef(kChrome);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return isSelected ? colorRef(kCanvas) : transparentRef(kChrome);
        }
    }
}

///
/// \brief MacAppStyle::tabBarBackgroundColor
///
QColor const& MacAppStyle::tabBarBackgroundColor(MouseState mouse) const
{
    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kChromeStrong) : colorRef(kChrome);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kChromeStrong) : colorRef(kChrome);
    }
}

///
/// \brief MacAppStyle::tabForegroundColor
///
QColor const& MacAppStyle::tabForegroundColor(MouseState mouse, SelectionState selected) const
{
    Q_UNUSED(selected)

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    }
}

///
/// \brief MacAppStyle::tableHeaderBgColor
///
QColor const& MacAppStyle::tableHeaderBgColor(MouseState mouse, CheckState checked) const
{
    Q_UNUSED(checked)

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
            case MouseState::Disabled:
                return colorRef(0x2c2c2e);
            default:
                return colorRef(kChrome);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
            case MouseState::Disabled:
                return colorRef(0xf2f2f7);
            default:
                return colorRef(kCanvas);
        }
    }
}

///
/// \brief MacAppStyle::tableHeaderFgColor
///
QColor const& MacAppStyle::tableHeaderFgColor(MouseState mouse, CheckState checked) const
{
    Q_UNUSED(checked)

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
    }
}

///
/// \brief MacAppStyle::tableLineColor
///
QColor const& MacAppStyle::tableLineColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kBorder);
    else
        return colorRef(0xe5e5ea);
}

///
/// \brief MacAppStyle::textFieldBackgroundColor
///
QColor const& MacAppStyle::textFieldBackgroundColor(MouseState mouse, Status status) const
{
    Q_UNUSED(status)

    if (isDarkMode()) {
        using namespace Dark;
        return mouse == MouseState::Disabled ? colorRef(0x2c2c2e) : colorRef(kCanvas);
    } else {
        using namespace Light;
        return mouse == MouseState::Disabled ? colorRef(0xf2f2f7) : colorRef(kCanvas);
    }
}

///
/// \brief MacAppStyle::textFieldBorderColor
///
QColor const& MacAppStyle::textFieldBorderColor(MouseState mouse, FocusState focus, Status status) const
{
    if (status != Status::Default)
        return QlementineAppStyle::textFieldBorderColor(mouse, focus, status);

    if (isDarkMode()) {
        using namespace Dark;
        if (mouse == MouseState::Disabled)
            return colorRef(0x2c2c2e);
        if (focus == FocusState::Focused)
            return colorRef(kBlue);
        if (mouse == MouseState::Hovered || mouse == MouseState::Pressed)
            return colorRef(kBorderActive);
        return colorRef(kBorder);
    } else {
        using namespace Light;
        if (mouse == MouseState::Disabled)
            return colorRef(0xe5e5ea);
        if (focus == FocusState::Focused)
            return colorRef(kBlue);
        if (mouse == MouseState::Hovered || mouse == MouseState::Pressed)
            return colorRef(kBorderActive);
        return colorRef(kBorder);
    }
}

///
/// \brief MacAppStyle::toolBarBackgroundColor
///
QColor const& MacAppStyle::toolBarBackgroundColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kChrome);
    else
        return colorRef(Light::kChrome);
}

///
/// \brief MacAppStyle::toolBarBorderColor
///
QColor const& MacAppStyle::toolBarBorderColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kBorder);
    else
        return colorRef(Light::kBorder);
}

///
/// \brief MacAppStyle::toolTipForegroundColor
///
QColor const& MacAppStyle::toolTipForegroundColor() const
{
    if (isDarkMode())
        return colorRef(Dark::kText);
    else
        return colorRef(Light::kCanvas);
}

///
/// \brief MacAppStyle::toolButtonBackgroundColor
///
QColor const& MacAppStyle::toolButtonBackgroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineAppStyle::toolButtonBackgroundColor(mouse, role);

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
            case MouseState::Disabled:
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return transparentRef(kChrome);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Pressed:
                return colorRef(kChromePressed);
            case MouseState::Hovered:
                return colorRef(kChromeStrong);
            case MouseState::Disabled:
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return transparentRef(kChrome);
        }
    }
}

///
/// \brief MacAppStyle::toolButtonForegroundColor
///
QColor const& MacAppStyle::toolButtonForegroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineAppStyle::toolButtonForegroundColor(mouse, role);

    if (isDarkMode()) {
        using namespace Dark;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(kIconActive);
            case MouseState::Disabled:
                return colorRef(kDisabledText);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return colorRef(kText);
        }
    } else {
        using namespace Light;
        switch (mouse) {
            case MouseState::Hovered:
            case MouseState::Pressed:
                return colorRef(0x0051a8);
            case MouseState::Disabled:
                return colorRef(kDisabledText);
            case MouseState::Transparent:
            case MouseState::Normal:
            default:
                return colorRef(kText);
        }
    }
}

#endif // HAVE_QLEMENTINE_APP_STYLE
