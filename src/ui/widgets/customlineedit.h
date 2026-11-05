// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file customlineedit.h
/// \brief Declares the customlineedit interfaces.
///

#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include <QLineEdit>

///
/// \brief The CustomLineEdit class
///
class CustomLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    CustomLineEdit(QWidget *parent = nullptr);
    void setText(const QString& text);
};

#endif // CUSTOMLINEEDIT_H

