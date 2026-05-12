#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "qlementineappstyle.h"
#include <oclero/qlementine/icons/QlementineIcons.hpp>

#include <QComboBox>
#include <QDockWidget>
#include <QPushButton>
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

constexpr QRgb kCanvas = 0xffffff;
constexpr QRgb kChrome = 0xf5f5f5;
constexpr QRgb kChromeStrong = 0xececec;
constexpr QRgb kChromePressed = 0xe0e0e0;
constexpr QRgb kBorder = 0xe3e6ea;
constexpr QRgb kBorderActive = 0xcdd1d6;
constexpr QRgb kText = 0x1f2937;
constexpr QRgb kDisabledText = 0xb2bac5;
constexpr QRgb kBlue = 0x007aff;
constexpr QRgb kBlueHover = 0x1a8aff;
constexpr QRgb kBluePressed = 0x0062cc;
constexpr QRgb kBlueDisabled = 0xb3d7ff;

QColor transparent(QRgb rgb)
{
    QColor color(rgb);
    color.setAlpha(0);
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
}

QlementineAppStyle::QlementineAppStyle(bool /*skipTheme*/, QObject* parent)
    : QlementineStyle(parent)
{
    setAutoIconColor(oclero::qlementine::AutoIconColor::ForegroundColor);

    oclero::qlementine::icons::initializeIconTheme();
    QIcon::setThemeName(QStringLiteral("qlementine"));
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

    return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
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
/// \brief QlementineAppStyle::iconForegroundColor
/// \param mouse
/// \param role
/// \return
///
QColor const& QlementineAppStyle::iconForegroundColor(MouseState mouse, ColorRole role) const
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
    Q_UNUSED(index)
    Q_UNUSED(widget)

    const bool isSelected = selected == SelectionState::Selected;
    if (isSelected)
        return mouse == MouseState::Disabled ? QColor(0xeaeaea) : QColor(0xd9eaff);

    switch (mouse) {
        case MouseState::Hovered:
            return QColor(0xf0f0f0);
        case MouseState::Pressed:
            return QColor(0xe5e5e5);
        case MouseState::Disabled:
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return transparent(kCanvas);
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

    return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
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
/// \brief QlementineAppStyle::tabBarBackgroundColor
/// \param mouse
/// \return
///
QColor const& QlementineAppStyle::tabBarBackgroundColor(MouseState mouse) const
{
    return mouse == MouseState::Disabled ? colorRef(kChromeStrong) : colorRef(kChrome);
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

    return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
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

    switch (mouse) {
        case MouseState::Pressed:
            return colorRef(kChromePressed);
        case MouseState::Hovered:
            return colorRef(kChromeStrong);
        case MouseState::Disabled:
            return colorRef(0xf2f2f2);
        case MouseState::Transparent:
        case MouseState::Normal:
        default:
            return colorRef(kCanvas);
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

    return mouse == MouseState::Disabled ? colorRef(kDisabledText) : colorRef(kText);
}

///
/// \brief QlementineAppStyle::tableLineColor
/// \return
///
QColor const& QlementineAppStyle::tableLineColor() const
{
    return colorRef(0xe6e9ee);
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

    return mouse == MouseState::Disabled ? colorRef(0xf2f2f2) : colorRef(kCanvas);
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

    if (mouse == MouseState::Disabled)
        return colorRef(0xebeef2);
    if (focus == FocusState::Focused)
        return colorRef(kBlue);
    if (mouse == MouseState::Hovered || mouse == MouseState::Pressed)
        return colorRef(kBorderActive);

    return colorRef(kBorder);
}

///
/// \brief QlementineAppStyle::toolBarBackgroundColor
/// \return
///
QColor const& QlementineAppStyle::toolBarBackgroundColor() const
{
    return colorRef(kChrome);
}

///
/// \brief QlementineAppStyle::toolBarBorderColor
/// \return
///
QColor const& QlementineAppStyle::toolBarBorderColor() const
{
    return colorRef(kBorder);
}

///
/// \brief QlementineAppStyle::toolTipForegroundColor
/// \return
///
QColor const& QlementineAppStyle::toolTipForegroundColor() const
{
    return colorRef(kCanvas);
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
/// \brief QlementineAppStyle::toolButtonForegroundColor
/// \param mouse
/// \param role
/// \return
///
QColor const& QlementineAppStyle::toolButtonForegroundColor(MouseState mouse, ColorRole role) const
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

#endif // HAVE_QLEMENTINE_APP_STYLE
