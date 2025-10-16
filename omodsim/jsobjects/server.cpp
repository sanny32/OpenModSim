#include "server.h"
#include "ansiutils.h"
#include "byteorderutils.h"

///
/// \brief qHash
/// \param key
/// \param seed
/// \return
///
uint qHash(const Server::KeyOnChange &key, uint seed)
{
    return ::qHash(static_cast<uint>(key.DeviceId), seed) ^
           ::qHash(static_cast<int>(key.Type), seed) ^
           ::qHash(key.Address, seed);
}

///
/// \brief Server::Server
/// \param server
/// \param order
/// \param base
///
Server::Server(ModbusMultiServer* server, const ByteOrder* order, AddressBase base)
    :_addressBase((Address::Base)base)
    ,_byteOrder(order)
    ,_mbMultiServer(server)
{
    Q_ASSERT(_byteOrder != nullptr);
    Q_ASSERT(_mbMultiServer != nullptr);

    connect(_mbMultiServer, &ModbusMultiServer::dataChanged, this, &Server::on_dataChanged);
    connect(_mbMultiServer, &ModbusMultiServer::errorOccured, this, &Server::on_errorOccured);
}

///
/// \brief Server::~Server
///
Server::~Server()
{
    disconnect(_mbMultiServer, &ModbusMultiServer::dataChanged, this, &Server::on_dataChanged);
}

///
/// \brief Server::addressBase
/// \return
///
Address::Base Server::addressBase() const
{
    return _addressBase;
}

///
/// \brief Server::setAddressBase
/// \param base
///
void Server::setAddressBase(Address::Base base)
{
    _addressBase = base;
}

///
/// \brief Server::useGlobalUnitMap
/// \return
///
bool Server::useGlobalUnitMap() const
{
    return _mbMultiServer->useGlobalUnitMap() == ModbusMultiServer::GlobalUnitMapStatus::CompletelySet;
}

///
/// \brief Server::setUseGlobalUnitMap
/// \param value
///
void Server::setUseGlobalUnitMap(bool value)
{
    _mbMultiServer->setUseGlobalUnitMap(value);
}

