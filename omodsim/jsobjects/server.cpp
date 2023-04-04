#include "server.h"
#include "byteorderutils.h"

///
/// \brief Server::Server
/// \param server
/// \param order
///
Server::Server(ModbusMultiServer* server, const ByteOrder* order)
    :_byteOrder(order)
    ,_mbMultiServer(server)
{
    Q_ASSERT(_byteOrder != nullptr);
    Q_ASSERT(_mbMultiServer != nullptr);

    connect(_mbMultiServer, &ModbusMultiServer::dataChanged, this, &Server::on_dataChanged);
}

///
/// \brief Server::~Server
///
Server::~Server()
{
    disconnect(_mbMultiServer, &ModbusMultiServer::dataChanged, this, &Server::on_dataChanged);
}

///
/// \brief Server::readHolding
/// \param address
/// \return
///
quint16 Server::readHolding(quint16 address) const
{
    const auto data = _mbMultiServer->data(QModbusDataUnit::HoldingRegisters, address - 1, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeValue
/// \param address
/// \param value
///
void Server::writeHolding(quint16 address, quint16 value)
{
    _mbMultiServer->writeValue(QModbusDataUnit::HoldingRegisters, address - 1, value, *_byteOrder);
}

///
/// \brief Server::readInput
/// \param address
/// \return
///
quint16 Server::readInput(quint16 address) const
{
    const auto data = _mbMultiServer->data(QModbusDataUnit::InputRegisters, address - 1, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeInput
/// \param address
/// \param value
///
void Server::writeInput(quint16 address, quint16 value)
{
    _mbMultiServer->writeValue(QModbusDataUnit::InputRegisters, address - 1, value, *_byteOrder);
}

///
/// \brief Server::readDiscrete
/// \param address
/// \return
///
bool Server::readDiscrete(quint16 address) const
{
    const auto data = _mbMultiServer->data(QModbusDataUnit::DiscreteInputs, address - 1, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeDiscrete
/// \param address
/// \param value
///
void Server::writeDiscrete(quint16 address, bool value)
{
    _mbMultiServer->writeValue(QModbusDataUnit::DiscreteInputs, address - 1, value, *_byteOrder);
}

///
/// \brief Server::readCoil
/// \param address
/// \return
///
bool Server::readCoil(quint16 address) const
{
    const auto data = _mbMultiServer->data(QModbusDataUnit::Coils, address - 1, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeCoil
/// \param address
/// \param value
///
void Server::writeCoil(quint16 address, bool value)
{
    _mbMultiServer->writeValue(QModbusDataUnit::Coils, address - 1, value, *_byteOrder);
}

///
/// \brief Server::readFloat
/// \param reg
/// \param address
/// \param swapped
/// \return
///
float Server::readFloat(Register::Type reg, quint16 address, bool swapped) const
{
    return _mbMultiServer->readFloat((QModbusDataUnit::RegisterType)reg, address - 1, *_byteOrder, swapped);
}

///
/// \brief Server::writeFloat
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeFloat(Register::Type reg, quint16 address, float value, bool swapped)
{
    _mbMultiServer->writeFloat((QModbusDataUnit::RegisterType)reg, address - 1, value, *_byteOrder, swapped);
}

///
/// \brief Server::readDouble
/// \param reg
/// \param address
/// \param swapped
/// \return
///
double Server::readDouble(Register::Type reg, quint16 address, bool swapped) const
{
    return _mbMultiServer->readDouble((QModbusDataUnit::RegisterType)reg, address - 1, *_byteOrder, swapped);
}

///
/// \brief Server::writeDouble
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeDouble(Register::Type reg, quint16 address, double value, bool swapped)
{
    _mbMultiServer->writeDouble((QModbusDataUnit::RegisterType)reg, address - 1, value, *_byteOrder, swapped);
}

///
/// \brief Server::onChange
/// \param reg
/// \param address
/// \param func
///
void Server::onChange(Register::Type reg, quint16 address, const QJSValue& func)
{
    if(!func.isCallable())
        return;

    _mapOnChange[{reg, address}] = func;
}

///
/// \brief Server::on_dataChanged
/// \param data
///
void Server::on_dataChanged(const QModbusDataUnit& data)
{
    const auto reg = (Register::Type)data.registerType();
    for(uint i = 0; i < data.valueCount(); i++)
    {
        const quint16 address = data.startAddress() + i + 1;
        if(_mapOnChange.contains({reg, address}))
        {
            _mapOnChange[{reg, address}].call(QJSValueList() << data.value(i));
        }
    }
}
