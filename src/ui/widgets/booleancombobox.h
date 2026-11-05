// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file booleancombobox.h
/// \brief Declares the booleancombobox interfaces.
///

#ifndef BOOLEANCOMBOBOX_H
#define BOOLEANCOMBOBOX_H

#include <QComboBox>

class BooleanComboBox: public QComboBox
{
    Q_OBJECT
public:
    BooleanComboBox(QWidget* parent = nullptr);

    bool currentValue() const;
    void setCurrentValue(bool value);
};

#endif // BOOLEANCOMBOBOX_H

