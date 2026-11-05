// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file htmldelegate.h
/// \brief Declares the htmldelegate interfaces.
///

#ifndef HTMLDELEGATE_H
#define HTMLDELEGATE_H

#include <QStyledItemDelegate>

///
/// \brief The HtmlDelegate class
///
class HtmlDelegate : public QStyledItemDelegate
{
public:
    ///
    /// \brief HtmlDelegate
    /// \param parent
    ///
    explicit HtmlDelegate(QObject* parent = nullptr);

protected:
    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

#endif // HTMLDELEGATE_H

