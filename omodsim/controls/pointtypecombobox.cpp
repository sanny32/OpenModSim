#include "pointtypecombobox.h"


///
/// \brief PointTypeComboBox::PointTypeComboBox
/// \param parent
///
PointTypeComboBox::PointTypeComboBox(QWidget *parent)
    : QComboBox(parent)
{
    addItem("COIL STATUS", QModbusDataUnit::Coils);
    addItem("INPUT STATUS", QModbusDataUnit::DiscreteInputs);
    addItem("HOLDING REGISTER", QModbusDataUnit::HoldingRegisters);
    addItem("INPUT REGISTER", QModbusDataUnit::InputRegisters);

    connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PointTypeComboBox::on_currentIndexChanged);
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
