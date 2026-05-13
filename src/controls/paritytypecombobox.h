// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file paritytypecombobox.h
/// \brief Declares the paritytypecombobox interfaces.
///

#ifndef PARITYTYPECOMBOBOX_H
#define PARITYTYPECOMBOBOX_H

#include <QComboBox>
#include <QSerialPort>

///
/// \brief The ParityTypeComboBox class
///
class ParityTypeComboBox : public QComboBox
{
    Q_OBJECT
public:
    ParityTypeComboBox(QWidget *parent = nullptr);

    QSerialPort::Parity currentParity() const;
    void setCurrentParity(QSerialPort::Parity parity);

protected:
    void changeEvent(QEvent* event) override;

private:
    void retranslateUi();
};

#endif // PARITYTYPECOMBOBOX_H

