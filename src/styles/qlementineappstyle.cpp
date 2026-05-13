#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "qlementineappstyle.h"
#include "apppreferences.h"
#include <oclero/qlementine/icons/QlementineIcons.hpp>

#include <QComboBox>
#include <QDockWidget>
#include <QGuiApplication>
#include <QPushButton>
#include <QBrush>
#include <QHash>
#include <QLineEdit>
#include <QLabel>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QStyleOptionDockWidget>
#include <QStyleOptionTab>
#include <QStyleOptionToolButton>
#include <QStyleHints>
#include <QTabBar>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

namespace {

using oclero::qlementine::ActiveState;
using oclero::qlementine::CheckState;
using oclero::qlementine::ColorRole;
using oclero::qlementine::FocusState;
using oclero::qlementine::MouseState;
using oclero::qlementine::SelectionState;
using oclero::qlementine::Status;
using oclero::qlementine::Theme;

namespace Light {
constexpr QRgb kCanvas = 0xffffff;
constexpr QRgb kChrome = 0xf5f5f5;
constexpr QRgb kChromeStrong = 0xececec;
constexpr QRgb kChromePressed = 0xe0e0e0;
constexpr QRgb kChromeMuted = 0xfefefe;
constexpr QRgb kChromeDisabled = 0xf4f6f8;
constexpr QRgb kSurfaceDisabled = 0xf2f2f2;
constexpr QRgb kBorder = 0xe3e6ea;
constexpr QRgb kBorderActive = 0xcdd1d6;
constexpr QRgb kBorderMuted = 0xe6e9ee;
constexpr QRgb kBorderDisabled = 0xebeef2;
constexpr QRgb kText = 0x1f2937;
constexpr QRgb kTextMuted = 0x4f627a;
constexpr QRgb kTextEmphasis = 0x24364d;
constexpr QRgb kDisabledText = 0xb2bac5;
constexpr QRgb kBlue = 0x007aff;
constexpr QRgb kBlueHover = 0x1a8aff;
constexpr QRgb kBluePressed = 0x0062cc;
constexpr QRgb kBlueDisabled = 0xb3d7ff;
constexpr QRgb kSelection = 0xd9eaff;
constexpr QRgb kSelectionDisabled = 0xeaeaea;
constexpr QRgb kItemHover = 0xf0f0f0;
constexpr QRgb kItemPressed = 0xe5e5e5;
constexpr QRgb kTooltipBase = 0x1f2937;
constexpr QRgb kTooltipText = 0xffffff;
}

namespace Dark {
constexpr QRgb kCanvas = 0x1e1f24;
constexpr QRgb kChrome = 0x272a30;
constexpr QRgb kChromeStrong = 0x333841;
constexpr QRgb kChromePressed = 0x414854;
constexpr QRgb kChromeMuted = 0x2d3138;
constexpr QRgb kChromeDisabled = 0x24272d;
constexpr QRgb kSurfaceDisabled = 0x21242a;
constexpr QRgb kBorder = 0x3b414c;
constexpr QRgb kBorderActive = 0x5b6575;
constexpr QRgb kBorderMuted = 0x313740;
constexpr QRgb kBorderDisabled = 0x2a2e36;
constexpr QRgb kText = 0xe7ecf3;
constexpr QRgb kTextMuted = 0xa0abbb;
constexpr QRgb kTextEmphasis = 0xffffff;
constexpr QRgb kDisabledText = 0x6c7582;
constexpr QRgb kBlue = 0x409cff;
constexpr QRgb kBlueHover = 0x66b2ff;
constexpr QRgb kBluePressed = 0x1f8bff;
constexpr QRgb kBlueDisabled = 0x27476a;
constexpr QRgb kSelection = 0x1d3c5c;
constexpr QRgb kSelectionDisabled = 0x2b3139;
constexpr QRgb kItemHover = 0x2e333b;
constexpr QRgb kItemPressed = 0x373d46;
constexpr QRgb kTooltipBase = 0xe7ecf3;
constexpr QRgb kTooltipText = 0x111318;
}

///
/// \brief transparent
/// \param rgb
/// \return
///
QColor transparent(QRgb rgb)
{
    QColor color(rgb);
    color.setAlpha(0);
    return color;
}

///
/// \brief alpha
/// \param rgb
/// \param value
/// \return
///
QColor alpha(QRgb rgb, int value)
{
    QColor color(rgb);
    color.setAlpha(value);
    return color;
}

///
/// \brief colorRef
/// \param rgb
/// \return
///
const QColor& colorRef(QRgb rgb)
{
    static QHash<QRgb, QColor> colors;
    auto it = colors.find(rgb);
    if (it == colors.end())
        it = colors.insert(rgb, QColor(rgb));
    return it.value();
}

///
/// \brief transparentRef
/// \param rgb
/// \return
///
const QColor& transparentRef(QRgb rgb)
{
    static QHash<QRgb, QColor> colors;
    auto it = colors.find(rgb);
    if (it == colors.end())
        it = colors.insert(rgb, transparent(rgb));
    return it.value();
}

///
/// \brief modelBackgroundColor
/// \param index
/// \return
///
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

///
/// \brief makeQlementineAppTheme
/// \param baseTheme
/// \param darkMode
/// \return
///
Theme makeQlementineAppTheme(const Theme& baseTheme, bool darkMode)
{
    const auto rgb = [darkMode](QRgb light, QRgb dark) { return darkMode ? dark : light; };
    const auto color = [&rgb](QRgb light, QRgb dark) { return QColor(rgb(light, dark)); };

    Theme theme = baseTheme;
    theme.meta.name = darkMode
        ? QStringLiteral("Open ModSim Qlementine Dark")
        : QStringLiteral("Open ModSim Qlementine Light");
    theme.meta.version = QStringLiteral("1.0");
    theme.meta.author = QStringLiteral("Open ModSim");

    theme.backgroundColorMain1 = color(Light::kCanvas, Dark::kCanvas);
    theme.backgroundColorMain2 = color(Light::kChrome, Dark::kChrome);
    theme.backgroundColorMain3 = color(Light::kChromeStrong, Dark::kChromeStrong);
    theme.backgroundColorMain4 = color(Light::kChromePressed, Dark::kChromePressed);
    theme.backgroundColorMainTransparent = transparent(rgb(Light::kCanvas, Dark::kCanvas));
    theme.backgroundColorWorkspace = color(Light::kCanvas, Dark::kCanvas);
    theme.backgroundColorTabBar = color(Light::kChrome, Dark::kChrome);

    theme.neutralColor = color(Light::kChromeStrong, Dark::kChromeStrong);
    theme.neutralColorHovered = color(Light::kChromePressed, Dark::kChromePressed);
    theme.neutralColorPressed = color(Light::kBorderActive, Dark::kBorderActive);
    theme.neutralColorDisabled = color(Light::kChromeDisabled, Dark::kChromeDisabled);
    theme.neutralColorTransparent = transparent(rgb(Light::kChromeStrong, Dark::kChromeStrong));

    theme.primaryColor = color(Light::kBlue, Dark::kBlue);
    theme.primaryColorHovered = color(Light::kBlueHover, Dark::kBlueHover);
    theme.primaryColorPressed = color(Light::kBluePressed, Dark::kBluePressed);
    theme.primaryColorDisabled = color(Light::kBlueDisabled, Dark::kBlueDisabled);
    theme.primaryColorTransparent = transparent(rgb(Light::kBlue, Dark::kBlue));

    theme.primaryColorForeground = color(Light::kTooltipText, Dark::kTextEmphasis);
    theme.primaryColorForegroundHovered = color(Light::kTooltipText, Dark::kTextEmphasis);
    theme.primaryColorForegroundPressed = color(Light::kTooltipText, Dark::kTextEmphasis);
    theme.primaryColorForegroundDisabled = color(Light::kChromeMuted, Dark::kDisabledText);
    theme.primaryColorForegroundTransparent = transparent(rgb(Light::kTooltipText, Dark::kTextEmphasis));

    theme.secondaryColor = color(Light::kText, Dark::kText);
    theme.secondaryColorHovered = color(Light::kTextEmphasis, Dark::kTextEmphasis);
    theme.secondaryColorPressed = color(Light::kTextEmphasis, Dark::kTextEmphasis);
    theme.secondaryColorDisabled = color(Light::kDisabledText, Dark::kDisabledText);
    theme.secondaryColorTransparent = transparent(rgb(Light::kText, Dark::kText));

    theme.secondaryColorForeground = color(Light::kTextMuted, Dark::kTextMuted);
    theme.secondaryColorForegroundHovered = color(Light::kBluePressed, Dark::kBlueHover);
    theme.secondaryColorForegroundPressed = color(Light::kBluePressed, Dark::kBluePressed);
    theme.secondaryColorForegroundDisabled = color(Light::kDisabledText, Dark::kDisabledText);
    theme.secondaryColorForegroundTransparent = transparent(rgb(Light::kTextMuted, Dark::kTextMuted));

    theme.secondaryAlternativeColor = color(Light::kTextMuted, Dark::kTextMuted);
    theme.secondaryAlternativeColorHovered = color(Light::kText, Dark::kText);
    theme.secondaryAlternativeColorPressed = color(Light::kText, Dark::kTextEmphasis);
    theme.secondaryAlternativeColorDisabled = color(Light::kDisabledText, Dark::kDisabledText);
    theme.secondaryAlternativeColorTransparent = transparent(rgb(Light::kTextMuted, Dark::kTextMuted));

    theme.borderColor = color(Light::kBorder, Dark::kBorder);
    theme.borderColorHovered = color(Light::kBorderActive, Dark::kBorderActive);
    theme.borderColorPressed = color(Light::kBorderActive, Dark::kBorderActive);
    theme.borderColorDisabled = color(Light::kBorderDisabled, Dark::kBorderDisabled);
    theme.borderColorTransparent = transparent(rgb(Light::kBorder, Dark::kBorder));

    theme.semiTransparentColor1 = alpha(rgb(Light::kText, Dark::kText), 0);
    theme.semiTransparentColor2 = alpha(rgb(Light::kText, Dark::kText), darkMode ? 28 : 18);
    theme.semiTransparentColor3 = alpha(rgb(Light::kText, Dark::kText), darkMode ? 42 : 26);
    theme.semiTransparentColor4 = alpha(rgb(Light::kText, Dark::kText), darkMode ? 58 : 36);
    theme.semiTransparentColorTransparent = transparent(rgb(Light::kText, Dark::kText));

    theme.shadowColor1 = alpha(0x000000, darkMode ? 32 : 14);
    theme.shadowColor2 = alpha(0x000000, darkMode ? 44 : 20);
    theme.shadowColor3 = alpha(0x000000, darkMode ? 64 : 32);
    theme.shadowColorTransparent = alpha(0x000000, 0);

    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Window, theme.backgroundColorMain2);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Base, theme.backgroundColorMain1);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::AlternateBase, theme.backgroundColorMain3);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Button, theme.neutralColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Text, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::WindowText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ButtonText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipBase, color(Light::kTooltipBase, Dark::kTooltipBase));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipText, color(Light::kTooltipText, Dark::kTooltipText));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Highlight, theme.primaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::HighlightedText, theme.primaryColorForeground);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::PlaceholderText, theme.secondaryColorForeground);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Mid, theme.borderColor);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Base, color(Light::kSurfaceDisabled, Dark::kSurfaceDisabled));
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Button, color(Light::kChromeDisabled, Dark::kChromeDisabled));
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Text, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::WindowText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ButtonText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Highlight, theme.primaryColorDisabled);

    return theme;
}

