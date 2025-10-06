#include <QBitArray>
#include <QModbusDeviceIdentification>
#include <QLoggingCategory>
#include "modbusserver.h"

Q_LOGGING_CATEGORY(QT_MODBUS, "qt.modbus")
Q_LOGGING_CATEGORY(QT_MODBUS_LOW, "qt.modbus.lowlevel")

///
/// \brief ModbusServer::ModbusServer
/// \param parent
///
ModbusServer::ModbusServer(QObject* parent)
    : QObject(parent)
    ,_counters()
{
    {
        static int reg = qRegisterMetaType<QModbusRequest>();
        Q_UNUSED(reg);
    }

    {
        static int reg = qRegisterMetaType<QModbusResponse>();
        Q_UNUSED(reg);
    }

    {
        static int reg = qRegisterMetaType<QModbusDataUnit>();
        Q_UNUSED(reg);
    }
}

///
/// \brief ModbusServer::addServerAddress
/// \param serverAddress
///
void ModbusServer::addServerAddress(int serverAddress)
{
    _serverAddresses.insert(serverAddress);
}

///
/// \brief ModbusServer::removeServerAddress
/// \param serverAddress
///
void ModbusServer::removeServerAddress(int serverAddress)
{
    _serverAddresses.remove(serverAddress);
    if(!hasServerAddress(serverAddress))
    {
        _serversOptions.remove(serverAddress);
        _modbusDataUnitMaps.remove(serverAddress);
        _errors.remove(serverAddress);
        _errorsString.remove(serverAddress);
    }
}

///
/// \brief ModbusServer::removeAllServerAddresses
///
void ModbusServer::removeAllServerAddresses()
{
    _serverAddresses.clear();
    _serversOptions.clear();
    _modbusDataUnitMaps.clear();
    _errors.clear();
    _errorsString.clear();
}

///
/// \brief ModbusServer::hasServerAddress
/// \param serverAddress
/// \return
///
bool ModbusServer::hasServerAddress(int serverAddress)
{
    return _serverAddresses.contains(serverAddress);
}

///
/// \brief ModbusServer::serverAddress
/// \return
///
QList<int> ModbusServer::serverAddresses() const
{
    return _serverAddresses.values();
}

///
/// \brief ModbusServer::setMap
/// \param map
/// \param serverAddress
/// \return
///
bool ModbusServer::setMap(const QModbusDataUnitMap &map, int serverAddress)
{
    _modbusDataUnitMaps[serverAddress] = map;
    return true;
}

///
/// \brief ModbusServer::value
/// \param option
/// \param serverAddress
/// \return
///
QVariant ModbusServer::value(int option, int serverAddress) const
{
    switch (option) {
    case DiagnosticRegister:
        return _serversOptions[serverAddress].value(option, quint16(0x0000));
    case ExceptionStatusOffset:
        return _serversOptions[serverAddress].value(option, quint16(0x0000));
    case DeviceBusy:
        return _serversOptions[serverAddress].value(option, quint16(0x0000));
    case AsciiInputDelimiter:
        return _serversOptions[serverAddress].value(option, '\n');
    case ListenOnlyMode:
        return _serversOptions[serverAddress].value(option, false);
    case ServerIdentifier:
        return _serversOptions[serverAddress].value(option, quint8(0x0a));
    case RunIndicatorStatus:
        return _serversOptions[serverAddress].value(option, quint8(0xff));
    case AdditionalData:
        return _serversOptions[serverAddress].value(option, QByteArray("Qt Modbus Server"));
    case DeviceIdentification:
        return _serversOptions[serverAddress].value(option, QVariant());
    };

    if (option < UserOption)
        return QVariant();

    return _serversOptions[serverAddress].value(option, QVariant());
}

///
/// \brief ModbusServer::setValue
/// \param option
/// \param newValue
/// \param serverAddress
/// \return
///
bool ModbusServer::setValue(int option, const QVariant &newValue, int serverAddress)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define CHECK_INT_OR_UINT(val) \
    do { \
        if ((val.typeId() != QMetaType::Int) && (val.typeId() != QMetaType::UInt)) \
            return false; \
    } while (0)

#define CHECK_BOOL(val) \
        do { \
            if(val.metaType().id() != QMetaType::Bool) \
                return false; \
    } while (0)

#define CHECK_BYTEARRAY(val) \
        do { \
            if(val.metaType().id() != QMetaType::QByteArray) \
                return false; \
        } while (0)
#else
// Qt5
#define CHECK_INT_OR_UINT(val) \
    do { \
        if ((val.type() != QVariant::Int) && (val.type() != QVariant::UInt)) \
            return false; \
    } while (0)

#define CHECK_BOOL(val) \
        do { \
            if (val.type() != QVariant::Bool) \
                return false; \
    } while (0)

#define CHECK_BYTEARRAY(val) \
        do { \
                if (val.type() != QVariant::ByteArray) \
                    return false; \
        } while (0)
