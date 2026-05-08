#ifdef Q_OS_MAC

#include "macsplittoolbutton.h"

#include <QImage>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOptionTabBarBase>
#include <QTabBar>

static QColor tabBarBaseColor(const QTabBar* tabBar, const QSize& sampleSize)
{
    if (!tabBar || !sampleSize.isValid())
        return {};

    QImage image(sampleSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QStyleOptionTabBarBase option;
    option.initFrom(tabBar);
    option.shape = tabBar->shape();
    option.documentMode = tabBar->documentMode();
    option.tabBarRect = QRect(QPoint(0, 0), sampleSize);

    QPainter painter(&image);
    tabBar->style()->drawPrimitive(QStyle::PE_FrameTabBarBase, &option, &painter, tabBar);
    painter.end();

    const QColor sampled = image.pixelColor(qBound(0, sampleSize.width() / 2, sampleSize.width() - 1),
                                            qBound(0, sampleSize.height() / 2, sampleSize.height() - 1));
    if (sampled.alpha() > 0)
        return sampled;

    return tabBar->palette().window().color();
}

///
/// \brief MacSplitToolButton::MacSplitToolButton
/// \param parent
///
MacSplitToolButton::MacSplitToolButton(QWidget* parent)
    : QToolButton(parent)
{
    setAutoRaise(false);
    setFocusPolicy(Qt::NoFocus);
    setIconSize(QSize(16, 16));
    setAttribute(Qt::WA_Hover, true);
}

///
/// \brief MacSplitToolButton::setReferenceTabBar
/// \param tabBar
///
void MacSplitToolButton::setReferenceTabBar(QTabBar* tabBar)
{
    _tabBar = tabBar;
    update();
}

///
/// \brief MacSplitToolButton::sizeHint
/// \return
///
QSize MacSplitToolButton::sizeHint() const
{
    return QSize(24, 24);
}

///
/// \brief MacSplitToolButton::paintEvent
/// \param event
///
void MacSplitToolButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QColor base = _tabBar ? tabBarBaseColor(_tabBar, rect().size())
                                : palette().window().color();
    painter.fillRect(rect(), base);

    if (_tabBar) {
        QStyleOptionTabBarBase option;
        option.initFrom(_tabBar);
        option.shape = _tabBar->shape();
        option.documentMode = _tabBar->documentMode();
        option.tabBarRect = rect();
        _tabBar->style()->drawPrimitive(QStyle::PE_FrameTabBarBase, &option, &painter, _tabBar);
    }

    const bool pressed = isDown();
    const bool active = isChecked();
    const bool hovered = underMouse();

    QColor fill = base;
    QColor border = palette().mid().color();
    bool drawChrome = false;

    if (active) {
        fill = mix(base, palette().highlight().color(), 0.20);
        border = palette().highlight().color();
        drawChrome = true;
    } else if (pressed) {
        fill = base.darker(108);
        drawChrome = true;
    } else if (hovered) {
        fill = base.lighter(106);
        drawChrome = true;
    }

    if (drawChrome) {
        const QRectF buttonRect = chromeRect().adjusted(1, 1, -1, -1);
        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawRoundedRect(buttonRect, 3, 3);
    }

    const QSize glyphSize = iconSize();
    const QRect innerRect = chromeRect();
    const QRect iconRect(QPoint(innerRect.x() + (innerRect.width() - glyphSize.width()) / 2,
                                innerRect.y() + (innerRect.height() - glyphSize.height()) / 2),
                         glyphSize);
    icon().paint(&painter,
                 iconRect,
                 Qt::AlignCenter,
                 isEnabled() ? QIcon::Normal : QIcon::Disabled,
                 active ? QIcon::On : QIcon::Off);
}

///
/// \brief MacSplitToolButton::hitButton
/// \param pos
/// \return
///
bool MacSplitToolButton::hitButton(const QPoint& pos) const
{
    return chromeRect().contains(pos);
}

///
/// \brief MacSplitToolButton::chromeRect
/// \return
///
QRect MacSplitToolButton::chromeRect() const
{
    const QSize chromeSize = sizeHint();
    const int x = qMax(0, (width() - chromeSize.width()) / 2);
    const int y = qMax(0, (height() - chromeSize.height()) / 2);
    return QRect(x, y,
                 qMin(chromeSize.width(), width()),
                 qMin(chromeSize.height(), height()));
}

///
/// \brief MacSplitToolButton::mix
/// \param a
/// \param b
/// \param amount
/// \return
///
QColor MacSplitToolButton::mix(const QColor& a, const QColor& b, qreal amount)
{
    amount = qBound<qreal>(0.0, amount, 1.0);
    const qreal inv = 1.0 - amount;
    return QColor::fromRgbF(a.redF()   * inv + b.redF()   * amount,
                            a.greenF() * inv + b.greenF() * amount,
                            a.blueF()  * inv + b.blueF()  * amount,
                            1.0);
}

#endif // Q_OS_MAC
