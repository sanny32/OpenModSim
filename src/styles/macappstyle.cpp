#ifdef Q_OS_MAC

#include "macappstyle.h"

#include <QComboBox>
#include <QDockWidget>
#include <QFontDatabase>
#include <QHash>
#include <QLineEdit>
#include <QLabel>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QStyleOptionDockWidget>
#include <QStyleOptionTab>
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

constexpr QRgb kCanvas = 0xffffff;
constexpr QRgb kChrome = 0xf8f9fb;
constexpr QRgb kChromeStrong = 0xf1f3f6;
constexpr QRgb kChromePressed = 0xe8edf5;
constexpr QRgb kBorder = 0xdfe3e8;
constexpr QRgb kBorderActive = 0xcfd6df;
constexpr QRgb kText = 0x1f2937;
constexpr QRgb kMutedText = 0x626b79;
constexpr QRgb kDisabledText = 0xb2bac5;
constexpr QRgb kBlue = 0x3a8dde;
constexpr QRgb kBlueHover = 0x4a9bea;
constexpr QRgb kBluePressed = 0x2f7fcc;
constexpr QRgb kBlueDisabled = 0xb8d8f6;
constexpr QRgb kGreen = 0x34c759;

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

Theme makeMacAppTheme()
{
    Theme theme;
    theme.meta.name = QStringLiteral("Open ModSim macOS");
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
    theme.neutralColorPressed = QColor(0xdce4ef);
    theme.neutralColorDisabled = QColor(0xf4f6f8);
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
    theme.secondaryColorHovered = QColor(0x111827);
    theme.secondaryColorPressed = QColor(0x0f172a);
    theme.secondaryColorDisabled = QColor(kDisabledText);
    theme.secondaryColorTransparent = transparent(kText);

    theme.secondaryColorForeground = QColor(0x39506b);
    theme.secondaryColorForegroundHovered = QColor(kBluePressed);
    theme.secondaryColorForegroundPressed = QColor(0x1f6db8);
    theme.secondaryColorForegroundDisabled = QColor(kDisabledText);
    theme.secondaryColorForegroundTransparent = transparent(0x39506b);

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
    theme.borderColorPressed = QColor(0xbcc6d2);
    theme.borderColorDisabled = QColor(0xebeef2);
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
    theme.fontSize = 12;
    theme.fontSizeMonospace = 12;
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
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::AlternateBase, QColor(0xf6f8fb));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Button, theme.neutralColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Text, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::WindowText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ButtonText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Highlight, theme.primaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::HighlightedText, theme.primaryColorForeground);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Text, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::WindowText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ButtonText, theme.secondaryColorDisabled);

    return theme;
}

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
/// \brief MacAppStyle::MacAppStyle
/// \param parent
///
MacAppStyle::MacAppStyle(QObject* parent)
    : QlementineStyle(parent)
{
    setTheme(makeMacAppTheme());
    setAutoIconColor(oclero::qlementine::AutoIconColor::ForegroundColor);
}

///
/// \brief MacAppStyle::buttonBackgroundColor
/// \param mouse
/// \param role
/// \param widget
/// \return
///
QColor const& MacAppStyle::buttonBackgroundColor(MouseState mouse, ColorRole role,
                                                 const QWidget* widget) const
{
    Q_UNUSED(widget)

    if (role == ColorRole::Primary)
        return QlementineStyle::buttonBackgroundColor(mouse, role, widget);

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
            return colorRef(0xfdfdfd);
    }
}