#endif

    switch (option) {
    case DiagnosticRegister:
        CHECK_INT_OR_UINT(newValue);
        _serversOptions[serverAddress].insert(option, newValue);
        return true;
    case ExceptionStatusOffset: {
        CHECK_INT_OR_UINT(newValue);
        const quint16 tmp = newValue.value<quint16>();
        QModbusDataUnit coils(QModbusDataUnit::Coils, tmp, 8);
        if (!data(&coils, serverAddress))
            return false;
        _serversOptions[serverAddress].insert(option, tmp);
        return true;
    }
    case DeviceBusy: {
        CHECK_INT_OR_UINT(newValue);
        const quint16 tmp = newValue.value<quint16>();
        if ((tmp != 0x0000) && (tmp != 0xffff))
            return false;
        _serversOptions[serverAddress].insert(option, tmp);
        return true;
    }
    case AsciiInputDelimiter: {
        CHECK_INT_OR_UINT(newValue);
        bool ok = false;
        if (newValue.toUInt(&ok) > 0xff || !ok)
            return false;
        _serversOptions[serverAddress].insert(option, newValue);
        return true;
    }
    case ListenOnlyMode: {
        CHECK_BOOL(newValue);
        _serversOptions[serverAddress].insert(option, newValue);
        return true;
    }
    case ServerIdentifier:
        CHECK_INT_OR_UINT(newValue);
        _serversOptions[serverAddress].insert(option, newValue);
        return true;
    case RunIndicatorStatus: {
        CHECK_INT_OR_UINT(newValue);
        const quint8 tmp = newValue.value<quint8>();
        if ((tmp != 0x00) && (tmp != 0xff))
            return false;
        _serversOptions[serverAddress].insert(option, tmp);
        return true;
    }
    case AdditionalData: {
        CHECK_BYTEARRAY(newValue);
        const QByteArray additionalData = newValue.toByteArray();
        if (additionalData.size() > 249)
            return false;
        _serversOptions[serverAddress].insert(option, additionalData);
        return true;
    }
    case DeviceIdentification:
        if (!newValue.canConvert<QModbusDeviceIdentification>())
            return false;
        _serversOptions[serverAddress].insert(option, newValue);
        return true;
    default:
        break;
    };

    if (option < UserOption)
        return false;
    _serversOptions[serverAddress].insert(option, newValue);
    return true;

#undef CHECK_INT_OR_UINT
}

///
/// \brief ModbusServer::data
/// \param table
/// \param address
/// \param data
/// \param serverAddress
/// \return
///
bool ModbusServer::data(QModbusDataUnit::RegisterType table, quint16 address, quint16 *data, int serverAddress) const
{
    QModbusDataUnit unit(table, address, 1u);
    if (data && readData(&unit, serverAddress)) {
        *data = unit.value(0);
        return true;
    }
    return false;
}

///
/// \brief ModbusServer::connectDevice
/// \return
///
bool ModbusServer::connectDevice()
{
    if (_state != QModbusDevice::UnconnectedState)
        return false;

    setState(QModbusDevice::ConnectingState);

    if (!open()) {
        setState(QModbusDevice::UnconnectedState);
        return false;
    }

    //Connected is set by backend -> might be delayed by event loop
    return true;
}

///
/// \brief ModbusServer::disconnectDevice
///
void ModbusServer::disconnectDevice()
{
    if (state() == QModbusDevice::UnconnectedState)
        return;

    setState(QModbusDevice::ClosingState);

    //Unconnected is set by backend -> might be delayed by event loop
    close();
}

///
/// \brief ModbusServer::setState
/// \param newState
///
void ModbusServer::setState(QModbusDevice::State newState)
{
    if (newState == _state)
        return;

    _state = newState;
    emit stateChanged(newState);
}

///
/// \brief ModbusServer::state
/// \return
///
QModbusServer::State ModbusServer::state() const
{
    return _state;
}

///
/// \brief ModbusServer::setError
/// \param errorText
/// \param error
/// \param serverAddress
///
void ModbusServer::setError(const QString &errorText, QModbusDevice::Error error, int serverAddress)
{
    _errors[serverAddress] = error;
    _errorsString[serverAddress] = errorText;
    emit errorOccurred(error, serverAddress);
}

///
/// \brief ModbusServer::error
/// \param serverAddress
/// \return
///
QModbusServer::Error ModbusServer::error(int serverAddress) const
{
    return _errors[serverAddress];
}

///
/// \brief ModbusServer::errorString
/// \param serverAddress
/// \return
///
QString ModbusServer::errorString(int serverAddress) const
{
    return _errorsString[serverAddress];
}

///
/// \brief ModbusServer::data
/// \param newData
/// \param serverAddress
/// \return
///
bool ModbusServer::data(QModbusDataUnit *newData, int serverAddress) const
{
    return readData(newData, serverAddress);
}

///
/// \brief ModbusServer::setData
/// \param table
/// \param address
/// \param data
/// \param serverAddress
/// \return
///
bool ModbusServer::setData(QModbusDataUnit::RegisterType table, quint16 address, quint16 data, int serverAddress)
{
    return writeData(QModbusDataUnit(table, address, QVector<quint16>() << data), serverAddress);
}

