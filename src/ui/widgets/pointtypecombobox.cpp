// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file pointtypecombobox.cpp
/// \brief Implements the pointtypecombobox functionality.
///

#include "pointtypecombobox.h"


///
/// \brief PointTypeComboBox::PointTypeComboBox
/// \param parent
///
PointTypeComboBox::PointTypeComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setSizeAdjustPolicy(QComboBox::AdjustToContents);

    addItem(tr("Coils (Read-Write)"), QModbusDataUnit::Coils);
    addItem(tr("Discrete Inputs (Read Only)"), QModbusDataUnit::DiscreteInputs);
    addItem(tr("Holding Registers (Read-Write)"), QModbusDataUnit::HoldingRegisters);
    addItem(tr("Input Registers (Read Only)"), QModbusDataUnit::InputRegisters);

    connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PointTypeComboBox::on_currentIndexChanged);
}

///
/// \brief PointTypeComboBox::changeEvent
/// \param event
///
void PointTypeComboBox::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        for(int i = 0; i < count(); i++)
        {
            const auto enron = static_cast<EnronModbus::RegisterType>(itemData(i, Qt::UserRole + 1).toInt());
            if (enron != EnronModbus::RegisterType::None) {
                setItemText(i, enron == EnronModbus::RegisterType::Group5x
                    ? tr("Enron Registers Int32 (Read-Write)") : tr("Enron Registers Float32 (Read-Write)"));
                continue;
            }
            switch(itemData(i).value<QModbusDataUnit::RegisterType>())
            {
                case QModbusDataUnit::Coils:
                    setItemText(i, tr("Coils (Read-Write)"));
                break;

                case QModbusDataUnit::DiscreteInputs:
                    setItemText(i, tr("Discrete Inputs (Read Only)"));
                break;

                case QModbusDataUnit::HoldingRegisters:
                    setItemText(i, tr("Holding Registers (Read-Write)"));
                break;

                case QModbusDataUnit::InputRegisters:
                    setItemText(i, tr("Input Registers (Read Only)"));
                break;

                default: break;
            }
        }
    }

    QComboBox::changeEvent(event);
}

///
/// \brief PointTypeComboBox::currentPointType
/// \return
///
QModbusDataUnit::RegisterType PointTypeComboBox::currentPointType() const
{
    return currentData().value<QModbusDataUnit::RegisterType>();
}

///
/// \brief PointTypeComboBox::setCurrentPointType
/// \param pointType
///
void PointTypeComboBox::setCurrentPointType(QModbusDataUnit::RegisterType pointType)
{
    const auto idx = findData(pointType);
    setCurrentIndex(idx);
}

///
/// \brief PointTypeComboBox::on_currentIndexChanged
/// \param index
///
void PointTypeComboBox::on_currentIndexChanged(int index)
{
    emit pointTypeChanged(itemData(index).value<QModbusDataUnit::RegisterType>());
}


///
/// \brief Returns the Enron group selected for the holding-register table.
///
EnronModbus::RegisterType PointTypeComboBox::currentEnronType() const
{
    return static_cast<EnronModbus::RegisterType>(currentData(Qt::UserRole + 1).toInt());
}

///
/// \brief Selects an Enron group without introducing a separate Modbus table.
///
void PointTypeComboBox::setCurrentEnronType(EnronModbus::RegisterType type)
{
    if (type == EnronModbus::RegisterType::None) {
        if (currentEnronType() != EnronModbus::RegisterType::None)
            setCurrentPointType(currentPointType());
        return;
    }
    const int index = findData(int(type), Qt::UserRole + 1);
    if (index >= 0)
        setCurrentIndex(index);
}

///
/// \brief Adds or removes Enron choices to match the server protocol.
///
void PointTypeComboBox::setEnronEnabled(bool enabled)
{
    const bool present = findData(int(EnronModbus::RegisterType::Group5x), Qt::UserRole + 1) >= 0;
    if (enabled == present)
        return;

    if (enabled) {
        addItem(tr("Enron Registers Int32 (Read-Write)"), QModbusDataUnit::HoldingRegisters);
        setItemData(count() - 1, int(EnronModbus::RegisterType::Group5x), Qt::UserRole + 1);
        addItem(tr("Enron Registers Float32 (Read-Write)"), QModbusDataUnit::HoldingRegisters);
        setItemData(count() - 1, int(EnronModbus::RegisterType::Group7x), Qt::UserRole + 1);
        return;
    }

    if (currentEnronType() != EnronModbus::RegisterType::None)
        setCurrentPointType(QModbusDataUnit::HoldingRegisters);
    for (int i = count() - 1; i >= 0; --i) {
        if (itemData(i, Qt::UserRole + 1).toInt() != 0)
            removeItem(i);
    }
}
