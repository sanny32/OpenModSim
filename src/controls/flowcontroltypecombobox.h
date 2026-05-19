// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file flowcontroltypecombobox.h
/// \brief Declares the flowcontroltypecombobox interfaces.
///

#ifndef FLOWCONTROLTYPECOMBOBOX_H
#define FLOWCONTROLTYPECOMBOBOX_H

#include <QComboBox>
#include <QSerialPort>

///
/// \brief The FlowControlTypeComboBox class
///
class FlowControlTypeComboBox : public QComboBox
{
    Q_OBJECT

public:
    FlowControlTypeComboBox(QWidget *parent = nullptr);

    QSerialPort::FlowControl currentFlowControl() const;
    void setCurrentFlowControl(QSerialPort::FlowControl fctrl);
};

#endif // FLOWCONTROLTYPECOMBOBOX_H