///
/// \brief ModbusServer::setData
/// \param newData
/// \param serverAddress
/// \return
///
bool ModbusServer::setData(const QModbusDataUnit &newData, int serverAddress)
{
    return writeData(newData, serverAddress);
}

///
/// \brief ModbusServer::writeData
/// \param newData
/// \param serverAddress
/// \return
///
bool ModbusServer::writeData(const QModbusDataUnit &newData, int serverAddress)
{
    if (!_modbusDataUnitMaps[serverAddress].contains(newData.registerType()))
        return false;

    QModbusDataUnit &current = _modbusDataUnitMaps[serverAddress][newData.registerType()];
    if (!current.isValid())
        return false;

    // check range start is within internal map range
    int internalRangeEndAddress = current.startAddress() + current.valueCount() - 1;
    if (newData.startAddress() < current.startAddress()
        || newData.startAddress() > internalRangeEndAddress) {
        return false;
    }

    // check range end is within internal map range
    int rangeEndAddress = newData.startAddress() + newData.valueCount() - 1;
    if (rangeEndAddress < current.startAddress() || rangeEndAddress > internalRangeEndAddress)
        return false;

    bool changeRequired = false;
    for (qsizetype i = 0; i < newData.valueCount(); i++) {
        const quint16 newValue = newData.value(i);
        const qsizetype translatedIndex = newData.startAddress() - current.startAddress() + i;
        changeRequired |= (current.value(translatedIndex) != newValue);
        current.setValue(translatedIndex, newValue);
    }

    if (changeRequired)
        emit dataWritten(serverAddress, newData.registerType(), newData.startAddress(), newData.valueCount());
    return true;
}

///
/// \brief ModbusServer::readData
/// \param newData
/// \param serverAddress
/// \return
///
bool ModbusServer::readData(QModbusDataUnit *newData, int serverAddress) const
{
    if ((!newData) || (!_modbusDataUnitMaps[serverAddress].contains(newData->registerType())))
        return false;

    const QModbusDataUnit &current = _modbusDataUnitMaps[serverAddress].value(newData->registerType());
    if (!current.isValid())
        return false;

    // return entire map for given type
    if (newData->startAddress() < 0) {
        *newData = current;
        return true;
    }

    // check range start is within internal map range
    int internalRangeEndAddress = current.startAddress() + current.valueCount() - 1;
    if (newData->startAddress() < current.startAddress()
        || newData->startAddress() > internalRangeEndAddress) {
        return false;
    }

    // check range end is within internal map range
    const int rangeEndAddress = newData->startAddress() + newData->valueCount() - 1;
    if (rangeEndAddress < current.startAddress() || rangeEndAddress > internalRangeEndAddress)
        return false;

    newData->setValues(current.values().mid(newData->startAddress() - current.startAddress(), newData->valueCount()));
    return true;
}

///
/// \brief ModbusServer::matchingServerAddress
/// \param unitId
/// \return
///
bool ModbusServer::matchingServerAddress(quint8 unitId) const
{
    if(_serverAddresses.contains(unitId))
        return true;

    qCDebug(QT_MODBUS) << "(server) Wrong server unit identifier address, expected one of"
                       << _serverAddresses.values() << "got" << unitId;
    return false;
}

///
/// \brief ModbusServer::processRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processRequest(const QModbusPdu &request, int serverAddress)
{
    switch (request.functionCode()) {
    case QModbusRequest::ReadCoils:
        return processReadCoilsRequest(request, serverAddress);
    case QModbusRequest::ReadDiscreteInputs:
        return processReadDiscreteInputsRequest(request, serverAddress);
    case QModbusRequest::ReadHoldingRegisters:
        return processReadHoldingRegistersRequest(request, serverAddress);
    case QModbusRequest::ReadInputRegisters:
        return processReadInputRegistersRequest(request, serverAddress);
    case QModbusRequest::WriteSingleCoil:
        return processWriteSingleCoilRequest(request, serverAddress);
    case QModbusRequest::WriteSingleRegister:
        return processWriteSingleRegisterRequest(request, serverAddress);
    case QModbusRequest::ReadExceptionStatus:
        return processReadExceptionStatusRequest(request, serverAddress);
    case QModbusRequest::Diagnostics:
        return processDiagnosticsRequest(request, serverAddress);
    case QModbusRequest::GetCommEventCounter:
        return processGetCommEventCounterRequest(request, serverAddress);
    case QModbusRequest::GetCommEventLog:
        return processGetCommEventLogRequest(request, serverAddress);
    case QModbusRequest::WriteMultipleCoils:
        return processWriteMultipleCoilsRequest(request, serverAddress);
    case QModbusRequest::WriteMultipleRegisters:
        return processWriteMultipleRegistersRequest(request, serverAddress);
    case QModbusRequest::ReportServerId:
        return processReportServerIdRequest(request, serverAddress);
    case QModbusRequest::ReadFileRecord:    // TODO: Implement.
    case QModbusRequest::WriteFileRecord:   // TODO: Implement.
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalFunction);
    case QModbusRequest::MaskWriteRegister:
        return processMaskWriteRegisterRequest(request, serverAddress);
    case QModbusRequest::ReadWriteMultipleRegisters:
        return processReadWriteMultipleRegistersRequest(request, serverAddress);
    case QModbusRequest::ReadFifoQueue:
        return processReadFifoQueueRequest(request, serverAddress);
    case QModbusRequest::EncapsulatedInterfaceTransport:
        return processEncapsulatedInterfaceTransportRequest(request, serverAddress);
    default:
        break;
    }
    return processPrivateRequest(request, serverAddress);
}

