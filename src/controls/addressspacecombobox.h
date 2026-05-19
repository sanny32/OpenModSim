// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacecombobox.h
/// \brief Declares the addressspacecombobox interfaces.
///

#ifndef ADDRESSSPACECOMBOBOX_H
#define ADDRESSSPACECOMBOBOX_H

#include <QComboBox>
#include "enums.h"

class AddressSpaceComboBox : public QComboBox
{
    Q_OBJECT
public:
    AddressSpaceComboBox(QWidget *parent = nullptr);

    AddressSpace currentAddressSpace() const;
    void setCurrentAddressSpace(AddressSpace asp);

signals:
    void addressSpaceChanged(AddressSpace value);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_currentIndexChanged(int);

private:
    AddressSpace _currentValue;
};

#endif // ADDRESSSPACECOMBOBOX_H