///
/// \brief tabShapePadding
/// \param option
/// \param spacing
/// \return
///
QMargins tabShapePadding(const QStyleOptionTab& option, int spacing)
{
    const bool isFirst = option.position == QStyleOptionTab::OnlyOneTab
        || option.position == QStyleOptionTab::Beginning;
    const bool isLast = option.position == QStyleOptionTab::OnlyOneTab
        || option.position == QStyleOptionTab::End;
    const bool notBesideSelected = option.selectedPosition == QStyleOptionTab::NotAdjacent;
    const bool onlyOneTab = option.position == QStyleOptionTab::OnlyOneTab;
    const bool isMovedTab = notBesideSelected && onlyOneTab;

    return QMargins(isMovedTab || isFirst ? spacing : 0,
                    spacing / 2,
                    isMovedTab || isLast ? spacing : 0,
                    0);
}

} // namespace

///
/// \brief QlementineAppStyle::QlementineAppStyle
/// \param parent
///
QlementineAppStyle::QlementineAppStyle(QObject* parent)
    : QlementineStyle(parent)
{
    setAutoIconColor(oclero::qlementine::AutoIconColor::ForegroundColor);

    oclero::qlementine::icons::initializeIconTheme();
    QIcon::setThemeName(QStringLiteral("qlementine"));

    const Theme baseTheme = theme();
    _lightTheme = makeQlementineAppTheme(baseTheme, false);
    _darkTheme = makeQlementineAppTheme(baseTheme, true);
    updateTheme();

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, [this](Qt::ColorScheme) { updateTheme(); });
    connect(&AppPreferences::instance(), &AppPreferences::settingChanged,
            this, [this](const QString& name, const QString&, const QString&) {
                if (name == QLatin1String("ThemeMode"))
                    updateTheme();
            });
}