///
/// \brief ModbusServer::processPrivateRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processPrivateRequest(const QModbusPdu &request, int serverAddress)
{
    return QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::IllegalFunction);
}

///
/// \brief ModbusServer::processReadCoilsRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processReadCoilsRequest(const QModbusRequest &request, int serverAddress)
{
    return readBits(request, QModbusDataUnit::Coils, serverAddress);
}

///
/// \brief ModbusServer::processReadDiscreteInputsRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processReadDiscreteInputsRequest(const QModbusRequest &request, int serverAddress)
{
    return readBits(request, QModbusDataUnit::DiscreteInputs, serverAddress);
}

#define CHECK_SIZE_EQUALS(req) \
do { \
        if (req.dataSize() != QModbusRequest::minimumDataSize(req)) { \
            qDebug(QT_MODBUS) << "(Server) The request's data size does not equal the expected size."; \
            return QModbusExceptionResponse(req.functionCode(), \
                                            QModbusExceptionResponse::IllegalDataValue); \
    } \
} while (0)

#define CHECK_SIZE_LESS_THAN(req) \
    do { \
        if (req.dataSize() < QModbusRequest::minimumDataSize(req)) { \
            qDebug(QT_MODBUS) << "(Server) The request's data size is less than the expected size."; \
            return QModbusExceptionResponse(req.functionCode(), \
                                            QModbusExceptionResponse::IllegalDataValue); \
    } \
} while (0)

///
/// \brief ModbusServer::readBits
/// \param request
/// \param unitType
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::readBits(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType, int serverAddress)
{
    CHECK_SIZE_EQUALS(request);
    quint16 address, count;
    request.decodeData(&address, &count);

    if(!_modbusDataUnitMaps[serverAddress].contains(unitType)) {
        return QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::IllegalFunction);
    }

    if ((count < 0x0001) || (count > 0x07D0)) {
        return QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::IllegalDataValue);
    }

    // Get the requested range out of the registers.
    QModbusDataUnit unit(unitType, address, count);
    if (!data(&unit, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::IllegalDataAddress);
    }

    quint8 byteCount = quint8(count / 8);
    if ((count % 8) != 0) {
        byteCount += 1;
        // If the range is not a multiple of 8, resize.
        unit.setValueCount(byteCount * 8);
    }

    // Using byteCount * 8 so the remaining bits in the last byte are zero
    QBitArray bytes(byteCount * 8);

    address = 0; // The data range now starts with zero.
    for ( ; address < count; ++address)
        bytes.setBit(address, unit.value(address));

    QByteArray payload = QByteArray::fromRawData(bytes.bits(), byteCount);
    payload.prepend(char(byteCount));
    return QModbusResponse(request.functionCode(), payload);
}

///
/// \brief ModbusServer::processReadHoldingRegistersRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processReadHoldingRegistersRequest(const QModbusRequest &request, int serverAddress)
{
    return readBytes(request, QModbusDataUnit::HoldingRegisters, serverAddress);
}

///
/// \brief ModbusServer::processReadInputRegistersRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processReadInputRegistersRequest(const QModbusRequest &request, int serverAddress)
{
    return readBytes(request, QModbusDataUnit::InputRegisters, serverAddress);
}

