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
    : QModbusServer(parent)
    ,_counters()
{
}

///
/// \brief ModbusServer::processRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processRequest(const QModbusPdu &request)
{
    switch (request.functionCode()) {
    case QModbusRequest::ReadCoils:
        return processReadCoilsRequest(request);
    case QModbusRequest::ReadDiscreteInputs:
        return processReadDiscreteInputsRequest(request);
    case QModbusRequest::ReadHoldingRegisters:
        return processReadHoldingRegistersRequest(request);
    case QModbusRequest::ReadInputRegisters:
        return processReadInputRegistersRequest(request);
    case QModbusRequest::WriteSingleCoil:
        return processWriteSingleCoilRequest(request);
    case QModbusRequest::WriteSingleRegister:
        return processWriteSingleRegisterRequest(request);
    case QModbusRequest::ReadExceptionStatus:
        return processReadExceptionStatusRequest(request);
    case QModbusRequest::Diagnostics:
        return processDiagnosticsRequest(request);
    case QModbusRequest::GetCommEventCounter:
        return processGetCommEventCounterRequest(request);
    case QModbusRequest::GetCommEventLog:
        return processGetCommEventLogRequest(request);
    case QModbusRequest::WriteMultipleCoils:
        return processWriteMultipleCoilsRequest(request);
    case QModbusRequest::WriteMultipleRegisters:
        return processWriteMultipleRegistersRequest(request);
    case QModbusRequest::ReportServerId:
        return processReportServerIdRequest(request);
    case QModbusRequest::ReadFileRecord:    // TODO: Implement.
    case QModbusRequest::WriteFileRecord:   // TODO: Implement.
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalFunction);
    case QModbusRequest::MaskWriteRegister:
        return processMaskWriteRegisterRequest(request);
    case QModbusRequest::ReadWriteMultipleRegisters:
        return processReadWriteMultipleRegistersRequest(request);
    case QModbusRequest::ReadFifoQueue:
        return processReadFifoQueueRequest(request);
    case QModbusRequest::EncapsulatedInterfaceTransport:
        return processEncapsulatedInterfaceTransportRequest(request);
    default:
        break;
    }
    return QModbusServer::processPrivateRequest(request);
}

///
/// \brief ModbusServer::processReadCoilsRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processReadCoilsRequest(const QModbusRequest &request)
{
    return readBits(request, QModbusDataUnit::Coils);
}