void QlementineAppStyle::updateTheme()
{
    setTheme(isDarkMode() ? _darkTheme : _lightTheme);
}

bool QlementineAppStyle::isDarkMode() const
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
/// \brief QlementineAppStyle::buttonBackgroundColor
/// \param mouse
/// \param role
/// \param widget
/// \return
///
QColor const& QlementineAppStyle::buttonBackgroundColor(MouseState mouse, ColorRole role,
                                                  const QWidget* widget) const
{
    Q_UNUSED(widget)

    if (role == ColorRole::Primary)
        return QlementineStyle::buttonBackgroundColor(mouse, role, widget);

    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kChromePressed : Light::kChromePressed);
        case MouseState::Hovered:
            return colorRef(darkMode ? Dark::kChromeStrong : Light::kChromeStrong);
        case MouseState::Disabled:
            return colorRef(darkMode ? Dark::kChromeDisabled : Light::kChromeDisabled);
        case MouseState::Transparent:
            return transparentRef(darkMode ? Dark::kChromeStrong : Light::kChromeStrong);
        case MouseState::Normal:
        default:
            return colorRef(darkMode ? Dark::kChromeMuted : Light::kChromeMuted);
    }
}

///
/// \brief QlementineAppStyle::buttonForegroundColor
/// \param mouse
/// \param role
/// \param widget
/// \return
///
QColor const& QlementineAppStyle::buttonForegroundColor(MouseState mouse, ColorRole role,
                                                 const QWidget* widget) const
{
    Q_UNUSED(widget)

    if (role == ColorRole::Primary)
        return QlementineStyle::buttonForegroundColor(mouse, role, widget);

    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kDisabledText : Light::kDisabledText)
        : colorRef(isDarkMode() ? Dark::kText : Light::kText);
}

