// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file settingstabwidget.cpp
/// \brief Paints the selected settings tab with the application accent.
///

#include "settingstabwidget.h"
#include <QStyleOptionTab>
#include <QStylePainter>
#include <QTabBar>

namespace {
class SettingsTabBar : public QTabBar
{
protected:
    ///
    /// \brief Preserves native inactive tabs while highlighting the current settings page.
    ///
    void paintEvent(QPaintEvent*) override
    {
        QStylePainter painter(this);
        for (int index = 0; index < count(); ++index) {
            QStyleOptionTab option;
            initStyleOption(&option, index);
            if (!(option.state & QStyle::State_Selected)) {
                painter.save();
                painter.drawControl(QStyle::CE_TabBarTab, option);
                painter.restore();
                continue;
            }
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(palette().color(QPalette::Highlight).lighter(125));
            painter.setBrush(palette().highlight());
            painter.drawRoundedRect(QRectF(option.rect).adjusted(1, 1, -1, -1), 7, 7);
            const QColor textColor = palette().color(QPalette::HighlightedText);
            QRect textRect = option.rect;
            if (!option.icon.isNull()) {
                const int gap = 8;
                const int width = option.iconSize.width() + gap + fontMetrics().horizontalAdvance(option.text);
                QRect iconRect(option.rect.center().x() - width / 2,
                               option.rect.center().y() - option.iconSize.height() / 2,
                               option.iconSize.width(), option.iconSize.height());
                QPixmap icon = option.icon.pixmap(option.iconSize);
                QPainter iconPainter(&icon);
                iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                iconPainter.fillRect(icon.rect(), textColor);
                iconPainter.end();
                painter.drawPixmap(QStyle::visualRect(layoutDirection(), option.rect, iconRect), icon);
                textRect.setLeft(iconRect.right() + gap);
                textRect.setRight(textRect.left() + fontMetrics().horizontalAdvance(option.text));
                textRect = QStyle::visualRect(layoutDirection(), option.rect, textRect);
            }
            painter.setPen(textColor);
            painter.drawText(textRect, Qt::AlignCenter | Qt::TextShowMnemonic, option.text);
            if (option.state & QStyle::State_HasFocus) {
                QStyleOptionFocusRect focus;
                focus.QStyleOption::operator=(option);
                focus.rect = option.rect.adjusted(4, 4, -4, -4);
                painter.drawPrimitive(QStyle::PE_FrameFocusRect, focus);
            }
        }
    }
};
}

///
/// \brief Creates settings navigation with a palette-aware selected tab.
/// \param parent Parent widget.
///
SettingsTabWidget::SettingsTabWidget(QWidget* parent) : QTabWidget(parent)
{
    setTabBar(new SettingsTabBar());
}

#ifdef Q_OS_MAC
///
/// \brief Separates the macOS settings pane from the surrounding dialog using the active palette.
///
void SettingsTabWidget::paintEvent(QPaintEvent* event)
{
    QTabWidget::paintEvent(event);
    QStyleOptionTabWidgetFrame option;
    initStyleOption(&option);
    const QRect pane = style()->subElementRect(QStyle::SE_TabWidgetTabPane, &option, this);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(palette().base());
    QColor border = palette().color(QPalette::Mid);
    border.setAlpha(64);
    painter.setPen(border);
    painter.drawRoundedRect(QRectF(pane).adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
}
#endif
