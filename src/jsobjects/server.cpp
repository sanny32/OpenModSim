#include <QCoreApplication>
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
        // Post async cleanup — do NOT use BlockingQueuedConnection here (deadlock risk).
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

            // Target QCoreApplication::instance() — never destroyed, so the queued
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

    // Build JS request object
    QJSValue jsRequest = state->engine->newObject();
    jsRequest.setProperty("deviceId",      deviceId);
    jsRequest.setProperty("functionCode",  static_cast<int>(pdu.functionCode()));

    const auto data = pdu.data();

    // Always expose raw PDU data bytes as a JS array
    QJSValue jsData = state->engine->newArray(data.size());
    for(int i = 0; i < data.size(); i++)
        jsData.setProperty(i, static_cast<int>(quint8(data[i])));
    jsRequest.setProperty("data", jsData);

    switch(pdu.functionCode())
    {
        case QModbusPdu::ReadCoils:
        case QModbusPdu::ReadDiscreteInputs:
        case QModbusPdu::ReadHoldingRegisters:
        case QModbusPdu::ReadInputRegisters:
            if(data.size() >= 4)
            {
                const quint16 addr  = (quint8(data[0]) << 8) | quint8(data[1]);
                const quint16 count = (quint8(data[2]) << 8) | quint8(data[3]);
                jsRequest.setProperty("address", addr);
                jsRequest.setProperty("count",   count);
            }
        break;

        case QModbusPdu::WriteSingleCoil:
        case QModbusPdu::WriteSingleRegister:
            if(data.size() >= 4)
            {
                const quint16 addr  = (quint8(data[0]) << 8) | quint8(data[1]);
                const quint16 value = (quint8(data[2]) << 8) | quint8(data[3]);
                jsRequest.setProperty("address", addr);
                jsRequest.setProperty("value",   value);
            }
        break;

        case QModbusPdu::WriteMultipleCoils:
            if(data.size() >= 5)
            {
                const quint16 addr  = (quint8(data[0]) << 8) | quint8(data[1]);
                const quint16 count = (quint8(data[2]) << 8) | quint8(data[3]);
                const quint8 byteCount = quint8(data[4]);
                jsRequest.setProperty("address", addr);
                jsRequest.setProperty("count",   count);

                QJSValue jsValues = state->engine->newArray(count);
                for(int i = 0; i < count && (i / 8) < byteCount && (5 + i / 8) < data.size(); i++)
                {
                    const bool bitVal = (quint8(data[5 + i / 8]) >> (i % 8)) & 1;
                    jsValues.setProperty(i, bitVal);
                }
                jsRequest.setProperty("values", jsValues);
            }
        break;

        case QModbusPdu::WriteMultipleRegisters:
            if(data.size() >= 5)
            {
                const quint16 addr  = (quint8(data[0]) << 8) | quint8(data[1]);
                const quint16 count = (quint8(data[2]) << 8) | quint8(data[3]);
                jsRequest.setProperty("address", addr);
                jsRequest.setProperty("count",   count);

                QJSValue jsValues = state->engine->newArray(count);
                for(int i = 0; i < count && (5 + i * 2 + 1) < data.size(); i++)
                {
                    const quint16 v = (quint8(data[5 + i * 2]) << 8) | quint8(data[5 + i * 2 + 1]);
                    jsValues.setProperty(i, v);
                }
                jsRequest.setProperty("values", jsValues);
            }
        break;

        case QModbusPdu::MaskWriteRegister:
            if(data.size() >= 6)
            {
                const quint16 addr    = (quint8(data[0]) << 8) | quint8(data[1]);
                const quint16 andMask = (quint8(data[2]) << 8) | quint8(data[3]);
                const quint16 orMask  = (quint8(data[4]) << 8) | quint8(data[5]);
                jsRequest.setProperty("address", addr);
                jsRequest.setProperty("andMask", andMask);
                jsRequest.setProperty("orMask",  orMask);
            }
        break;

        case QModbusPdu::ReadWriteMultipleRegisters:
            if(data.size() >= 9)
            {
                const quint16 readAddr   = (quint8(data[0]) << 8) | quint8(data[1]);
                const quint16 readCount  = (quint8(data[2]) << 8) | quint8(data[3]);
                const quint16 writeAddr  = (quint8(data[4]) << 8) | quint8(data[5]);
                const quint16 writeCount = (quint8(data[6]) << 8) | quint8(data[7]);
                jsRequest.setProperty("readAddress",  readAddr);
                jsRequest.setProperty("readCount",    readCount);
                jsRequest.setProperty("writeAddress", writeAddr);
                jsRequest.setProperty("writeCount",   writeCount);

                QJSValue jsValues = state->engine->newArray(writeCount);
                for(int i = 0; i < writeCount && (9 + i * 2 + 1) < data.size(); i++)
                {
                    const quint16 v = (quint8(data[9 + i * 2]) << 8) | quint8(data[9 + i * 2 + 1]);
                    jsValues.setProperty(i, v);
                }
                jsRequest.setProperty("values", jsValues);
            }
        break;

        default:
        break;
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
        response = QModbusExceptionResponse(pdu.functionCode(),
                       static_cast<QModbusExceptionResponse::ExceptionCode>(exCode));
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
            response = QModbusResponse(pdu.functionCode(), respData);
            return true;
        }
    }

    // Normal response: array of values
    if(result.isArray())
    {
        const int len = result.property("length").toInt();
        switch(pdu.functionCode())
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
                response = QModbusResponse(pdu.functionCode(), respData);
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
                        respData[1 + i / 8] |= char(1 << (i % 8));
                }
                response = QModbusResponse(pdu.functionCode(), respData);
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