///
/// \brief QlementineAppStyle::drawControl
/// \param element
/// \param option
/// \param painter
/// \param widget
///
void QlementineAppStyle::drawControl(ControlElement element, const QStyleOption* option,
                              QPainter* painter, const QWidget* widget) const
{
    if (element == CE_DockWidgetTitle) {
        const auto* dockOption = qstyleoption_cast<const QStyleOptionDockWidget*>(option);
        if (!dockOption) {
            QlementineStyle::drawControl(element, option, painter, widget);
            return;
        }

        const QRect rect = dockOption->rect;
        const bool darkMode = isDarkMode();
        painter->save();
        painter->fillRect(rect, colorRef(darkMode ? Dark::kChrome : Light::kChrome));
        painter->setPen(QPen(colorRef(darkMode ? Dark::kBorder : Light::kBorder), 1));
        painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

        QRect textRect = rect.adjusted(6, 0, -42, 0);
        if (textRect.isValid()) {
            QFont font = painter->font();
            font.setPointSize(qMax(11, font.pointSize()));
            painter->setFont(font);
            painter->setPen(colorRef(darkMode ? Dark::kText : Light::kText));
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                              painter->fontMetrics().elidedText(dockOption->title, Qt::ElideRight, textRect.width()));
        }
        painter->restore();
        return;
    }

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

    QRect r = tabBar->tabRect(tabBar->currentIndex());
    if (!r.isValid())
        r = tabOption->rect;

    r = r.marginsRemoved(tabShapePadding(*tabOption, theme().spacing));
    r = r.adjusted(1, 0, -1, -1);
    const QColor color = widget->palette().highlight().color();
    constexpr int thickness = 2;
    const qreal radius = theme().borderRadius;

    painter->save();
    QPainterPath clipPath;
    clipPath.addRoundedRect(QRectF(r), radius, radius);
    painter->setClipPath(clipPath);

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

    painter->restore();
}

