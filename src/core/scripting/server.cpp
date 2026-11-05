// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file server.cpp
/// \brief Implements the server functionality.
///

#include <QCoreApplication>
#include "server.h"
#include "ansiutils.h"
#include "byteorderutils.h"
#include "modbusmessages.h"

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
Server::Server(ModbusMultiServer* server, const ByteOrder* order, AddressBase base, QJSEngine* engine)
    :_addressBase((Address::Base)base)
    ,_byteOrder(order)
    ,_mbMultiServer(server)
    ,_jsEngine(engine)
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

    // Deactivate shared call state so any in-flight lambda becomes a no-op
    // and does not reference 'this' after destruction.
    if(_callState)
        _callState->active = false;

    // Post async cleanup to worker thread via QueuedConnection.
    // BlockingQueuedConnection must NOT be used here: if the worker is blocked on
    // sem.acquire() waiting for the main thread to process an invokeMethod event,
    // using BlockingQueuedConnection would cause a deadlock.
    QMetaObject::invokeMethod(_mbMultiServer, [mbms = _mbMultiServer]()
    {
        mbms->setRequestHandler(nullptr);
    }, Qt::QueuedConnection);
}

///
/// \brief Server::deviceId
/// \return
///
int Server::deviceId() const
{
    return _deviceId;
}

///
/// \brief Server::setDeviceId
/// \param deviceId
///
void Server::setDeviceId(int deviceId)
{
    if(deviceId < 0 || deviceId > 255)
    {
        emit errorOccured(static_cast<quint8>(_deviceId), tr("An incorrect device ID was specified (%1)").arg(deviceId));
        return;
    }

    _deviceId = deviceId;
}