///
/// \brief MacAppStyle::buttonForegroundColor
/// \param mouse
/// \param role
/// \param widget
/// \return
///
QColor const& MacAppStyle::buttonForegroundColor(MouseState mouse, ColorRole role,
                                                 const QWidget* widget) const
{
    Q_UNUSED(widget)

    if (role == ColorRole::Primary)
        return QlementineStyle::buttonForegroundColor(mouse, role, widget);

    return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
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
    if (element == CE_DockWidgetTitle) {
        const auto* dockOption = qstyleoption_cast<const QStyleOptionDockWidget*>(option);
        if (!dockOption) {
            QlementineStyle::drawControl(element, option, painter, widget);
            return;
        }

        const QRect rect = dockOption->rect;
        painter->save();
        painter->fillRect(rect, colorRef(kChrome));
        painter->setPen(QPen(colorRef(kBorder), 1));
        painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

        QRect textRect = rect.adjusted(6, 0, -42, 0);
        if (textRect.isValid()) {
            QFont font = painter->font();
            font.setPointSize(qMax(11, font.pointSize()));
            painter->setFont(font);
            painter->setPen(colorRef(kText));
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
/// \brief MacAppStyle::iconForegroundColor
/// \param mouse
/// \param role
/// \return
///
QColor const& MacAppStyle::iconForegroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineStyle::iconForegroundColor(mouse, role);

    switch (mouse) {
        case MouseState::Hovered:
        case MouseState::Pressed:
            return colorRef(kBluePressed);
        case MouseState::Disabled:
            return colorRef(kDisabledText);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(0x4f627a);
    }
}

///
/// \brief MacAppStyle::listItemBackgroundColor
/// \param mouse
/// \param selected
/// \param focus
/// \param active
/// \param index
/// \param widget
/// \return
///
QColor MacAppStyle::listItemBackgroundColor(MouseState mouse, SelectionState selected, FocusState focus,
                                            ActiveState active, const QModelIndex& index,
                                            const QWidget* widget) const
{
    Q_UNUSED(focus)
    Q_UNUSED(active)
    Q_UNUSED(index)
    Q_UNUSED(widget)

    const bool isSelected = selected == SelectionState::Selected;
    if (isSelected)
        return mouse == MouseState::Disabled ? QColor(0xf0f3f7) : QColor(0xeaf2ff);

    switch (mouse) {
        case MouseState::Hovered:
            return QColor(0xf3f7fc);
        case MouseState::Pressed:
            return QColor(0xe8eef7);
        case MouseState::Disabled:
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return transparent(kCanvas);
    }
}

///
/// \brief MacAppStyle::listItemForegroundColor
/// \param mouse
/// \param selected
/// \param focus
/// \param active
/// \return
///
QColor const& MacAppStyle::listItemForegroundColor(MouseState mouse, SelectionState selected,
                                                   FocusState focus, ActiveState active) const
{
    Q_UNUSED(selected)
    Q_UNUSED(focus)
    Q_UNUSED(active)

    return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
}

///
/// \brief MacAppStyle::pixelMetric
/// \param metric
/// \param option
/// \param widget
/// \return
///
int MacAppStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
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
/// \brief MacAppStyle::polish
/// \param widget
///
void MacAppStyle::polish(QWidget* widget)
{
    QlementineStyle::polish(widget);

    if (auto* label = qobject_cast<QLabel*>(widget))
        label->setForegroundRole(QPalette::WindowText);

    if (auto* toolBar = qobject_cast<QToolBar*>(widget)) {
        toolBar->setIconSize(QSize(16, 16));
        toolBar->setContentsMargins(8, 0, 8, 0);
        if (toolBar->objectName() == QStringLiteral("toolBarMain"))
            toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
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

    if (type == CT_ToolButton && widget && widget->parentWidget()
        && widget->parentWidget()->objectName() == QStringLiteral("toolBarMain")) {
        size.rheight() = qMax(size.height(), 36);
        size.rwidth() += 4;
    }

    return size;
}

///
/// \brief MacAppStyle::splitterColor
/// \param mouse
/// \return
///
QColor const& MacAppStyle::splitterColor(MouseState mouse) const
{
    switch (mouse) {
        case MouseState::Hovered:
        case MouseState::Pressed:
            return colorRef(kBorderActive);
        case MouseState::Disabled:
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(kBorder);
    }
}

///
/// \brief MacAppStyle::styleHint
/// \param hint
/// \param option
/// \param widget
/// \param returnData
/// \return
///
int MacAppStyle::styleHint(StyleHint hint, const QStyleOption* option,
                           const QWidget* widget, QStyleHintReturn* returnData) const
{
    if (hint == SH_ToolButtonStyle) {
        const auto* toolButton = qobject_cast<const QToolButton*>(widget);
        const auto* parent = toolButton ? toolButton->parentWidget() : nullptr;
        if (parent && parent->objectName() == QStringLiteral("toolBarMain"))
            return Qt::ToolButtonTextBesideIcon;
    }

    return QlementineStyle::styleHint(hint, option, widget, returnData);
}

///
/// \brief MacAppStyle::tabBackgroundColor
/// \param mouse
/// \param selected
/// \return
///
QColor const& MacAppStyle::tabBackgroundColor(MouseState mouse, SelectionState selected) const
{
    const bool isSelected = selected == SelectionState::Selected;

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

///
/// \brief MacAppStyle::tabBarBackgroundColor
/// \param mouse
/// \return
///
QColor const& MacAppStyle::tabBarBackgroundColor(MouseState mouse) const
{
    return mouse == MouseState::Disabled ? colorRef(kChromeStrong) : colorRef(kChrome);
}

///
/// \brief MacAppStyle::tabForegroundColor
/// \param mouse
/// \param selected
/// \return
///
QColor const& MacAppStyle::tabForegroundColor(MouseState mouse, SelectionState selected) const
{
    Q_UNUSED(selected)

    return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
}

///
/// \brief MacAppStyle::tableHeaderBgColor
/// \param mouse
/// \param checked
/// \return
///
QColor const& MacAppStyle::tableHeaderBgColor(MouseState mouse, CheckState checked) const
{
    Q_UNUSED(checked)

    switch (mouse) {
        case MouseState::Pressed:
            return colorRef(kChromePressed);
        case MouseState::Hovered:
            return colorRef(kChromeStrong);
        case MouseState::Disabled:
            return colorRef(0xf4f6f8);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(kCanvas);
    }
}

///
/// \brief MacAppStyle::tableHeaderFgColor
/// \param mouse
/// \param checked
/// \return
///
QColor const& MacAppStyle::tableHeaderFgColor(MouseState mouse, CheckState checked) const
{
    Q_UNUSED(checked)

    return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
}

///
/// \brief MacAppStyle::tableLineColor
/// \return
///
QColor const& MacAppStyle::tableLineColor() const
{
    return colorRef(0xe6e9ee);
}

///
/// \brief MacAppStyle::textFieldBackgroundColor
/// \param mouse
/// \param status
/// \return
///
QColor const& MacAppStyle::textFieldBackgroundColor(MouseState mouse, Status status) const
{
    Q_UNUSED(status)

    return mouse == MouseState::Disabled ? colorRef(0xf4f6f8) : colorRef(kCanvas);
}

///
/// \brief MacAppStyle::textFieldBorderColor
/// \param mouse
/// \param focus
/// \param status
/// \return
///
QColor const& MacAppStyle::textFieldBorderColor(MouseState mouse, FocusState focus, Status status) const
{
    if (status != Status::Default)
        return QlementineStyle::textFieldBorderColor(mouse, focus, status);

    if (mouse == MouseState::Disabled)
        return colorRef(0xebeef2);
    if (focus == FocusState::Focused)
        return colorRef(kBlue);
    if (mouse == MouseState::Hovered || mouse == MouseState::Pressed)
        return colorRef(kBorderActive);

    return colorRef(kBorder);
}

///
/// \brief MacAppStyle::toolBarBackgroundColor
/// \return
///
QColor const& MacAppStyle::toolBarBackgroundColor() const
{
    return colorRef(kChrome);
}

///
/// \brief MacAppStyle::toolBarBorderColor
/// \return
///
QColor const& MacAppStyle::toolBarBorderColor() const
{
    return colorRef(kBorder);
}

///
/// \brief MacAppStyle::toolButtonBackgroundColor
/// \param mouse
/// \param role
/// \return
///
QColor const& MacAppStyle::toolButtonBackgroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineStyle::toolButtonBackgroundColor(mouse, role);

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

///
/// \brief MacAppStyle::toolButtonForegroundColor
/// \param mouse
/// \param role
/// \return
///
QColor const& MacAppStyle::toolButtonForegroundColor(MouseState mouse, ColorRole role) const
{
    if (role == ColorRole::Primary)
        return QlementineStyle::toolButtonForegroundColor(mouse, role);

    switch (mouse) {
        case MouseState::Hovered:
        case MouseState::Pressed:
            return colorRef(0x24364d);
        case MouseState::Disabled:
            return colorRef(kDisabledText);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(kText);
    }
}

#endif // Q_OS_MAC