///
/// \brief QlementineAppStyle::iconForegroundColor
/// \param mouse
/// \param role
/// \return
///
QColor const& QlementineAppStyle::iconForegroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineStyle::iconForegroundColor(mouse, role);

    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Hovered:
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kBluePressed : Light::kBluePressed);
        case MouseState::Disabled:
            return colorRef(darkMode ? Dark::kDisabledText : Light::kDisabledText);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(darkMode ? Dark::kTextMuted : Light::kTextMuted);
    }
}

///
/// \brief QlementineAppStyle::listItemBackgroundColor
/// \param mouse
/// \param selected
/// \param focus
/// \param active
/// \param index
/// \param widget
/// \return
///
QColor QlementineAppStyle::listItemBackgroundColor(MouseState mouse, SelectionState selected, FocusState focus,
                                            ActiveState active, const QModelIndex& index,
                                            const QWidget* widget) const
{
    Q_UNUSED(focus)
    Q_UNUSED(active)
    Q_UNUSED(widget)

    const bool darkMode = isDarkMode();
    const bool isSelected = selected == SelectionState::Selected;
    const QColor rowColor = modelBackgroundColor(index);
    if (isSelected)
        return mouse == MouseState::Disabled
            ? QColor(darkMode ? Dark::kSelectionDisabled : Light::kSelectionDisabled)
            : QColor(darkMode ? Dark::kSelection : Light::kSelection);

    switch (mouse) {
        case MouseState::Hovered:
            return QColor(darkMode ? Dark::kItemHover : Light::kItemHover);
        case MouseState::Pressed:
            return QColor(darkMode ? Dark::kItemPressed : Light::kItemPressed);
        case MouseState::Disabled:
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            if (rowColor.isValid())
                return rowColor;
            return transparent(darkMode ? Dark::kCanvas : Light::kCanvas);
    }
}

///
/// \brief QlementineAppStyle::listItemForegroundColor
/// \param mouse
/// \param selected
/// \param focus
/// \param active
/// \return
///
QColor const& QlementineAppStyle::listItemForegroundColor(MouseState mouse, SelectionState selected,
                                                   FocusState focus, ActiveState active) const
{
    Q_UNUSED(selected)
    Q_UNUSED(focus)
    Q_UNUSED(active)

    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kDisabledText : Light::kDisabledText)
        : colorRef(isDarkMode() ? Dark::kText : Light::kText);
}

///
/// \brief QlementineAppStyle::pixelMetric
/// \param metric
/// \param option
/// \param widget
/// \return
///
int QlementineAppStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
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

    switch (metric) {
        case PM_DockWidgetFrameWidth:
        case PM_DockWidgetSeparatorExtent:
            return 1;
        case PM_DockWidgetHandleExtent:
            return 22;
        case PM_DockWidgetTitleMargin:
        case PM_DockWidgetTitleBarButtonMargin:
            return 4;
        case PM_SplitterWidth:
            return 1;
        case PM_ToolBarIconSize:
        case PM_SmallIconSize:
            return 16;
        default:
            break;
    }

    return QlementineStyle::pixelMetric(metric, option, widget);
}

///
/// \brief QlementineAppStyle::polish
/// \param widget
///
void QlementineAppStyle::polish(QWidget* widget)
{
    QlementineStyle::polish(widget);

    if (auto* label = qobject_cast<QLabel*>(widget)) {
        if (label->inherits("QTipLabel"))
            label->setForegroundRole(QPalette::ToolTipText);
        else
            label->setForegroundRole(QPalette::WindowText);
    }

    if (qobject_cast<QPushButton*>(widget) || qobject_cast<QToolButton*>(widget)) {
        auto font = widget->font();
        font.setBold(false);
        widget->setFont(font);
    }
}

