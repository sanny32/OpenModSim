#include "modbusserverobject.h"

///
/// \brief ModbusServerObject::ModbusObject
///
ModbusServerObject::ModbusServerObject(ModbusMultiServer& server)
    :_mbMultiServer(server)
{
}

///
/// \brief ModbusServerObject::readHolding
/// \param address
/// \return
///
quint16 ModbusServerObject::readHolding(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::HoldingRegisters, address - 1, 1);
    return data.value(0);
}

///
/// \brief ModbusServerObject::writeValue
/// \param address
/// \param value
///
void ModbusServerObject::writeHolding(quint16 address, quint16 value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::HoldingRegisters, address - 1, value, ByteOrder::LittleEndian);
}

///
/// \brief ModbusServerObject::readInput
/// \param address
/// \return
///
quint16 ModbusServerObject::readInput(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::InputRegisters, address - 1, 1);
    return data.value(0);
}

///
/// \brief ModbusServerObject::writeInput
/// \param address
/// \param value
///
void ModbusServerObject::writeInput(quint16 address, quint16 value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::InputRegisters, address - 1, value, ByteOrder::LittleEndian);
}

///
/// \brief ModbusServerObject::readDiscrete
/// \param address
/// \return
///
bool ModbusServerObject::readDiscrete(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::DiscreteInputs, address - 1, 1);
    return data.value(0);
}

///
/// \brief ModbusServerObject::writeDiscrete
/// \param address
/// \param value
///
void ModbusServerObject::writeDiscrete(quint16 address, bool value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::DiscreteInputs, address - 1, value, ByteOrder::LittleEndian);
}

///
/// \brief ModbusServerObject::readCoil
/// \param address
/// \return
///
bool ModbusServerObject::readCoil(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::Coils, address - 1, 1);
    return data.value(0);
}

///
/// \brief ModbusServerObject::writeCoil
/// \param address
/// \param value
///
void ModbusServerObject::writeCoil(quint16 address, bool value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::Coils, address - 1, value, ByteOrder::LittleEndian);
}