///
/// \brief ModbusServer::readBytes
/// \param request
/// \param unitType
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::readBytes(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType, int serverAddress)
{
    CHECK_SIZE_EQUALS(request);
    quint16 address, count;
    request.decodeData(&address, &count);

    if(!_modbusDataUnitMaps[serverAddress].contains(unitType)) {
        return QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::IllegalFunction);
    }

    if ((count < 0x0001) || (count > 0x007D)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    // Get the requested range out of the registers.
    QModbusDataUnit unit(unitType, address, count);
    if (!data(&unit, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    return QModbusResponse(request.functionCode(), quint8(count * 2), unit.values());
}

///
/// \brief ModbusServer::processWriteSingleCoilRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processWriteSingleCoilRequest(const QModbusRequest &request, int serverAddress)
{
    return writeSingle(request, QModbusDataUnit::Coils, serverAddress);
}

///
/// \brief ModbusServer::processWriteSingleRegisterRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processWriteSingleRegisterRequest(const QModbusRequest &request, int serverAddress)
{
    return writeSingle(request, QModbusDataUnit::HoldingRegisters, serverAddress);
}

///
/// \brief ModbusServer::writeSingle
/// \param request
/// \param unitType
/// \return
///
QModbusResponse ModbusServer::writeSingle(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType, int serverAddress)
{
    CHECK_SIZE_EQUALS(request);
    quint16 address, value;
    request.decodeData(&address, &value);

    if(!_modbusDataUnitMaps[serverAddress].contains(unitType)) {
        return QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::IllegalFunction);
    }

    if ((unitType == QModbusDataUnit::Coils) && ((value != Coil::Off) && (value != Coil::On))) {
        return QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::IllegalDataValue);
    }

    quint16 reg;   // Get the requested register, but deliberately ignore.
    if (!data(unitType, address, &reg, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::IllegalDataAddress);
    }

    if (!setData(unitType, address, value, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::ServerDeviceFailure);
    }

    return QModbusResponse(request.functionCode(), address, value);
}