///
/// \brief QlementineAppStyle::sizeFromContents
/// \param type
/// \param option
/// \param contentsSize
/// \param widget
/// \return
///
QSize QlementineAppStyle::sizeFromContents(ContentsType type, const QStyleOption* option,
                                    const QSize& contentsSize, const QWidget* widget) const
{
    QSize size = QlementineStyle::sizeFromContents(type, option, contentsSize, widget);

    if (type == CT_LineEdit && qobject_cast<const QLineEdit*>(widget))
        size.setWidth(qMax(size.width(), contentsSize.width()));

    if (type == CT_ToolButton && widget && widget->parentWidget()
        && widget->parentWidget()->objectName() == QStringLiteral("toolBarMain")) {
        size.rheight() = qMax(size.height(), 36);
        size.rwidth() += 4;
    }

    return size;
}

///
/// \brief QlementineAppStyle::splitterColor
/// \param mouse
/// \return
///
QColor const& QlementineAppStyle::splitterColor(MouseState mouse) const
{
    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Hovered:
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kBorderActive : Light::kBorderActive);
        case MouseState::Disabled:
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(darkMode ? Dark::kBorder : Light::kBorder);
    }
}

///
/// \brief QlementineAppStyle::styleHint
/// \param hint
/// \param option
/// \param widget
/// \param returnData
/// \return
///
int QlementineAppStyle::styleHint(StyleHint hint, const QStyleOption* option,
                           const QWidget* widget, QStyleHintReturn* returnData) const
{
    if (hint == SH_ToolButtonStyle) {
        const auto* toolButton = qobject_cast<const QToolButton*>(widget);
        const auto* parent = toolButton ? toolButton->parentWidget() : nullptr;
        if (parent && parent->objectName() == QStringLiteral("toolBarMain"))
            return toolButton->toolButtonStyle();
    }

    return QlementineStyle::styleHint(hint, option, widget, returnData);
}

///
/// \brief QlementineAppStyle::tabBackgroundColor
/// \param mouse
/// \param selected
/// \return
///
QColor const& QlementineAppStyle::tabBackgroundColor(MouseState mouse, SelectionState selected) const
{
    const bool darkMode = isDarkMode();
    const bool isSelected = selected == SelectionState::Selected;

    switch (mouse) {
        case MouseState::Pressed:
            return isSelected
                ? colorRef(darkMode ? Dark::kCanvas : Light::kCanvas)
                : colorRef(darkMode ? Dark::kChromePressed : Light::kChromePressed);
        case MouseState::Hovered:
            return isSelected
                ? colorRef(darkMode ? Dark::kCanvas : Light::kCanvas)
                : colorRef(darkMode ? Dark::kChromeStrong : Light::kChromeStrong);
        case MouseState::Disabled:
            return transparentRef(darkMode ? Dark::kChrome : Light::kChrome);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return isSelected
                ? colorRef(darkMode ? Dark::kCanvas : Light::kCanvas)
                : transparentRef(darkMode ? Dark::kChrome : Light::kChrome);
    }
}

///
/// \brief QlementineAppStyle::tabBarBackgroundColor
/// \param mouse
/// \return
///
QColor const& QlementineAppStyle::tabBarBackgroundColor(MouseState mouse) const
{
    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kChromeStrong : Light::kChromeStrong)
        : colorRef(isDarkMode() ? Dark::kChrome : Light::kChrome);
}

///
/// \brief QlementineAppStyle::tabForegroundColor
/// \param mouse
/// \param selected
/// \return
///
QColor const& QlementineAppStyle::tabForegroundColor(MouseState mouse, SelectionState selected) const
{
    Q_UNUSED(selected)

    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kDisabledText : Light::kDisabledText)
        : colorRef(isDarkMode() ? Dark::kText : Light::kText);
}

///
/// \brief QlementineAppStyle::tableHeaderBgColor
/// \param mouse
/// \param checked
/// \return
///
QColor const& QlementineAppStyle::tableHeaderBgColor(MouseState mouse, CheckState checked) const
{
    Q_UNUSED(checked)

    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kChromePressed : Light::kChromePressed);
        case MouseState::Hovered:
            return colorRef(darkMode ? Dark::kChromeStrong : Light::kChromeStrong);
        case MouseState::Disabled:
            return colorRef(darkMode ? Dark::kSurfaceDisabled : Light::kSurfaceDisabled);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(darkMode ? Dark::kCanvas : Light::kCanvas);
    }
}