///
/// \brief ModbusServer::processReadDiscreteInputsRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processReadDiscreteInputsRequest(const QModbusRequest &request)
{
    return readBits(request, QModbusDataUnit::DiscreteInputs);
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
/// \return
///
QModbusResponse ModbusServer::readBits(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType)
{
    CHECK_SIZE_EQUALS(request);
    quint16 address, count;
    request.decodeData(&address, &count);

    if ((count < 0x0001) || (count > 0x07D0)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    // Get the requested range out of the registers.
    QModbusDataUnit unit(unitType, address, count);
    if (!data(&unit)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
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
/// \return
///
QModbusResponse ModbusServer::processReadHoldingRegistersRequest(const QModbusRequest &request)
{
    return readBytes(request, QModbusDataUnit::HoldingRegisters);
}

///
/// \brief ModbusServer::processReadInputRegistersRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processReadInputRegistersRequest(const QModbusRequest &request)
{
    return readBytes(request, QModbusDataUnit::InputRegisters);
}

///
/// \brief ModbusServer::readBytes
/// \param request
/// \param unitType
/// \return
///
QModbusResponse ModbusServer::readBytes(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType)
{
    CHECK_SIZE_EQUALS(request);
    quint16 address, count;
    request.decodeData(&address, &count);

    if ((count < 0x0001) || (count > 0x007D)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    // Get the requested range out of the registers.
    QModbusDataUnit unit(unitType, address, count);
    if (!data(&unit)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    return QModbusResponse(request.functionCode(), quint8(count * 2), unit.values());
}

///
/// \brief ModbusServer::processWriteSingleCoilRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processWriteSingleCoilRequest(const QModbusRequest &request)
{
    return writeSingle(request, QModbusDataUnit::Coils);
}

///
/// \brief ModbusServer::processWriteSingleRegisterRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processWriteSingleRegisterRequest(const QModbusRequest &request)
{
    return writeSingle(request, QModbusDataUnit::HoldingRegisters);
}

///
/// \brief ModbusServer::writeSingle
/// \param request
/// \param unitType
/// \return
///
QModbusResponse ModbusServer::writeSingle(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType)
{
    CHECK_SIZE_EQUALS(request);
    quint16 address, value;
    request.decodeData(&address, &value);

    if ((unitType == QModbusDataUnit::Coils) && ((value != Coil::Off) && (value != Coil::On))) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    quint16 reg;   // Get the requested register, but deliberately ignore.
    if (!data(unitType, address, &reg)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    if (!setData(unitType, address, value)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }

    return QModbusResponse(request.functionCode(), address, value);
}

///
/// \brief ModbusServer::processReadExceptionStatusRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processReadExceptionStatusRequest(const QModbusRequest &request)
{
    CHECK_SIZE_EQUALS(request);

    // Get the requested range out of the registers.
    const QVariant tmp = value(QModbusServer::ExceptionStatusOffset);
    if (tmp.isNull() || (!tmp.isValid())) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    const quint16 exceptionStatusOffset = tmp.value<quint16>();
    QModbusDataUnit coils(QModbusDataUnit::Coils, exceptionStatusOffset, 8);
    if (!data(&coils)) {
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
/// \return
///
QModbusResponse ModbusServer::processDiagnosticsRequest(const QModbusRequest &request)
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

        resetCommunicationCounters();
        setValue(QModbusServer::ListenOnlyMode, false);
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
        setValue(QModbusServer::AsciiInputDelimiter, data[0]);
        return QModbusResponse(request.functionCode(), request.data());
    }   break;

    case Diagnostics::ForceListenOnlyMode:
        CHECK_SIZE_AND_CONDITION(request, (data != 0x0000));
        setValue(QModbusServer::ListenOnlyMode, true);
        storeModbusCommEvent(QModbusCommEvent::EnteredListenOnlyMode);
        return QModbusResponse();

    case Diagnostics::ClearCountersAndDiagnosticRegister:
        CHECK_SIZE_AND_CONDITION(request, (data != 0x0000));
        resetCommunicationCounters();
        setValue(QModbusServer::DiagnosticRegister, 0x0000);
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
                               _counters[static_cast<Counter> (subFunctionCode)]);

    case Diagnostics::ClearOverrunCounterAndFlag: {
        CHECK_SIZE_AND_CONDITION(request, (data != 0x0000));
        _counters[Diagnostics::ReturnBusCharacterOverrunCount] = 0;
        quint16 reg = value(QModbusServer::DiagnosticRegister).value<quint16>();
        setValue(QModbusServer::DiagnosticRegister, reg &~ 1); // clear first bit
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
/// \return
///
QModbusResponse ModbusServer::processGetCommEventCounterRequest(const QModbusRequest &request)
{
    CHECK_SIZE_EQUALS(request);
    const QVariant tmp = value(QModbusServer::DeviceBusy);
    if (tmp.isNull() || (!tmp.isValid())) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    const quint16 deviceBusy = tmp.value<quint16>();
    return QModbusResponse(request.functionCode(), deviceBusy, _counters[Counter::CommEvent]);
}

///
/// \brief ModbusServer::processGetCommEventLogRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processGetCommEventLogRequest(const QModbusRequest &request)
{
    CHECK_SIZE_EQUALS(request);
    const QVariant tmp = value(QModbusServer::DeviceBusy);
    if (tmp.isNull() || (!tmp.isValid())) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    const quint16 deviceBusy = tmp.value<quint16>();

    QList<quint8> eventLog(int(_commEventLog.size()));
    std::copy(_commEventLog.cbegin(), _commEventLog.cend(), eventLog.begin());

    // 6 -> 3 x 2 Bytes (Status, Event Count and Message Count)
    return QModbusResponse(request.functionCode(), quint8(eventLog.size() + 6), deviceBusy,
                           _counters[Counter::CommEvent], _counters[Counter::BusMessage], eventLog);
}

///
/// \brief ModbusServer::processWriteMultipleCoilsRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processWriteMultipleCoilsRequest(const QModbusRequest &request)
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
    if (!data(&coils)) {
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

    if (!setData(coils)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }

    return QModbusResponse(request.functionCode(), address, numberOfCoils);
}

///
/// \brief ModbusServer::processWriteMultipleRegistersRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processWriteMultipleRegistersRequest(const QModbusRequest &request)
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
    if (!data(&registers)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    const QByteArray pduData = request.data().remove(0,5);
    QDataStream stream(pduData);

    QList<quint16> values;
    quint16 tmp;
    for (int i = 0; i < numberOfRegisters; i++) {
        stream >> tmp;
        values.append(tmp);
    }

    registers.setValues(values);

    if (!setData(registers)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }

    return QModbusResponse(request.functionCode(), address, numberOfRegisters);
}

///
/// \brief ModbusServer::processReportServerIdRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processReportServerIdRequest(const QModbusRequest &request)
{
    CHECK_SIZE_EQUALS(request);

    QByteArray data;
    QVariant tmp = value(QModbusServer::ServerIdentifier);
    if (tmp.isNull() || (!tmp.isValid())) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    data.append(tmp.value<quint8>());

    tmp = value(QModbusServer::RunIndicatorStatus);
    if (tmp.isNull() || (!tmp.isValid())) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    data.append(tmp.value<quint8>());

    tmp = value(QModbusServer::AdditionalData);
    if (!tmp.isNull() && tmp.isValid())
        data.append(tmp.toByteArray());

    data.prepend(data.size()); // byte count
    return QModbusResponse(request.functionCode(), data);
}

///
/// \brief ModbusServer::processMaskWriteRegisterRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processMaskWriteRegisterRequest(const QModbusRequest &request)
{
    CHECK_SIZE_EQUALS(request);
    quint16 address, andMask, orMask;
    request.decodeData(&address, &andMask, &orMask);

    quint16 reg;
    if (!data(QModbusDataUnit::HoldingRegisters, address, &reg)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    const quint16 result = (reg & andMask) | (orMask & (~ andMask));
    if (!setData(QModbusDataUnit::HoldingRegisters, address, result)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }
    return QModbusResponse(request.functionCode(), request.data());
}

///
/// \brief ModbusServer::processReadWriteMultipleRegistersRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processReadWriteMultipleRegistersRequest(const QModbusRequest &request)
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
    if (!data(&writeRegisters)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    const QByteArray pduData = request.data().remove(0,9);
    QDataStream stream(pduData);

    QList<quint16> values;
    quint16 tmp;
    for (int i = 0; i < writeQuantity; i++) {
        stream >> tmp;
        values.append(tmp);
    }

    writeRegisters.setValues(values);

    if (!setData(writeRegisters)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::ServerDeviceFailure);
    }

    // Get the requested range out of the registers.
    QModbusDataUnit readRegisters(QModbusDataUnit::HoldingRegisters, readStartAddress,
                                  readQuantity);
    if (!data(&readRegisters)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    return QModbusResponse(request.functionCode(), quint8(readQuantity * 2),
                           readRegisters.values());
}

///
/// \brief ModbusServer::processReadFifoQueueRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processReadFifoQueueRequest(const QModbusRequest &request)
{
    CHECK_SIZE_LESS_THAN(request);
    quint16 address;
    request.decodeData(&address);

    quint16 fifoCount;
    if (!data(QModbusDataUnit::HoldingRegisters, address, &fifoCount)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    if (fifoCount > 31u) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataValue);
    }

    QModbusDataUnit fifoRegisters(QModbusDataUnit::HoldingRegisters, address + 1u, fifoCount);
    if (!data(&fifoRegisters)) {
        return QModbusExceptionResponse(request.functionCode(),
                                        QModbusExceptionResponse::IllegalDataAddress);
    }

    return QModbusResponse(request.functionCode(), quint16((fifoCount * 2) + 2u), fifoCount,
                           fifoRegisters.values());
}

///
/// \brief ModbusServer::processEncapsulatedInterfaceTransportRequest
/// \param request
/// \return
///
QModbusResponse ModbusServer::processEncapsulatedInterfaceTransportRequest(const QModbusRequest &request)
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

        const QVariant tmp = value(QModbusServer::DeviceIdentification);
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
