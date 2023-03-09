#include "server.h"

///
/// \brief Server::Server
///
Server::Server(ModbusMultiServer& server)
    :_mbMultiServer(server)
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
    return data.value(0);
}

///
/// \brief Server::writeValue
/// \param address
/// \param value
///
void Server::writeHolding(quint16 address, quint16 value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::HoldingRegisters, address - 1, value, ByteOrder::LittleEndian);
}

///
/// \brief Server::readInput
/// \param address
/// \return
///
quint16 Server::readInput(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::InputRegisters, address - 1, 1);
    return data.value(0);
}

///
/// \brief Server::writeInput
/// \param address
/// \param value
///
void Server::writeInput(quint16 address, quint16 value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::InputRegisters, address - 1, value, ByteOrder::LittleEndian);
}

///
/// \brief Server::readDiscrete
/// \param address
/// \return
///
bool Server::readDiscrete(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::DiscreteInputs, address - 1, 1);
    return data.value(0);
}

///
/// \brief Server::writeDiscrete
/// \param address
/// \param value
///
void Server::writeDiscrete(quint16 address, bool value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::DiscreteInputs, address - 1, value, ByteOrder::LittleEndian);
}

///
/// \brief Server::readCoil
/// \param address
/// \return
///
bool Server::readCoil(quint16 address)
{
    const auto data = _mbMultiServer.data(QModbusDataUnit::Coils, address - 1, 1);
    return data.value(0);
}

///
/// \brief Server::writeCoil
/// \param address
/// \param value
///
void Server::writeCoil(quint16 address, bool value)
{
    _mbMultiServer.writeValue(QModbusDataUnit::Coils, address - 1, value, ByteOrder::LittleEndian);
}