///
/// \brief ModbusServer::processReadExceptionStatusRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processReadExceptionStatusRequest(const QModbusRequest &request, int serverAddress)
{
    CHECK_SIZE_EQUALS(request);

    // Get the requested range out of the registers.
    const QVariant tmp = value(QModbusServer::ExceptionStatusOffset, serverAddress);
    if (tmp.isNull() || (!tmp.isValid())) {
        return QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::ServerDeviceFailure);
    }
    const quint16 exceptionStatusOffset = tmp.value<quint16>();
    QModbusDataUnit coils(QModbusDataUnit::Coils, exceptionStatusOffset, 8);
    if (!data(&coils, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    qsizetype address = 0;
    quint8 byte = 0;
    for (int currentBit = 0; currentBit < 8; ++currentBit)
        if (coils.value(address++)) // The padding happens inside value().
            byte |= (1U << currentBit);

    return QModbusResponse(request.functionCode(), byte);
}

///
/// \brief ModbusServer::processDiagnosticsRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processDiagnosticsRequest(const QModbusRequest &request, int serverAddress)
{
#define CHECK_SIZE_AND_CONDITION(req, condition) \
    CHECK_SIZE_EQUALS(req); \
        do { \
            if ((condition)) { \
                return QModbusExceptionResponse(req.functionCode(), \
                                                QModbusExceptionResponse::IllegalDataValue); \
        } \
    } while (0)

        quint16 subFunctionCode, data = 0xffff;
    request.decodeData(&subFunctionCode, &data);

    switch (subFunctionCode) {
    case Diagnostics::ReturnQueryData:
        return QModbusResponse(request.functionCode(), request.data());

    case Diagnostics::RestartCommunicationsOption: {
        CHECK_SIZE_AND_CONDITION(request, ((data != 0xff00) && (data != 0x0000)));
        // Restarts the communication by closing the connection and re-opening. After closing,
        // all communication counters are cleared and the listen only mode set to false. This
        // function is the only way to remotely clear the listen only mode and bring the device
        // back into communication. If data is 0xff00, the event log history is also cleared.
        disconnectDevice();
        if (data == 0xff00)
            _commEventLog.clear();

        resetCommunicationCounters(serverAddress);
        setValue(QModbusServer::ListenOnlyMode, false, serverAddress);
        storeModbusCommEvent(QModbusCommEvent::InitiatedCommunicationRestart);

        if (!connectDevice()) {
            qCWarning(QT_MODBUS) << "(Server) Cannot restart server communication";
            return QModbusExceptionResponse(request.functionCode(),
                                            QModbusExceptionResponse::ServerDeviceFailure);
        }
        return QModbusResponse(request.functionCode(), request.data());
    }   break;

    case Diagnostics::ChangeAsciiInputDelimiter: {
        const QByteArray data = request.data().mid(2, 2);
        CHECK_SIZE_AND_CONDITION(request, (data[1] != 0x00));
        setValue(QModbusServer::AsciiInputDelimiter, data[0], serverAddress);
        return QModbusResponse(request.functionCode(), request.data());
    }   break;

    case Diagnostics::ForceListenOnlyMode:
        CHECK_SIZE_AND_CONDITION(request, (data != 0x0000));
        setValue(QModbusServer::ListenOnlyMode, true, serverAddress);
        storeModbusCommEvent(QModbusCommEvent::EnteredListenOnlyMode);
        return QModbusResponse();

    case Diagnostics::ClearCountersAndDiagnosticRegister:
        CHECK_SIZE_AND_CONDITION(request, (data != 0x0000));
        resetCommunicationCounters(serverAddress);
        setValue(QModbusServer::DiagnosticRegister, 0x0000, serverAddress);
        return QModbusResponse(request.functionCode(), request.data());

    case Diagnostics::ReturnDiagnosticRegister:
    case Diagnostics::ReturnBusMessageCount:
    case Diagnostics::ReturnBusCommunicationErrorCount:
    case Diagnostics::ReturnBusExceptionErrorCount:
    case Diagnostics::ReturnServerMessageCount:
    case Diagnostics::ReturnServerNoResponseCount:
    case Diagnostics::ReturnServerNAKCount:
    case Diagnostics::ReturnServerBusyCount:
    case Diagnostics::ReturnBusCharacterOverrunCount:
        CHECK_SIZE_AND_CONDITION(request, (data != 0x0000));
        return QModbusResponse(request.functionCode(), subFunctionCode,
                               _counters[serverAddress][static_cast<Counter> (subFunctionCode)]);

    case Diagnostics::ClearOverrunCounterAndFlag: {
        CHECK_SIZE_AND_CONDITION(request, (data != 0x0000));
        _counters[serverAddress][Diagnostics::ReturnBusCharacterOverrunCount] = 0;
        quint16 reg = value(QModbusServer::DiagnosticRegister, serverAddress).value<quint16>();
        setValue(QModbusServer::DiagnosticRegister, reg &~ 1, serverAddress); // clear first bit
        return QModbusResponse(request.functionCode(), request.data());
    }
    }
    return QModbusExceptionResponse(request.functionCode(),
                                    QModbusExceptionResponse::IllegalFunction);

#undef CHECK_SIZE_AND_CONDITION
}

///
/// \brief ModbusServer::processGetCommEventCounterRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processGetCommEventCounterRequest(const QModbusRequest &request, int serverAddress)
{
    CHECK_SIZE_EQUALS(request);
    const QVariant tmp = value(QModbusServer::DeviceBusy, serverAddress);
    if (tmp.isNull() || (!tmp.isValid())) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    const quint16 deviceBusy = tmp.value<quint16>();
    return QModbusResponse(request.functionCode(), deviceBusy, _counters[serverAddress][Counter::CommEvent]);
}

///
/// \brief ModbusServer::processGetCommEventLogRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processGetCommEventLogRequest(const QModbusRequest &request,int serverAddress)
{
    CHECK_SIZE_EQUALS(request);
    const QVariant tmp = value(QModbusServer::DeviceBusy, serverAddress);
    if (tmp.isNull() || (!tmp.isValid())) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    const quint16 deviceBusy = tmp.value<quint16>();

    QVector<quint8> eventLog(int(_commEventLog.size()));
    std::copy(_commEventLog.cbegin(), _commEventLog.cend(), eventLog.begin());

    // 6 -> 3 x 2 Bytes (Status, Event Count and Message Count)
    return QModbusResponse(request.functionCode(), quint8(eventLog.size() + 6), deviceBusy,
                           _counters[serverAddress][Counter::CommEvent], _counters[serverAddress][Counter::BusMessage], eventLog);
}

///
/// \brief ModbusServer::processWriteMultipleCoilsRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processWriteMultipleCoilsRequest(const QModbusRequest &request, int serverAddress)
{
    CHECK_SIZE_LESS_THAN(request);
    quint16 address, numberOfCoils;
    quint8 byteCount;
    request.decodeData(&address, &numberOfCoils, &byteCount);

    // byte count does not match number of data bytes following
    if (byteCount != (request.dataSize() - 5 )) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    quint8 expectedBytes = numberOfCoils / 8;
    if ((numberOfCoils % 8) != 0)
        expectedBytes += 1;

    if ((numberOfCoils < 0x0001) || (numberOfCoils > 0x07B0) || (expectedBytes != byteCount)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    // Get the requested range out of the registers.
    QModbusDataUnit coils(QModbusDataUnit::Coils, address, numberOfCoils);
    if (!data(&coils, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    QList<quint8> bytes;
    const QByteArray payload = request.data().mid(5);
    for (qint32 i = payload.size() - 1; i >= 0; --i)
        bytes.append(quint8(payload[i]));

    // Since we picked the coils at start address, data
    // range is numberOfCoils and therefore index too.
    quint16 coil = numberOfCoils;
    qint32 currentBit = 8 - ((byteCount * 8) - numberOfCoils);
    for (quint8 currentByte : std::as_const(bytes)) {
        for (currentBit -= 1; currentBit >= 0; --currentBit)
            coils.setValue(--coil, currentByte & (1U << currentBit) ? 1 : 0);
        currentBit = 8;
    }

    if (!setData(coils, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }

    return QModbusResponse(request.functionCode(), address, numberOfCoils);
}

///
/// \brief ModbusServer::processWriteMultipleRegistersRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processWriteMultipleRegistersRequest(const QModbusRequest &request, int serverAddress)
{
    CHECK_SIZE_LESS_THAN(request);
    quint16 address, numberOfRegisters;
    quint8 byteCount;
    request.decodeData(&address, &numberOfRegisters, &byteCount);

    // byte count does not match number of data bytes following or register count
    if ((byteCount != (request.dataSize() - 5 )) || (byteCount != (numberOfRegisters * 2))) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    if ((numberOfRegisters < 0x0001) || (numberOfRegisters > 0x007B)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    // Get the requested range out of the registers.
    QModbusDataUnit registers(QModbusDataUnit::HoldingRegisters, address, numberOfRegisters);
    if (!data(&registers, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    const QByteArray pduData = request.data().remove(0,5);
    QDataStream stream(pduData);

    QVector<quint16> values;
    quint16 tmp;
    for (int i = 0; i < numberOfRegisters; i++) {
        stream >> tmp;
        values.append(tmp);
    }

    registers.setValues(values);

    if (!setData(registers, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }

    return QModbusResponse(request.functionCode(), address, numberOfRegisters);
}

///
/// \brief ModbusServer::processReportServerIdRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processReportServerIdRequest(const QModbusRequest &request, int serverAddress)
{
    CHECK_SIZE_EQUALS(request);

    QByteArray data;
    QVariant tmp = value(QModbusServer::ServerIdentifier, serverAddress);
    if (tmp.isNull() || (!tmp.isValid())) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    data.append(tmp.value<quint8>());

    tmp = value(QModbusServer::RunIndicatorStatus, serverAddress);
    if (tmp.isNull() || (!tmp.isValid())) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    data.append(tmp.value<quint8>());

    tmp = value(QModbusServer::AdditionalData, serverAddress);
    if (!tmp.isNull() && tmp.isValid())
        data.append(tmp.toByteArray());

    data.prepend(data.size()); // byte count
    return QModbusResponse(request.functionCode(), data);
}

///
/// \brief ModbusServer::processMaskWriteRegisterRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processMaskWriteRegisterRequest(const QModbusRequest &request, int serverAddress)
{
    CHECK_SIZE_EQUALS(request);
    quint16 address, andMask, orMask;
    request.decodeData(&address, &andMask, &orMask);

    quint16 reg;
    if (!data(QModbusDataUnit::HoldingRegisters, address, &reg, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    const quint16 result = (reg & andMask) | (orMask & (~ andMask));
    if (!setData(QModbusDataUnit::HoldingRegisters, address, result, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    return QModbusResponse(request.functionCode(), request.data());
}

///
/// \brief ModbusServer::processReadWriteMultipleRegistersRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processReadWriteMultipleRegistersRequest(const QModbusRequest &request, int serverAddress)
{
    CHECK_SIZE_LESS_THAN(request);
    quint16 readStartAddress, readQuantity, writeStartAddress, writeQuantity;
    quint8 byteCount;
    request.decodeData(&readStartAddress, &readQuantity,
                       &writeStartAddress, &writeQuantity, &byteCount);

    // byte count does not match number of data bytes following or register count
    if ((byteCount != (request.dataSize() - 9 )) || (byteCount != (writeQuantity * 2))) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    if ((readQuantity < 0x0001) || (readQuantity > 0x007B)
        || (writeQuantity < 0x0001) || (writeQuantity > 0x0079)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    // According to spec, write operation is executed before the read operation
    // Get the requested range out of the registers.
    QModbusDataUnit writeRegisters(QModbusDataUnit::HoldingRegisters, writeStartAddress,
                                   writeQuantity);
    if (!data(&writeRegisters, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    const QByteArray pduData = request.data().remove(0,9);
    QDataStream stream(pduData);

    QVector<quint16> values;
    quint16 tmp;
    for (int i = 0; i < writeQuantity; i++) {
        stream >> tmp;
        values.append(tmp);
    }

    writeRegisters.setValues(values);

    if (!setData(writeRegisters, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }

    // Get the requested range out of the registers.
    QModbusDataUnit readRegisters(QModbusDataUnit::HoldingRegisters, readStartAddress,
                                  readQuantity);
    if (!data(&readRegisters, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    return QModbusResponse(request.functionCode(), quint8(readQuantity * 2),
                           readRegisters.values());
}

///
/// \brief ModbusServer::processReadFifoQueueRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processReadFifoQueueRequest(const QModbusRequest &request, int serverAddress)
{
    CHECK_SIZE_LESS_THAN(request);
    quint16 address;
    request.decodeData(&address);

    quint16 fifoCount;
    if (!data(QModbusDataUnit::HoldingRegisters, address, &fifoCount, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    if (fifoCount > 31u) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    QModbusDataUnit fifoRegisters(QModbusDataUnit::HoldingRegisters, address + 1u, fifoCount);
    if (!data(&fifoRegisters, serverAddress)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    return QModbusResponse(request.functionCode(), quint16((fifoCount * 2) + 2u), fifoCount,
                           fifoRegisters.values());
}

///
/// \brief ModbusServer::processEncapsulatedInterfaceTransportRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusServer::processEncapsulatedInterfaceTransportRequest(const QModbusRequest &request, int serverAddress)
{
    CHECK_SIZE_LESS_THAN(request);
    quint8 MEIType;
    request.decodeData(&MEIType);

    switch (MEIType) {
    case EncapsulatedInterfaceTransport::CanOpenGeneralReference:
        break;
    case EncapsulatedInterfaceTransport::ReadDeviceIdentification: {
        if (request.dataSize() != 3u) {
            return QModbusExceptionResponse(request.functionCode(),
                                            QModbusExceptionResponse::IllegalDataValue);
        }

        const QVariant tmp = value(QModbusServer::DeviceIdentification, serverAddress);
        if (tmp.isNull() || (!tmp.isValid())) {
            // TODO: Is this correct?
            return QModbusExceptionResponse(request.functionCode(),
                                            QModbusExceptionResponse::ServerDeviceFailure);
        }

        QModbusDeviceIdentification objectPool = tmp.value<QModbusDeviceIdentification>();
        if (!objectPool.isValid()) {
            // TODO: Is this correct?
            return QModbusExceptionResponse(request.functionCode(),
                                            QModbusExceptionResponse::ServerDeviceFailure);
        }

        quint8 readDeviceIdCode, objectId;
        request.decodeData(&MEIType, &readDeviceIdCode, &objectId);
        if (!objectPool.contains(objectId)) {
            // Individual access requires the object Id to be present, so we will always fail.
            // For all other cases we will reevaluate object Id after we reset it as per spec.
            objectId = QModbusDeviceIdentification::VendorNameObjectId;
            if (readDeviceIdCode == QModbusDeviceIdentification::IndividualReadDeviceIdCode
                || !objectPool.contains(objectId)) {
                return QModbusExceptionResponse(request.functionCode(),
                                                QModbusExceptionResponse::IllegalDataAddress);
            }
        }

        auto payload = [MEIType, readDeviceIdCode, objectId, objectPool](int lastObjectId) {
            // TODO: Take conformity level into account.
            QByteArray payload(6, Qt::Uninitialized);
            payload[0] = MEIType;
            payload[1] = readDeviceIdCode;
            payload[2] = quint8(objectPool.conformityLevel());
            payload[3] = quint8(0x00); // no more follows
            payload[4] = quint8(0x00); // next object id
            payload[5] = quint8(0x00); // number of objects

            const QList<int> objectIds = objectPool.objectIds();
            for (int id : objectIds) {
                if (id < objectId)
                    continue;
                if (id > lastObjectId)
                    break;
                const QByteArray object = objectPool.value(id);
                QByteArray objectData(2, Qt::Uninitialized);
                objectData[0] = id;
                objectData[1] = quint8(object.size());
                objectData += object;
                if (payload.size() + objectData.size() > 253) {
                    payload[3] = char(0xff); // more follows
                    payload[4] = id; // next object id
                    break;
                }
                payload.append(objectData);
                payload[5] = payload[5] + 1u; // number of objects
            }
            return payload;
        };

        switch (readDeviceIdCode) {
        case QModbusDeviceIdentification::BasicReadDeviceIdCode:
            // TODO: How to handle a valid Id <> VendorName ... MajorMinorRevision
            return QModbusResponse(request.functionCode(),
                                   payload(QModbusDeviceIdentification::MajorMinorRevisionObjectId));
        case QModbusDeviceIdentification::RegularReadDeviceIdCode:
            // TODO: How to handle a valid Id <> VendorUrl ... UserApplicationName
            return QModbusResponse(request.functionCode(),
                                   payload(QModbusDeviceIdentification::UserApplicationNameObjectId));
        case QModbusDeviceIdentification::ExtendedReadDeviceIdCode:
            // TODO: How to handle a valid Id < ProductDependent
            return QModbusResponse(request.functionCode(),
                                   payload(QModbusDeviceIdentification::UndefinedObjectId));
        case QModbusDeviceIdentification::IndividualReadDeviceIdCode: {
            // TODO: Take conformity level into account.
            const QByteArray object = objectPool.value(objectId);
            QByteArray header(8, Qt::Uninitialized);
            header[0] = MEIType;
            header[1] = readDeviceIdCode;
            header[2] = quint8(objectPool.conformityLevel());
            header[3] = quint8(0x00); // no more follows
            header[4] = quint8(0x00); // next object id
            header[5] = quint8(0x01); // number of objects
            header[6] = objectId;
            header[7] = quint8(object.size());
            return QModbusResponse(request.functionCode(), QByteArray(header + object));
        }
        default:
            return QModbusExceptionResponse(request.functionCode(),
                                            QModbusExceptionResponse::IllegalDataValue);
        }
    }   break;
    }
    return QModbusExceptionResponse(request.functionCode(),
                                    QModbusExceptionResponse::IllegalFunction);
}

///
/// \brief ModbusServer::storeModbusCommEvent
/// \param eventByte
///
void ModbusServer::storeModbusCommEvent(const QModbusCommEvent &eventByte)
{
    // Inserts an event byte at the start of the event log. If the event log
    // is already full, the byte at the end of the log will be removed. The
    // event log size is 64 bytes, starting at index 0.
    _commEventLog.push_front(eventByte);
    if (_commEventLog.size() > 64)
        _commEventLog.pop_back();
}

#undef CHECK_SIZE_EQUALS
#undef CHECK_SIZE_LESS_THAN
