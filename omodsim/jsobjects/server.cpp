#include "server.h"
#include "ansiutils.h"
#include "byteorderutils.h"

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
    return _mbMultiServer->useGlobalUnitMap();
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
quint16 Server::readHolding(quint16 address) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    const auto data = _mbMultiServer->data(QModbusDataUnit::HoldingRegisters, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeValue
/// \param address
/// \param value
///
void Server::writeHolding(quint16 address, quint16 value)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeValue(QModbusDataUnit::HoldingRegisters, address, value, *_byteOrder);
}

///
/// \brief Server::readInput
/// \param address
/// \return
///
quint16 Server::readInput(quint16 address) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    const auto data = _mbMultiServer->data(QModbusDataUnit::InputRegisters, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeInput
/// \param address
/// \param value
///
void Server::writeInput(quint16 address, quint16 value)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeValue(QModbusDataUnit::InputRegisters, address, value, *_byteOrder);
}

///
/// \brief Server::readDiscrete
/// \param address
/// \return
///
bool Server::readDiscrete(quint16 address) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    const auto data = _mbMultiServer->data(QModbusDataUnit::DiscreteInputs, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeDiscrete
/// \param address
/// \param value
///
void Server::writeDiscrete(quint16 address, bool value)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeValue(QModbusDataUnit::DiscreteInputs, address, value, *_byteOrder);
}

///
/// \brief Server::readCoil
/// \param address
/// \return
///
bool Server::readCoil(quint16 address) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    const auto data = _mbMultiServer->data(QModbusDataUnit::Coils, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeCoil
/// \param address
/// \param value
///
void Server::writeCoil(quint16 address, bool value)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeValue(QModbusDataUnit::Coils, address, value, *_byteOrder);
}

///
/// \brief Server::readAnsi
/// \param reg
/// \param address
/// \param swapped
/// \return
///
QString Server::readAnsi(Register::Type reg, quint16 address, const QString& codepage) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    const auto data = _mbMultiServer->data((QModbusDataUnit::RegisterType)reg, address, 1);
    return printableAnsi(uint16ToAnsi(toByteOrderValue(data.value(0), *_byteOrder)), codepage);
}

///
/// \brief Server::writeAnsi
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeAnsi(Register::Type reg, quint16 address, const QString& value, const QString& codepage)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    auto codec = QTextCodec::codecForName(codepage.toUtf8());
    if(codec == nullptr) codec = QTextCodec::codecForLocale();
    _mbMultiServer->writeValue(QModbusDataUnit::HoldingRegisters, address, uint16FromAnsi(codec->fromUnicode(value)), *_byteOrder);
}

///
/// \brief Server::readInt32
/// \param reg
/// \param address
/// \param swapped
/// \return
///
qint32 Server::readInt32(Register::Type reg, quint16 address, bool swapped) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readInt32((QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeInt32
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeInt32(Register::Type reg, quint16 address, qint32 value, bool swapped)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeInt32((QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readUInt32
/// \param reg
/// \param address
/// \param swapped
/// \return
///
quint32 Server::readUInt32(Register::Type reg, quint16 address, bool swapped) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readUInt32((QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeUInt32
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeUInt32(Register::Type reg, quint16 address, quint32 value, bool swapped)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeUInt32((QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readInt64
/// \param reg
/// \param address
/// \param swapped
/// \return
///
qint64 Server::readInt64(Register::Type reg, quint16 address, bool swapped) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readInt64((QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeInt64
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeInt64(Register::Type reg, quint16 address, qint64 value, bool swapped)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeInt64((QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readUInt64
/// \param reg
/// \param address
/// \param swapped
/// \return
///
quint64 Server::readUInt64(Register::Type reg, quint16 address, bool swapped) const
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readUInt64((QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeUInt64
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeUInt64(Register::Type reg, quint16 address, quint64 value, bool swapped)
{
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeUInt64((QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
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
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readFloat((QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
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
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeFloat((QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
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
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    return _mbMultiServer->readDouble((QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
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
    address -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeDouble((QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
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
        const quint16 address = data.startAddress() + i + (_addressBase == Address::Base::Base0 ? 0 : 1);
        if(_mapOnChange.contains({reg, address}))
        {
            _mapOnChange[{reg, address}].call(QJSValueList() << data.value(i));
        }
    }
}
