// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file macsplittoolbutton.cpp
/// \brief Implements the macsplittoolbutton functionality.
///

#ifdef Q_OS_MAC

#include "macsplittoolbutton.h"

#include <QImage>
#include <QPalette>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOptionTabBarBase>
#include <QStyleOptionToolButton>
#include <QTabBar>

///
/// \brief tabBarBaseColor
/// \param tabBar
/// \param sampleSize
/// \return
///
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

    QStyleOptionToolButton option;
    initStyleOption(&option);
    option.rect = chromeRect();
    option.state.setFlag(QStyle::State_MouseOver, isEnabled() && underMouse());

    style()->drawPrimitive(QStyle::PE_PanelButtonTool, &option, &painter, this);
    style()->drawControl(QStyle::CE_ToolButtonLabel, &option, &painter, this);
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

#endif // Q_OS_MAC
