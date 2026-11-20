// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file elidinglineedit.h
/// \brief Declares a line edit that abbreviates long values outside editing.
///

#ifndef ELIDINGLINEEDIT_H
#define ELIDINGLINEEDIT_H

#include <QLineEdit>

class ElidingLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    ///
    /// \brief Creates an editor with an abbreviated preview when unfocused.
    /// \param parent Parent widget.
    ///
    explicit ElidingLineEdit(QWidget* parent = nullptr);

    ///
    /// \brief Returns the preview without changing the editable value or undo history.
    ///
    QString visibleText() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QRect textContentsRect() const;
};

#endif
