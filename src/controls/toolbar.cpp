#include "toolbar.h"

#include <QActionEvent>
#include <QApplication>
#include <QChildEvent>
#include <QShowEvent>
#include <QWidget>

///
/// \brief ToolBar::ToolBar
/// \param parent
///
ToolBar::ToolBar(QWidget* parent)
    : QToolBar(parent)
{
    applyPlatformFont();
}

///
/// \brief ToolBar::actionEvent
/// \param event
///
void ToolBar::actionEvent(QActionEvent* event)
{
    QToolBar::actionEvent(event);
    applyPlatformFont();
}

///
/// \brief ToolBar::childEvent
/// \param event
///
void ToolBar::childEvent(QChildEvent* event)
{
    QToolBar::childEvent(event);
    if (event->added())
        applyPlatformFont();
}

///
/// \brief ToolBar::showEvent
/// \param event
///
void ToolBar::showEvent(QShowEvent* event)
{
    QToolBar::showEvent(event);
    applyPlatformFont();
}

///
/// \brief ToolBar::applyPlatformFont
///
void ToolBar::applyPlatformFont()
{
#ifdef Q_OS_MAC
    QFont toolbarFont = QApplication::font();
    toolbarFont.setPointSizeF(qMax<qreal>(toolbarFont.pointSizeF(), 12.0));
    setFont(toolbarFont);

    const auto childWidgets = findChildren<QWidget*>();
    for (auto* widget : childWidgets) {
        widget->setFont(toolbarFont);
        widget->setAttribute(Qt::WA_MacSmallSize, false);
        widget->setAttribute(Qt::WA_MacMiniSize, false);
        widget->setAttribute(Qt::WA_MacNormalSize, true);
    }

#endif
}
