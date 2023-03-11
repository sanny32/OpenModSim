#include "server.h"
#include "byteorderutils.h"

///
/// \brief Server::Server
/// \param server
/// \param order
///
Server::Server(ModbusMultiServer& server, const ByteOrder& order)
    :_byteOrder(order)
    ,_mbMultiServer(server)
{
}

///
/// \brief Server::readHolding
/// \param address
/// \return
///
quint16 Server::readHolding(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::HoldingRegisters, address - 1, 1);
    return toByteOrderValue(data.value(0), _byteOrder);
}

///
/// \brief Server::writeValue
/// \param address
/// \param value
///
void Server::writeHolding(quint16 address, quint16 value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::HoldingRegisters, address - 1, value, _byteOrder);
}

///
/// \brief Server::readInput
/// \param address
/// \return
///
quint16 Server::readInput(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::InputRegisters, address - 1, 1);
    return toByteOrderValue(data.value(0), _byteOrder);
}

///
/// \brief Server::writeInput
/// \param address
/// \param value
///
void Server::writeInput(quint16 address, quint16 value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::InputRegisters, address - 1, value, _byteOrder);
}

///
/// \brief Server::readDiscrete
/// \param address
/// \return
///
bool Server::readDiscrete(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::DiscreteInputs, address - 1, 1);
    return toByteOrderValue(data.value(0), _byteOrder);
}

///
/// \brief Server::writeDiscrete
/// \param address
/// \param value
///
void Server::writeDiscrete(quint16 address, bool value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::DiscreteInputs, address - 1, value, _byteOrder);
}

///
/// \brief Server::readCoil
/// \param address
/// \return
///
bool Server::readCoil(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::Coils, address - 1, 1);
    return toByteOrderValue(data.value(0), _byteOrder);
}

///
/// \brief Server::writeCoil
/// \param address
/// \param value
///
void Server::writeCoil(quint16 address, bool value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::Coils, address - 1, value, _byteOrder);
}