///
/// \brief Server::resolveDeviceId
/// \param deviceId
/// \return the explicitly requested unit, or Server.deviceId when omitted
///
quint8 Server::resolveDeviceId(int deviceId) const
{
    return static_cast<quint8>(deviceId < 0 ? _deviceId : deviceId);
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
/// \brief Server::addressSpace
/// \return
///
Address::Space Server::addressSpace() const
{
    return (Address::Space)_mbMultiServer->getModbusDefinitions().AddrSpace;
}

///
/// \brief Server::setAddressSpace
/// \param space
///
void Server::setAddressSpace(Address::Space space)
{
    auto defs = _mbMultiServer->getModbusDefinitions();
    defs.AddrSpace = (AddressSpace)space;
    _mbMultiServer->setModbusDefinitions(defs);
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
/// \brief Server::responseDelayTime
/// \return
///
int  Server::responseDelayTime() const
{
    return _mbMultiServer->getModbusDefinitions().ErrorSimulations.responseDelayTime();
}

///
/// \brief Server::responseRandomDelayUpToTime
/// \return
///
int  Server::responseRandomDelayUpToTime() const
{
    return _mbMultiServer->getModbusDefinitions().ErrorSimulations.responseRandomDelayUpToTime();
}

///
/// \brief Server::noResponse
/// \return
///
bool Server::noResponse() const
{
    return _mbMultiServer->getModbusDefinitions().ErrorSimulations.noResponse();
}

///
/// \brief Server::responseIncorrectId
/// \return
///
bool Server::responseIncorrectId() const
{
    return _mbMultiServer->getModbusDefinitions().ErrorSimulations.responseIncorrectId();
}

///
/// \brief Server::responseIllegalFunction
/// \return
///
bool Server::responseIllegalFunction() const
{
    return _mbMultiServer->getModbusDefinitions().ErrorSimulations.responseIllegalFunction();
}

///
/// \brief Server::responseDeviceBusy
/// \return
///
bool Server::responseDeviceBusy() const
{
    return _mbMultiServer->getModbusDefinitions().ErrorSimulations.responseDeviceBusy();
}

///
/// \brief Server::responseIncorrectCrc
/// \return
///
bool Server::responseIncorrectCrc() const
{
    return _mbMultiServer->getModbusDefinitions().ErrorSimulations.responseIncorrectCrc();
}

///
/// \brief Server::responseDelay
/// \return
///
bool Server::responseDelay() const
{
    return _mbMultiServer->getModbusDefinitions().ErrorSimulations.responseDelay();
}

///
/// \brief Server::responseRandomDelay
/// \return
///
bool Server::responseRandomDelay() const
{
    return _mbMultiServer->getModbusDefinitions().ErrorSimulations.responseRandomDelay();
}

///
/// \brief Server::setResponseDelayTime
/// \param value
///
void Server::setResponseDelayTime(int value)
{
    auto defs = _mbMultiServer->getModbusDefinitions();
    defs.ErrorSimulations.setResponseDelayTime(value);
    _mbMultiServer->setModbusDefinitions(defs);
}

///
/// \brief Server::setResponseRandomDelayUpToTime
/// \param value
///
void Server::setResponseRandomDelayUpToTime(int value)
{
    auto defs = _mbMultiServer->getModbusDefinitions();
    defs.ErrorSimulations.setResponseRandomDelayUpToTime(value);
    _mbMultiServer->setModbusDefinitions(defs);
}

///
/// \brief Server::setNoResponse
/// \param value
///
void Server::setNoResponse(bool value)
{
    auto defs = _mbMultiServer->getModbusDefinitions();
    defs.ErrorSimulations.setNoResponse(value);
    _mbMultiServer->setModbusDefinitions(defs);
}

///
/// \brief Server::setResponseIncorrectId
/// \param value
///
void Server::setResponseIncorrectId(bool value)
{
    auto defs = _mbMultiServer->getModbusDefinitions();
    defs.ErrorSimulations.setResponseIncorrectId(value);
    _mbMultiServer->setModbusDefinitions(defs);
}

///
/// \brief Server::setResponseIllegalFunction
/// \param value
///
void Server::setResponseIllegalFunction(bool value)
{
    auto defs = _mbMultiServer->getModbusDefinitions();
    defs.ErrorSimulations.setResponseIllegalFunction(value);
    _mbMultiServer->setModbusDefinitions(defs);
}

///
/// \brief Server::setResponseDeviceBusy
/// \param value
///
void Server::setResponseDeviceBusy(bool value)
{
    auto defs = _mbMultiServer->getModbusDefinitions();
    defs.ErrorSimulations.setResponseDeviceBusy(value);
    _mbMultiServer->setModbusDefinitions(defs);
}

///
/// \brief Server::setResponseIncorrectCrc
/// \param value
///
void Server::setResponseIncorrectCrc(bool value)
{
    auto defs = _mbMultiServer->getModbusDefinitions();
    defs.ErrorSimulations.setResponseIncorrectCrc(value);
    _mbMultiServer->setModbusDefinitions(defs);
}

///
/// \brief Server::setResponseDelay
/// \param value
///
void Server::setResponseDelay(bool value)
{
    auto defs = _mbMultiServer->getModbusDefinitions();
    defs.ErrorSimulations.setResponseDelay(value);
    _mbMultiServer->setModbusDefinitions(defs);
}

///
/// \brief Server::setResponseRandomDelay
/// \param value
///
void Server::setResponseRandomDelay(bool value)
{
    auto defs = _mbMultiServer->getModbusDefinitions();
    defs.ErrorSimulations.setResponseRandomDelay(value);
    _mbMultiServer->setModbusDefinitions(defs);
}

///
/// \brief Server::toServerAddress converts a script-visible address to the zero-based
/// server address according to the configured address base.
/// \param address The address as seen by the script (0- or 1-based).
/// \return The zero-based address used by the underlying server.
///
quint16 Server::toServerAddress(quint16 address) const
{
    return address - (_addressBase == Address::Base::Base0 ? 0 : 1);
}

///
/// \brief Server::readHolding
/// \param address
/// \return
///
quint16 Server::readHolding(quint16 address, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    const auto data = _mbMultiServer->data(deviceId, QModbusDataUnit::HoldingRegisters, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeValue
/// \param address
/// \param value
///
void Server::writeHolding(quint16 address, quint16 value, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    _mbMultiServer->writeValue(deviceId, QModbusDataUnit::HoldingRegisters, address, value, *_byteOrder);
}

///
/// \brief Server::readInput
/// \param address
/// \return
///
quint16 Server::readInput(quint16 address, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    const auto data = _mbMultiServer->data(deviceId, QModbusDataUnit::InputRegisters, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeInput
/// \param address
/// \param value
///
void Server::writeInput(quint16 address, quint16 value, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    _mbMultiServer->writeValue(deviceId, QModbusDataUnit::InputRegisters, address, value, *_byteOrder);
}

///
/// \brief Server::readDiscrete
/// \param address
/// \return
///
bool Server::readDiscrete(quint16 address, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    const auto data = _mbMultiServer->data(deviceId, QModbusDataUnit::DiscreteInputs, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeDiscrete
/// \param address
/// \param value
///
void Server::writeDiscrete(quint16 address, bool value, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    _mbMultiServer->writeValue(deviceId, QModbusDataUnit::DiscreteInputs, address, value, *_byteOrder);
}

///
/// \brief Server::readCoil
/// \param address
/// \return
///
bool Server::readCoil(quint16 address, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    const auto data = _mbMultiServer->data(deviceId, QModbusDataUnit::Coils, address, 1);
    return toByteOrderValue(data.value(0), *_byteOrder);
}

///
/// \brief Server::writeCoil
/// \param address
/// \param value
///
void Server::writeCoil(quint16 address, bool value, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    _mbMultiServer->writeValue(deviceId, QModbusDataUnit::Coils, address, value, *_byteOrder);
}

///
/// \brief Server::writeHoldings
/// \param startAddress
/// \param values
/// \param deviceId
///
void Server::writeHoldings(quint16 startAddress, const QJSValue& values, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    writeRange(QModbusDataUnit::HoldingRegisters, startAddress, values, deviceId);
}

///
/// \brief Server::writeInputs
/// \param startAddress
/// \param values
/// \param deviceId
///
void Server::writeInputs(quint16 startAddress, const QJSValue& values, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    writeRange(QModbusDataUnit::InputRegisters, startAddress, values, deviceId);
}

///
/// \brief Server::writeCoils
/// \param startAddress
/// \param values
/// \param deviceId
///
void Server::writeCoils(quint16 startAddress, const QJSValue& values, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    writeRange(QModbusDataUnit::Coils, startAddress, values, deviceId);
}

///
/// \brief Server::writeDiscretes
/// \param startAddress
/// \param values
/// \param deviceId
///
void Server::writeDiscretes(quint16 startAddress, const QJSValue& values, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    writeRange(QModbusDataUnit::DiscreteInputs, startAddress, values, deviceId);
}

///
/// \brief Server::writeRange
/// \param type
/// \param startAddress
/// \param values
/// \param deviceId
///
void Server::writeRange(QModbusDataUnit::RegisterType type, quint16 startAddress, const QJSValue& values, quint8 deviceId)
{
    if(!values.isArray())
    {
        emit errorOccured(deviceId, tr("An array of values is expected"));
        return;
    }

    const int count = values.property("length").toInt();
    if(count <= 0)
        return;

    const bool isBitType = (type == QModbusDataUnit::Coils || type == QModbusDataUnit::DiscreteInputs);

    QVector<quint16> data(count);
    for(int i = 0; i < count; i++)
    {
        const auto value = values.property(i);
        data[i] = isBitType ? (value.toBool() ? 1 : 0)
                            : static_cast<quint16>(value.toUInt());
    }

    startAddress -= _addressBase == Address::Base::Base0 ? 0 : 1;
    _mbMultiServer->writeValues(deviceId, type, startAddress, data, *_byteOrder);
}

///
/// \brief Server::readAnsi
/// \param reg
/// \param address
/// \param swapped
/// \return
///
QString Server::readAnsi(Register::Type reg, quint16 address, const QString& codepage, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
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
void Server::writeAnsi(Register::Type reg, quint16 address, const QString& value, const QString& codepage, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
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
qint32 Server::readInt32(Register::Type reg, quint16 address, bool swapped, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    return _mbMultiServer->readInt32(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeInt32
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeInt32(Register::Type reg, quint16 address, qint32 value, bool swapped, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    _mbMultiServer->writeInt32(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readUInt32
/// \param reg
/// \param address
/// \param swapped
/// \return
///
quint32 Server::readUInt32(Register::Type reg, quint16 address, bool swapped, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    return _mbMultiServer->readUInt32(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeUInt32
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeUInt32(Register::Type reg, quint16 address, quint32 value, bool swapped, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    _mbMultiServer->writeUInt32(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readInt64
/// \param reg
/// \param address
/// \param swapped
/// \return
///
qint64 Server::readInt64(Register::Type reg, quint16 address, bool swapped, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    return _mbMultiServer->readInt64(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeInt64
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeInt64(Register::Type reg, quint16 address, qint64 value, bool swapped, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    _mbMultiServer->writeInt64(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readUInt64
/// \param reg
/// \param address
/// \param swapped
/// \return
///
quint64 Server::readUInt64(Register::Type reg, quint16 address, bool swapped, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    return _mbMultiServer->readUInt64(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeUInt64
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeUInt64(Register::Type reg, quint16 address, quint64 value, bool swapped, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    _mbMultiServer->writeUInt64(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readFloat
/// \param reg
/// \param address
/// \param swapped
/// \return
///
float Server::readFloat(Register::Type reg, quint16 address, bool swapped, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    return _mbMultiServer->readFloat(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeFloat
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeFloat(Register::Type reg, quint16 address, float value, bool swapped, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    _mbMultiServer->writeFloat(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::readDouble
/// \param reg
/// \param address
/// \param swapped
/// \return
///
double Server::readDouble(Register::Type reg, quint16 address, bool swapped, int deviceIdArg) const
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    return _mbMultiServer->readDouble(deviceId, (QModbusDataUnit::RegisterType)reg, address, *_byteOrder, swapped);
}

///
/// \brief Server::writeDouble
/// \param reg
/// \param address
/// \param value
/// \param swapped
///
void Server::writeDouble(Register::Type reg, quint16 address, double value, bool swapped, int deviceIdArg)
{
    const quint8 deviceId = resolveDeviceId(deviceIdArg);
    address = toServerAddress(address);
    _mbMultiServer->writeDouble(deviceId, (QModbusDataUnit::RegisterType)reg, address, value, *_byteOrder, swapped);
}

///
/// \brief Server::onRequest
/// \param func
///
void Server::onRequest(const QJSValue& func)
{
    // Deactivate any existing handler so in-flight lambdas become no-ops.
    if(_callState)
        _callState->active = false;
    _callState.reset();

    if(!func.isCallable())
    {
        // Post async cleanup - do NOT use BlockingQueuedConnection here (deadlock risk).
///
/// \brief QMetaObject::invokeMethod
///
        QMetaObject::invokeMethod(_mbMultiServer, [mbms = _mbMultiServer]()
        {
            mbms->setRequestHandler(nullptr);
        }, Qt::QueuedConnection);
        return;
    }

    auto state = JsCallStatePtr::create();
    state->engine = _jsEngine;
    state->callback = func;
    _callState = state;

    auto handler = RequestHandlerPtr(new RequestHandlerFunc(
        [state](const QModbusPdu& pdu, int deviceId, QModbusResponse& response) -> bool
        {
            bool handled = false;
            QSemaphore sem;

            // Target QCoreApplication::instance() - never destroyed, so the queued
            // event is guaranteed to be delivered and sem.release() will always be
            // called (unlike targeting 'this' which gets cancelled on Server destruction).
            QMetaObject::invokeMethod(QCoreApplication::instance(),
                [state, &pdu, deviceId, &response, &handled, &sem]()
                {
                    if(state->active)
                        handled = runJsHandler(state, pdu, deviceId, response);
                    sem.release();
                }, Qt::QueuedConnection);

            sem.acquire();
            return handled;
        }
    ));

    // Post setup to worker thread via QueuedConnection to avoid deadlock
    // if onRequest() is called from within the request callback itself.
///
/// \brief QMetaObject::invokeMethod
///
    QMetaObject::invokeMethod(_mbMultiServer, [mbms = _mbMultiServer, handler]()
    {
        mbms->setRequestHandler(handler);
    }, Qt::QueuedConnection);
}

///
/// \brief Server::runJsHandler
/// \param state
/// \param pdu
/// \param deviceId
/// \param response
/// \return
///
bool Server::runJsHandler(const JsCallStatePtr& state, const QModbusPdu& pdu, int deviceId, QModbusResponse& response)
{
    if(!state->callback.isCallable())
        return false;

    if(!state->engine || state->engine->isInterrupted())
        return false;

    // Wrap in ModbusMessage to get correct function code (handles FC > 127 sign issue)
    // and typed field access via subclass API.
    const auto msg = ModbusMessage::create(pdu, ModbusMessage::Rtu, deviceId, 0, QDateTime(), true);
    const QModbusPdu::FunctionCode fc = msg->functionCode();

    // Build JS request object
    QJSValue jsRequest = state->engine->newObject();
    jsRequest.setProperty("deviceId",     deviceId);
    jsRequest.setProperty("functionCode", static_cast<int>(fc));

    // Always expose raw PDU data bytes as a JS array
    const QByteArray data = pdu.data();
    QJSValue jsData = state->engine->newArray(data.size());
    for(int i = 0; i < data.size(); i++)
        jsData.setProperty(i, static_cast<int>(quint8(data[i])));
    jsRequest.setProperty("data", jsData);

    // Decode function-specific fields using ModbusMessage subclass API
    if(const auto* req = dynamic_cast<const ReadCoilsRequest*>(msg.get()))
    {
        jsRequest.setProperty("address", req->startAddress());
        jsRequest.setProperty("count",   req->length());
    }
    else if(const auto* req = dynamic_cast<const ReadDiscreteInputsRequest*>(msg.get()))
    {
        jsRequest.setProperty("address", req->startAddress());
        jsRequest.setProperty("count",   req->length());
    }
    else if(const auto* req = dynamic_cast<const ReadHoldingRegistersRequest*>(msg.get()))
    {
        jsRequest.setProperty("address", req->startAddress());
        jsRequest.setProperty("count",   req->length());
    }
    else if(const auto* req = dynamic_cast<const ReadInputRegistersRequest*>(msg.get()))
    {
        jsRequest.setProperty("address", req->startAddress());
        jsRequest.setProperty("count",   req->length());
    }
    else if(const auto* req = dynamic_cast<const WriteSingleCoilRequest*>(msg.get()))
    {
        jsRequest.setProperty("address", req->address());
        jsRequest.setProperty("value",   req->value());
    }
    else if(const auto* req = dynamic_cast<const WriteSingleRegisterRequest*>(msg.get()))
    {
        jsRequest.setProperty("address", req->address());
        jsRequest.setProperty("value",   req->value());
    }
    else if(const auto* req = dynamic_cast<const WriteMultipleCoilsRequest*>(msg.get()))
    {
        jsRequest.setProperty("address", req->startAddress());
        jsRequest.setProperty("count",   req->quantity());

        const QByteArray coilBytes = req->values();
        const quint16 count = req->quantity();
        QJSValue jsValues = state->engine->newArray(count);
        for(int i = 0; i < count; i++)
        {
            const bool bitVal = (i / 8 < coilBytes.size()) ? ((quint8(coilBytes[i / 8]) >> (i % 8)) & 1) : false;
            jsValues.setProperty(i, bitVal);
        }
        jsRequest.setProperty("values", jsValues);
    }
    else if(const auto* req = dynamic_cast<const WriteMultipleRegistersRequest*>(msg.get()))
    {
        jsRequest.setProperty("address", req->startAddress());
        jsRequest.setProperty("count",   req->quantity());

        const QByteArray regBytes = req->values();
        const quint16 count = req->quantity();
        QJSValue jsValues = state->engine->newArray(count);
        for(int i = 0; i < count && (i * 2 + 1) < regBytes.size(); i++)
        {
            const quint16 v = (quint8(regBytes[i * 2]) << 8) | quint8(regBytes[i * 2 + 1]);
            jsValues.setProperty(i, v);
        }
        jsRequest.setProperty("values", jsValues);
    }
    else if(const auto* req = dynamic_cast<const MaskWriteRegisterRequest*>(msg.get()))
    {
        jsRequest.setProperty("address", req->address());
        jsRequest.setProperty("andMask", req->andMask());
        jsRequest.setProperty("orMask",  req->orMask());
    }
    else if(const auto* req = dynamic_cast<const ReadWriteMultipleRegistersRequest*>(msg.get()))
    {
        jsRequest.setProperty("readAddress",  req->readStartAddress());
        jsRequest.setProperty("readCount",    req->readLength());
        jsRequest.setProperty("writeAddress", req->writeStartAddress());
        jsRequest.setProperty("writeCount",   req->writeLength());

        const QByteArray writeBytes = req->writeValues();
        const quint16 writeCount = req->writeLength();
        QJSValue jsValues = state->engine->newArray(writeCount);
        for(int i = 0; i < writeCount && (i * 2 + 1) < writeBytes.size(); i++)
        {
            const quint16 v = (quint8(writeBytes[i * 2]) << 8) | quint8(writeBytes[i * 2 + 1]);
            jsValues.setProperty(i, v);
        }
        jsRequest.setProperty("values", jsValues);
    }
    else if(const auto* req = dynamic_cast<const DiagnosticsRequest*>(msg.get()))
    {
        jsRequest.setProperty("subfunc", req->subfunc());
    }
    else if(const auto* req = dynamic_cast<const ReadFileRecordRequest*>(msg.get()))
    {
        jsRequest.setProperty("byteCount", req->byteCount());
    }
    else if(const auto* req = dynamic_cast<const WriteFileRecordRequest*>(msg.get()))
    {
        jsRequest.setProperty("byteCount", req->length());
    }
    else if(const auto* req = dynamic_cast<const ReadFifoQueueRequest*>(msg.get()))
    {
        jsRequest.setProperty("address", req->fifoAddress());
    }

    // Call JS callback
    const QJSValue result = state->callback.call({jsRequest});

    if(result.isError())
        return false;

    if(result.isNull() || result.isUndefined())
        return false;

    // Exception response: { exception: N }
    if(result.isObject() && result.hasProperty("exception"))
    {
        const int exCode = result.property("exception").toInt();
        response = QModbusExceptionResponse(fc, static_cast<QModbusExceptionResponse::ExceptionCode>(exCode));
        return true;
    }

    // Raw data response: { data: [byte1, byte2, ...] }
    if(result.isObject() && result.hasProperty("data"))
    {
        QJSValue jsRespData = result.property("data");
        if(jsRespData.isArray())
        {
            QByteArray respData;
            const int len = jsRespData.property("length").toInt();
            for(int i = 0; i < len; i++)
                respData.append(char(jsRespData.property(i).toInt()));
            response = QModbusResponse(fc, respData);
            return true;
        }
    }

    // Normal response: array of values
    if(result.isArray())
    {
        const int len = result.property("length").toInt();
        switch(fc)
        {
            case QModbusPdu::ReadHoldingRegisters:
            case QModbusPdu::ReadInputRegisters:
            {
                QByteArray respData;
                respData.append(char(len * 2));
                for(int i = 0; i < len; i++)
                {
                    const quint16 v = result.property(i).toUInt();
                    respData.append(char(v >> 8));
                    respData.append(char(v & 0xFF));
                }
                response = QModbusResponse(fc, respData);
                return true;
            }

            case QModbusPdu::ReadCoils:
            case QModbusPdu::ReadDiscreteInputs:
            {
                const int byteCount = (len + 7) / 8;
                QByteArray respData(byteCount + 1, 0);
                respData[0] = char(byteCount);
                for(int i = 0; i < len; i++)
                {
                    if(result.property(i).toBool())
                        respData[1 + i / 8] = char(uchar(respData[1 + i / 8]) | uchar(1 << (i % 8)));
                }
                response = QModbusResponse(fc, respData);
                return true;
            }

            default:
            break;
        }
    }

    return false;
}

///
/// \brief Server::onChange
/// \param reg
/// \param address
/// \param func
///
void Server::onChange(int deviceId, Register::Type reg, quint16 address, const QJSValue& func)
{
    if(!func.isCallable())
    {
        emit errorOccured(resolveDeviceId(deviceId), tr("A callback function is expected"));
        return;
    }

    _mapOnChange[{resolveDeviceId(deviceId), reg, address}] = func;
}

///
/// \brief Server::onChange
/// \param reg
/// \param address
/// \param func
///
void Server::onChange(Register::Type reg, quint16 address, const QJSValue& func)
{
    onChange(-1, reg, address, func);
}

///
/// \brief Server::onError
/// \param deviceId
/// \param func
///
void Server::onError(int deviceId, const QJSValue& func)
{
    if(!func.isCallable())
        return;

    _mapOnError[resolveDeviceId(deviceId)] = func;
}

///
/// \brief Server::onError
/// \param func
///
void Server::onError(const QJSValue& func)
{
    onError(-1, func);
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

    if(_mapOnError.contains(deviceId))
        _mapOnError[deviceId].call(QJSValueList() << error);
    else
        emit errorOccured(deviceId, error);
}