///
/// \brief Server::readHolding
/// \param address
/// \return
///
quint16 Server::readHolding(quint16 address, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    const auto data = _mbMultiServer->data(deviceId, QModbusDataUnit::HoldingRegisters, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeValue
/// \param address
/// \param value
///
void Server::writeHolding(quint16 address, quint16 value, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeValue(deviceId, QModbusDataUnit::HoldingRegisters, address, value, *_byteOrder);
}

///
/// \brief Server::readInput
/// \param address
/// \return
///
quint16 Server::readInput(quint16 address, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    const auto data = _mbMultiServer->data(deviceId, QModbusDataUnit::InputRegisters, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeInput
/// \param address
/// \param value
///
void Server::writeInput(quint16 address, quint16 value, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeValue(deviceId, QModbusDataUnit::InputRegisters, address, value, *_byteOrder);
}

///
/// \brief Server::readDiscrete
/// \param address
/// \return
///
bool Server::readDiscrete(quint16 address, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    const auto data = _mbMultiServer->data(deviceId, QModbusDataUnit::DiscreteInputs, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeDiscrete
/// \param address
/// \param value
///
void Server::writeDiscrete(quint16 address, bool value, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeValue(deviceId, QModbusDataUnit::DiscreteInputs, address, value, *_byteOrder);
}

///
/// \brief Server::readCoil
/// \param address
/// \return
///
bool Server::readCoil(quint16 address, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    const auto data = _mbMultiServer->data(deviceId, QModbusDataUnit::Coils, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeCoil
/// \param address
/// \param value
///
void Server::writeCoil(quint16 address, bool value, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeValue(deviceId, QModbusDataUnit::Coils, address, value, *_byteOrder);
}

///
/// \brief Server::readAnsi
/// \param reg
/// \param address
/// \param swapped
/// \return
///
QString Server::readAnsi(Register::Type reg, quint16 address, const QString& codepage, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    const auto data = _mbMultiServer->data(deviceId, (QModbusDataUnit::RegisterType)reg, address, 1);
    return printableAnsi(uint16ToAnsi(toByteOrderValue(data.value(0), *_byteOrder)), codepage);
}

///
/// \brief Server::writeAnsi
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeAnsi(Register::Type reg, quint16 address, const QString& value, const QString& codepage, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    auto codec = QTextCodec::codecForName(codepage.toUtf8());
    if(codec == nullptr) codec = QTextCodec::codecForLocale();
    _mbMultiServer->writeValue(deviceId, QModbusDataUnit::HoldingRegisters, address, uint16FromAnsi(codec->fromUnicode(value)), *_byteOrder);
}

///
/// \brief Server::readInt32
/// \param reg
/// \param address
/// \param swapped
/// \return
///
qint32 Server::readInt32(Register::Type reg, quint16 address, bool swapped, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readInt32(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeInt32
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeInt32(Register::Type reg, quint16 address, qint32 value, bool swapped, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeInt32(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readUInt32
/// \param reg
/// \param address
/// \param swapped
/// \return
///
quint32 Server::readUInt32(Register::Type reg, quint16 address, bool swapped, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readUInt32(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeUInt32
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeUInt32(Register::Type reg, quint16 address, quint32 value, bool swapped, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeUInt32(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readInt64
/// \param reg
/// \param address
/// \param swapped
/// \return
///
qint64 Server::readInt64(Register::Type reg, quint16 address, bool swapped, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readInt64(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeInt64
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeInt64(Register::Type reg, quint16 address, qint64 value, bool swapped, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeInt64(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readUInt64
/// \param reg
/// \param address
/// \param swapped
/// \return
///
quint64 Server::readUInt64(Register::Type reg, quint16 address, bool swapped, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readUInt64(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeUInt64
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeUInt64(Register::Type reg, quint16 address, quint64 value, bool swapped, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeUInt64(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readFloat
/// \param reg
/// \param address
/// \param swapped
/// \return
///
float Server::readFloat(Register::Type reg, quint16 address, bool swapped, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readFloat(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeFloat
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeFloat(Register::Type reg, quint16 address, float value, bool swapped, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeFloat(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readDouble
/// \param reg
/// \param address
/// \param swapped
/// \return
///
double Server::readDouble(Register::Type reg, quint16 address, bool swapped, quint8 deviceId) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readDouble(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeDouble
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeDouble(Register::Type reg, quint16 address, double value, bool swapped, quint8 deviceId)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeDouble(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::onChange
/// \param reg
/// \param address
/// \param func
///
void Server::onChange(quint8 deviceId, Register::Type reg, quint16 address, const QJSValue& func)
{
    if(!func.isCallable())
        return;

    _mapOnChange[{deviceId, reg, address}] = func;
}

///
/// \brief Server::onError
/// \param deviceId
/// \param func
///
void Server::onError(quint8 deviceId, const QJSValue& func)
{
    if(!func.isCallable())
        return;

    _mapOnError[deviceId] = func;
}

///
/// \brief Server::on_dataChanged
/// \param data
///
void Server::on_dataChanged(quint8 deviceId, const QModbusDataUnit& data)
{
    const auto reg = (Register::Type)data.registerType();
    for(uint i = 0; i < data.valueCount(); i++)
    {
        const quint16 address = data.startAddress() + i + (_addressBase == Address::Base::Base0 ? 0 : 1);
        if(_mapOnChange.contains({deviceId, reg, address}))
        {
            _mapOnChange[{deviceId, reg, address}].call(QJSValueList() << data.value(i));
        }
    }
}

///
/// \brief Server::on_errorOccured
/// \param deviceId
/// \param error
///
void Server::on_errorOccured(quint8 deviceId, const QString& error)
{
    if(error.isEmpty())
        return;

    _mapOnError[deviceId].call(QJSValueList() << error);
}