///
/// \brief QlementineAppStyle::tableHeaderFgColor
/// \param mouse
/// \param checked
/// \return
///
QColor const& QlementineAppStyle::tableHeaderFgColor(MouseState mouse, CheckState checked) const
{
    Q_UNUSED(checked)

    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kDisabledText : Light::kDisabledText)
        : colorRef(isDarkMode() ? Dark::kText : Light::kText);
}

///
/// \brief QlementineAppStyle::tableLineColor
/// \return
///
QColor const& QlementineAppStyle::tableLineColor() const
{
    return colorRef(isDarkMode() ? Dark::kBorderMuted : Light::kBorderMuted);
}

///
/// \brief QlementineAppStyle::textFieldBackgroundColor
/// \param mouse
/// \param status
/// \return
///
QColor const& QlementineAppStyle::textFieldBackgroundColor(MouseState mouse, Status status) const
{
    Q_UNUSED(status)

    return mouse == MouseState::Disabled
        ? colorRef(isDarkMode() ? Dark::kSurfaceDisabled : Light::kSurfaceDisabled)
        : colorRef(isDarkMode() ? Dark::kCanvas : Light::kCanvas);
}

///
/// \brief QlementineAppStyle::textFieldBorderColor
/// \param mouse
/// \param focus
/// \param status
/// \return
///
QColor const& QlementineAppStyle::textFieldBorderColor(MouseState mouse, FocusState focus, Status status) const
{
    if (status != Status::Default)
        return QlementineStyle::textFieldBorderColor(mouse, focus, status);

    const bool darkMode = isDarkMode();

    if (mouse == MouseState::Disabled)
        return colorRef(darkMode ? Dark::kBorderDisabled : Light::kBorderDisabled);
    if (focus == FocusState::Focused)
        return colorRef(darkMode ? Dark::kBlue : Light::kBlue);
    if (mouse == MouseState::Hovered || mouse == MouseState::Pressed)
        return colorRef(darkMode ? Dark::kBorderActive : Light::kBorderActive);

    return colorRef(darkMode ? Dark::kBorder : Light::kBorder);
}

///
/// \brief QlementineAppStyle::toolBarBackgroundColor
/// \return
///
QColor const& QlementineAppStyle::toolBarBackgroundColor() const
{
    return colorRef(isDarkMode() ? Dark::kChrome : Light::kChrome);
}

///
/// \brief QlementineAppStyle::toolBarBorderColor
/// \return
///
QColor const& QlementineAppStyle::toolBarBorderColor() const
{
    return colorRef(isDarkMode() ? Dark::kBorder : Light::kBorder);
}

///
/// \brief QlementineAppStyle::toolTipForegroundColor
/// \return
///
QColor const& QlementineAppStyle::toolTipForegroundColor() const
{
    return colorRef(isDarkMode() ? Dark::kTooltipText : Light::kTooltipText);
}

///
/// \brief QlementineAppStyle::toolButtonBackgroundColor
/// \param mouse
/// \param role
/// \return
///
QColor const& QlementineAppStyle::toolButtonBackgroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineStyle::toolButtonBackgroundColor(mouse, role);

    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kChromePressed : Light::kChromePressed);
        case MouseState::Hovered:
            return colorRef(darkMode ? Dark::kChromeStrong : Light::kChromeStrong);
        case MouseState::Disabled:
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return transparentRef(darkMode ? Dark::kChrome : Light::kChrome);
    }
}

///
/// \brief QlementineAppStyle::toolButtonForegroundColor
/// \param mouse
/// \param role
/// \return
///
QColor const& QlementineAppStyle::toolButtonForegroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineStyle::toolButtonForegroundColor(mouse, role);

    const bool darkMode = isDarkMode();

    switch (mouse) {
        case MouseState::Hovered:
        case MouseState::Pressed:
            return colorRef(darkMode ? Dark::kTextEmphasis : Light::kTextEmphasis);
        case MouseState::Disabled:
            return colorRef(darkMode ? Dark::kDisabledText : Light::kDisabledText);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(darkMode ? Dark::kText : Light::kText);
    }
}

#endif // HAVE_QLEMENTINE_APP_STYLE
