// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file elidinglineedit.cpp
/// \brief Draws abbreviated previews while preserving normal line editing.
///

#include "elidinglineedit.h"
#include <QStyleOptionFrame>
#include <QStylePainter>

///
/// \brief Creates an editor with an abbreviated preview when unfocused.
/// \param parent Parent widget.
///
ElidingLineEdit::ElidingLineEdit(QWidget* parent) : QLineEdit(parent)
{
}

///
/// \brief Returns the preview without changing the editable value or undo history.
///
QString ElidingLineEdit::visibleText() const
{
    return hasFocus() ? displayText()
                      : fontMetrics().elidedText(displayText(), Qt::ElideRight, textContentsRect().width());
}

///
/// \brief Respects the active style's text padding and explicit line edit margins.
///
QRect ElidingLineEdit::textContentsRect() const
{
    QStyleOptionFrame option;
    initStyleOption(&option);
    return style()->subElementRect(QStyle::SE_LineEditContents, &option, this)
        .marginsRemoved(textMargins()).adjusted(2, 0, -2, 0);
}

///
/// \brief Uses native editing while focused and paints an abbreviated preview otherwise.
///
void ElidingLineEdit::paintEvent(QPaintEvent* event)
{
    if (hasFocus() || echoMode() != QLineEdit::Normal || visibleText() == displayText()) {
        QLineEdit::paintEvent(event);
        return;
    }
    QStyleOptionFrame option;
    initStyleOption(&option);
    QStylePainter painter(this);
    painter.drawPrimitive(QStyle::PE_PanelLineEdit, option);
    const QRect content = textContentsRect();
    painter.setClipRect(content);
    style()->drawItemText(&painter, content,
                          QStyle::visualAlignment(layoutDirection(), alignment()) | Qt::AlignVCenter,
                          palette(), isEnabled(), visibleText(), QPalette::Text);
}
