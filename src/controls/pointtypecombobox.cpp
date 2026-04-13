#include "pointtypecombobox.h"


///
/// \brief PointTypeComboBox::PointTypeComboBox
/// \param parent
///
PointTypeComboBox::PointTypeComboBox(QWidget *parent)
    : QComboBox(parent)
{
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

