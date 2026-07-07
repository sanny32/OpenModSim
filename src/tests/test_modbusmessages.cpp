// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbusmessages.cpp
/// \brief Unit tests for the per-function-code Modbus message classes.
///

#include <QModbusRequest>
#include <QModbusResponse>
#include <QTest>

#include "modbusmessages.h"

namespace {

const QDateTime kTs = QDateTime::currentDateTime();
constexpr auto kProto = ModbusMessage::Tcp;

QByteArray be16(quint16 value)
{
    return QByteArray(1, char(value >> 8)) + QByteArray(1, char(value & 0xFF));
}

QModbusRequest request(QModbusPdu::FunctionCode code, const QByteArray& data)
{
    return QModbusRequest(code, data);
}

QModbusResponse response(QModbusPdu::FunctionCode code, const QByteArray& data)
{
    return QModbusResponse(code, data);
}

}

class TestModbusMessages : public QObject
{
    Q_OBJECT

private slots:
    void readCoils();
    void readDiscreteInputs();
    void readHoldingRegisters();
    void readInputRegisters();
    void writeSingleCoil();
    void writeSingleRegister();
    void writeMultipleCoils();
    void writeMultipleRegisters();
    void readExceptionStatus();
    void diagnostics();
    void getCommEventCounter();
    void getCommEventLog();
    void reportServerId();
    void readFileRecord();
    void writeFileRecord();
    void maskWriteRegister();
    void readWriteMultipleRegisters();
    void readFifoQueue();
};

void TestModbusMessages::readCoils()
{
    ReadCoilsRequest req(request(QModbusPdu::ReadCoils, be16(0x0010) + be16(5)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.startAddress(), quint16(0x0010));
    QCOMPARE(req.length(), quint16(5));

    ReadCoilsRequest maxLength(request(QModbusPdu::ReadCoils, be16(0) + be16(0x07D0)), kProto, 1, 0, kTs);
    QVERIFY(maxLength.isValid());

    ReadCoilsRequest bad(request(QModbusPdu::ReadCoils, be16(0) + be16(0)), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    ReadCoilsRequest tooLong(request(QModbusPdu::ReadCoils, be16(0) + be16(0x07D1)), kProto, 1, 0, kTs);
    QVERIFY(!tooLong.isValid());

    ReadCoilsResponse resp(response(QModbusPdu::ReadCoils, QByteArray::fromHex("02FF01")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(2));
    QCOMPARE(resp.coilStatus(), QByteArray::fromHex("FF01"));

    ReadCoilsResponse padded(response(QModbusPdu::ReadCoils, QByteArray::fromHex("01A55A")), kProto, 1, 0, kTs);
    QVERIFY(padded.isValid());

    ReadCoilsResponse zeroBytes(response(QModbusPdu::ReadCoils, QByteArray::fromHex("00")), kProto, 1, 0, kTs);
    QVERIFY(!zeroBytes.isValid());

    ReadCoilsResponse shortResp(response(QModbusPdu::ReadCoils, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(!shortResp.isValid());

    ReadCoilsResponse badResp(response(QModbusPdu::ReadCoils, QByteArray::fromHex("05FF")), kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());
}

void TestModbusMessages::readDiscreteInputs()
{
    ReadDiscreteInputsRequest req(request(QModbusPdu::ReadDiscreteInputs, be16(7) + be16(3)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.startAddress(), quint16(7));
    QCOMPARE(req.length(), quint16(3));

    ReadDiscreteInputsRequest maxLength(request(QModbusPdu::ReadDiscreteInputs, be16(7) + be16(0x07D0)), kProto, 1, 0, kTs);
    QVERIFY(maxLength.isValid());

    ReadDiscreteInputsRequest zero(request(QModbusPdu::ReadDiscreteInputs, be16(7) + be16(0)), kProto, 1, 0, kTs);
    QVERIFY(!zero.isValid());

    ReadDiscreteInputsRequest tooLong(request(QModbusPdu::ReadDiscreteInputs, be16(7) + be16(0x07D1)), kProto, 1, 0, kTs);
    QVERIFY(!tooLong.isValid());

    ReadDiscreteInputsResponse resp(response(QModbusPdu::ReadDiscreteInputs, QByteArray::fromHex("0105")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(1));
    QCOMPARE(resp.inputStatus(), QByteArray::fromHex("05"));

    ReadDiscreteInputsResponse padded(response(QModbusPdu::ReadDiscreteInputs, QByteArray::fromHex("0155AA")), kProto, 1, 0, kTs);
    QVERIFY(padded.isValid());

    ReadDiscreteInputsResponse zeroBytes(response(QModbusPdu::ReadDiscreteInputs, QByteArray::fromHex("00")), kProto, 1, 0, kTs);
    QVERIFY(!zeroBytes.isValid());

    ReadDiscreteInputsResponse shortResp(response(QModbusPdu::ReadDiscreteInputs, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(!shortResp.isValid());

    ReadDiscreteInputsResponse badResp(response(QModbusPdu::ReadDiscreteInputs, QByteArray::fromHex("03FF")), kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());
}

void TestModbusMessages::readHoldingRegisters()
{
    ReadHoldingRegistersRequest req(request(QModbusPdu::ReadHoldingRegisters, be16(0x0100) + be16(2)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.startAddress(), quint16(0x0100));
    QCOMPARE(req.length(), quint16(2));

    ReadHoldingRegistersRequest maxLength(request(QModbusPdu::ReadHoldingRegisters, be16(0) + be16(0x7D)), kProto, 1, 0, kTs);
    QVERIFY(maxLength.isValid());

    ReadHoldingRegistersRequest bad(request(QModbusPdu::ReadHoldingRegisters, be16(0) + be16(0x7E)), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    ReadHoldingRegistersRequest zero(request(QModbusPdu::ReadHoldingRegisters, be16(0) + be16(0)), kProto, 1, 0, kTs);
    QVERIFY(!zero.isValid());

    ReadHoldingRegistersRequest emptyReq(request(QModbusPdu::ReadHoldingRegisters, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(!emptyReq.isValid());

    ReadHoldingRegistersResponse ok(response(QModbusPdu::ReadHoldingRegisters, QByteArray::fromHex("0400010002")), kProto, 1, 0, kTs);
    QVERIFY(ok.isValid());
    QCOMPARE(ok.byteCount(), quint8(4));
    QCOMPARE(ok.registerValue(), QByteArray::fromHex("00010002"));

    ReadHoldingRegistersResponse emptyResp(response(QModbusPdu::ReadHoldingRegisters, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(!emptyResp.isValid());

    ReadHoldingRegistersResponse zeroBytes(response(QModbusPdu::ReadHoldingRegisters, QByteArray::fromHex("00")), kProto, 1, 0, kTs);
    QVERIFY(!zeroBytes.isValid());

    ReadHoldingRegistersResponse mismatch(response(QModbusPdu::ReadHoldingRegisters, QByteArray::fromHex("040001")),
                                          kProto, 1, 0, kTs);
    QVERIFY(!mismatch.isValid());

    QModbusExceptionResponse exception(QModbusPdu::ReadHoldingRegisters,
                                       QModbusExceptionResponse::IllegalDataAddress);
    ReadHoldingRegistersResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::readInputRegisters()
{
    ReadInputRegistersRequest req(request(QModbusPdu::ReadInputRegisters, be16(0) + be16(1)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.length(), quint16(1));

    ReadInputRegistersRequest maxLength(request(QModbusPdu::ReadInputRegisters, be16(0) + be16(0x7D)), kProto, 1, 0, kTs);
    QVERIFY(maxLength.isValid());

    ReadInputRegistersRequest zero(request(QModbusPdu::ReadInputRegisters, be16(0) + be16(0)), kProto, 1, 0, kTs);
    QVERIFY(!zero.isValid());

    ReadInputRegistersRequest tooLong(request(QModbusPdu::ReadInputRegisters, be16(0) + be16(0x007E)), kProto, 1, 0, kTs);
    QVERIFY(!tooLong.isValid());

    ReadInputRegistersRequest emptyReq(request(QModbusPdu::ReadInputRegisters, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(!emptyReq.isValid());

    ReadInputRegistersResponse resp(response(QModbusPdu::ReadInputRegisters, QByteArray::fromHex("020ABC")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(2));
    QCOMPARE(resp.registerValue(), QByteArray::fromHex("0ABC"));

    ReadInputRegistersResponse emptyResp(response(QModbusPdu::ReadInputRegisters, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(!emptyResp.isValid());

    ReadInputRegistersResponse zeroBytes(response(QModbusPdu::ReadInputRegisters, QByteArray::fromHex("00")), kProto, 1, 0, kTs);
    QVERIFY(!zeroBytes.isValid());

    ReadInputRegistersResponse badResp(response(QModbusPdu::ReadInputRegisters, QByteArray::fromHex("040ABC")), kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());

    QModbusExceptionResponse exception(QModbusPdu::ReadInputRegisters,
                                       QModbusExceptionResponse::IllegalDataValue);
    ReadInputRegistersResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::writeSingleCoil()
{
    WriteSingleCoilRequest on(request(QModbusPdu::WriteSingleCoil, be16(3) + be16(0xFF00)), kProto, 1, 0, kTs);
    QVERIFY(on.isValid());
    QCOMPARE(on.address(), quint16(3));
    QCOMPARE(on.value(), quint16(0xFF00));

    WriteSingleCoilRequest off(request(QModbusPdu::WriteSingleCoil, be16(3) + be16(0)), kProto, 1, 0, kTs);
    QVERIFY(off.isValid());

    WriteSingleCoilRequest bad(request(QModbusPdu::WriteSingleCoil, be16(3) + be16(0x1234)), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    WriteSingleCoilResponse resp(response(QModbusPdu::WriteSingleCoil, be16(3) + be16(0)), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.value(), quint16(0));

    WriteSingleCoilResponse onResp(response(QModbusPdu::WriteSingleCoil, be16(3) + be16(0xFF00)), kProto, 1, 0, kTs);
    QVERIFY(onResp.isValid());

    WriteSingleCoilResponse badResp(response(QModbusPdu::WriteSingleCoil, be16(3) + be16(0x1234)), kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());
}

void TestModbusMessages::writeSingleRegister()
{
    WriteSingleRegisterRequest req(request(QModbusPdu::WriteSingleRegister, be16(7) + be16(0xABCD)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.address(), quint16(7));
    QCOMPARE(req.value(), quint16(0xABCD));

    WriteSingleRegisterResponse resp(response(QModbusPdu::WriteSingleRegister, be16(7) + be16(0xABCD)), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.address(), quint16(7));
    QCOMPARE(resp.value(), quint16(0xABCD));

    WriteSingleRegisterRequest badReq(request(QModbusPdu::WriteSingleRegister, be16(7)), kProto, 1, 0, kTs);
    QVERIFY(!badReq.isValid());

    WriteSingleRegisterResponse badResp(response(QModbusPdu::WriteSingleRegister, be16(7)), kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());
}

void TestModbusMessages::writeMultipleCoils()
{
    WriteMultipleCoilsRequest req(request(QModbusPdu::WriteMultipleCoils,
                                          be16(0x0013) + be16(10) + QByteArray::fromHex("02CD01")), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.startAddress(), quint16(0x0013));
    QCOMPARE(req.quantity(), quint16(10));
    QCOMPARE(req.byteCount(), quint8(2));
    QCOMPARE(req.values(), QByteArray::fromHex("CD01"));

    WriteMultipleCoilsRequest zeroBytes(request(QModbusPdu::WriteMultipleCoils,
                                                be16(0x0013) + be16(10) + QByteArray::fromHex("00")),
                                        kProto, 1, 0, kTs);
    QVERIFY(!zeroBytes.isValid());

    WriteMultipleCoilsRequest zeroQuantity(request(QModbusPdu::WriteMultipleCoils,
                                                   be16(0x0013) + be16(0) + QByteArray::fromHex("01CD")),
                                           kProto, 1, 0, kTs);
    QVERIFY(zeroQuantity.isValid());
    QCOMPARE(zeroQuantity.quantity(), quint16(0));

    WriteMultipleCoilsRequest mismatch(request(QModbusPdu::WriteMultipleCoils,
                                               be16(0x0013) + be16(10) + QByteArray::fromHex("03CD01")),
                                       kProto, 1, 0, kTs);
    QVERIFY(!mismatch.isValid());

    WriteMultipleCoilsResponse resp(response(QModbusPdu::WriteMultipleCoils, be16(0x0013) + be16(10)), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.startAddress(), quint16(0x0013));
    QCOMPARE(resp.quantity(), quint16(10));

    WriteMultipleCoilsResponse badResp(response(QModbusPdu::WriteMultipleCoils, be16(0x0013)), kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());

    QModbusExceptionResponse exception(QModbusPdu::WriteMultipleCoils,
                                       QModbusExceptionResponse::IllegalDataAddress);
    WriteMultipleCoilsResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::writeMultipleRegisters()
{
    WriteMultipleRegistersRequest req(request(QModbusPdu::WriteMultipleRegisters,
                                              be16(0x0001) + be16(2) + QByteArray::fromHex("0400010002")), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.startAddress(), quint16(0x0001));
    QCOMPARE(req.quantity(), quint16(2));
    QCOMPARE(req.byteCount(), quint8(4));
    QCOMPARE(req.values(), QByteArray::fromHex("00010002"));

    QModbusExceptionResponse exception(QModbusPdu::WriteMultipleRegisters,
                                       QModbusExceptionResponse::IllegalDataValue);
    WriteMultipleRegistersRequest exceptionReq(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionReq.isValid());

    WriteMultipleRegistersRequest zeroBytes(request(QModbusPdu::WriteMultipleRegisters,
                                                    be16(0x0001) + be16(2) + QByteArray::fromHex("00")),
                                            kProto, 1, 0, kTs);
    QVERIFY(!zeroBytes.isValid());

    WriteMultipleRegistersRequest mismatch(request(QModbusPdu::WriteMultipleRegisters,
                                                   be16(0x0001) + be16(2) + QByteArray::fromHex("0600010002")),
                                           kProto, 1, 0, kTs);
    QVERIFY(!mismatch.isValid());

    WriteMultipleRegistersRequest extraBytes(request(QModbusPdu::WriteMultipleRegisters,
                                                     be16(0x0001) + be16(2) + QByteArray::fromHex("0400010002FF")),
                                             kProto, 1, 0, kTs);
    QVERIFY(!extraBytes.isValid());

    WriteMultipleRegistersResponse resp(response(QModbusPdu::WriteMultipleRegisters, be16(0x0001) + be16(2)), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.startAddress(), quint16(0x0001));
    QCOMPARE(resp.quantity(), quint16(2));

    WriteMultipleRegistersResponse emptyResp(response(QModbusPdu::WriteMultipleRegisters, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(!emptyResp.isValid());

    WriteMultipleRegistersResponse badResp(response(QModbusPdu::WriteMultipleRegisters, be16(0x0001)), kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());
}

void TestModbusMessages::readExceptionStatus()
{
    ReadExceptionStatusRequest req(request(QModbusPdu::ReadExceptionStatus, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());

    ReadExceptionStatusResponse resp(response(QModbusPdu::ReadExceptionStatus, QByteArray::fromHex("6D")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.outputData(), quint8(0x6D));

    ReadExceptionStatusResponse bad(response(QModbusPdu::ReadExceptionStatus, QByteArray::fromHex("6D6D")), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    QModbusExceptionResponse exception(QModbusPdu::ReadExceptionStatus,
                                       QModbusExceptionResponse::ServerDeviceFailure);
    ReadExceptionStatusResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::diagnostics()
{
    DiagnosticsRequest req(request(QModbusPdu::Diagnostics, be16(0) + QByteArray::fromHex("A537")), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.subfunc(), quint16(0));
    QCOMPARE(req.data(), QByteArray::fromHex("A537"));

    DiagnosticsRequest badReq(request(QModbusPdu::Diagnostics, be16(0)), kProto, 1, 0, kTs);
    QVERIFY(!badReq.isValid());

    DiagnosticsRequest emptyReq(request(QModbusPdu::Diagnostics, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(!emptyReq.isValid());

    DiagnosticsResponse resp(response(QModbusPdu::Diagnostics, be16(0) + QByteArray::fromHex("A537")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.subfunc(), quint16(0));
    QCOMPARE(resp.data(), QByteArray::fromHex("A537"));

    DiagnosticsResponse badResp(response(QModbusPdu::Diagnostics, be16(0)), kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());

    QModbusExceptionResponse exception(QModbusPdu::Diagnostics,
                                       QModbusExceptionResponse::IllegalDataValue);
    DiagnosticsResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::getCommEventCounter()
{
    GetCommEventCounterResponse resp(response(QModbusPdu::GetCommEventCounter, be16(0xFFFF) + be16(0x0108)), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.status(), quint16(0xFFFF));
    QCOMPARE(resp.eventCount(), quint16(0x0108));

    GetCommEventCounterResponse bad(response(QModbusPdu::GetCommEventCounter, be16(0)), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    QModbusExceptionResponse exception(QModbusPdu::GetCommEventCounter,
                                       QModbusExceptionResponse::ServerDeviceFailure);
    GetCommEventCounterResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::getCommEventLog()
{
    GetCommEventLogRequest req(request(QModbusPdu::GetCommEventLog, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());

    const QByteArray events = QByteArray::fromHex("00112233445566778899");
    const QByteArray payload = QByteArray(1, char(events.size() - 6))
                               + be16(0x0000) + be16(0x0108) + be16(0x0121) + events;
    GetCommEventLogResponse resp(response(QModbusPdu::GetCommEventLog, payload), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.status(), quint16(0x0000));
    QCOMPARE(resp.eventCount(), quint16(0x0108));
    QCOMPARE(resp.messageCount(), quint16(0x0121));
    QCOMPARE(resp.events(), events);

    GetCommEventLogResponse zeroBytes(response(QModbusPdu::GetCommEventLog, QByteArray::fromHex("00")), kProto, 1, 0, kTs);
    QVERIFY(!zeroBytes.isValid());

    GetCommEventLogResponse emptyResp(response(QModbusPdu::GetCommEventLog, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(!emptyResp.isValid());

    GetCommEventLogResponse mismatch(response(QModbusPdu::GetCommEventLog, QByteArray::fromHex("03000001080121")),
                                     kProto, 1, 0, kTs);
    QVERIFY(!mismatch.isValid());

    QModbusExceptionResponse exception(QModbusPdu::GetCommEventLog,
                                       QModbusExceptionResponse::ServerDeviceFailure);
    GetCommEventLogResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::reportServerId()
{
    ReportServerIdRequest req(request(QModbusPdu::ReportServerId, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());

    ReportServerIdResponse resp(response(QModbusPdu::ReportServerId, QByteArray::fromHex("02FF01")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(2));
    QCOMPARE(resp.data(), QByteArray::fromHex("FF01"));

    ReportServerIdResponse bad(response(QModbusPdu::ReportServerId, QByteArray::fromHex("02")), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    QModbusExceptionResponse exception(QModbusPdu::ReportServerId,
                                       QModbusExceptionResponse::IllegalDataValue);
    ReportServerIdResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::readFileRecord()
{
    ReadFileRecordRequest req(request(QModbusPdu::ReadFileRecord, QByteArray::fromHex("07") + QByteArray(7, '\0')), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.byteCount(), quint8(0x07));

    ReadFileRecordRequest bad(request(QModbusPdu::ReadFileRecord, QByteArray::fromHex("06") + QByteArray(6, '\0')), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    ReadFileRecordRequest tooLong(request(QModbusPdu::ReadFileRecord, QByteArray::fromHex("F6") + QByteArray(0xF6, '\0')),
                                  kProto, 1, 0, kTs);
    QVERIFY(!tooLong.isValid());

    ReadFileRecordResponse resp(response(QModbusPdu::ReadFileRecord, QByteArray::fromHex("07") + QByteArray(7, '\1')), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(0x07));
    QCOMPARE(resp.data(), QByteArray(7, '\1'));

    ReadFileRecordResponse badResp(response(QModbusPdu::ReadFileRecord, QByteArray::fromHex("06") + QByteArray(6, '\1')),
                                   kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());

    ReadFileRecordResponse tooLongResp(response(QModbusPdu::ReadFileRecord, QByteArray::fromHex("F6") + QByteArray(0xF6, '\1')),
                                       kProto, 1, 0, kTs);
    QVERIFY(!tooLongResp.isValid());

    QModbusExceptionResponse exception(QModbusPdu::ReadFileRecord,
                                       QModbusExceptionResponse::IllegalDataAddress);
    ReadFileRecordResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::writeFileRecord()
{
    WriteFileRecordRequest req(request(QModbusPdu::WriteFileRecord, QByteArray::fromHex("09") + QByteArray(9, '\0')), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.length(), quint8(0x09));

    WriteFileRecordRequest bad(request(QModbusPdu::WriteFileRecord, QByteArray::fromHex("08") + QByteArray(8, '\0')), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    WriteFileRecordRequest tooLong(request(QModbusPdu::WriteFileRecord, QByteArray::fromHex("FC") + QByteArray(0xFC, '\0')),
                                   kProto, 1, 0, kTs);
    QVERIFY(!tooLong.isValid());

    WriteFileRecordResponse resp(response(QModbusPdu::WriteFileRecord, QByteArray::fromHex("09") + QByteArray(9, '\1')), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.length(), quint8(0x09));
    QCOMPARE(resp.data(), QByteArray(9, '\1'));

    WriteFileRecordResponse badResp(response(QModbusPdu::WriteFileRecord, QByteArray::fromHex("08") + QByteArray(8, '\1')),
                                    kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());

    WriteFileRecordResponse tooLongResp(response(QModbusPdu::WriteFileRecord, QByteArray::fromHex("FC") + QByteArray(0xFC, '\1')),
                                        kProto, 1, 0, kTs);
    QVERIFY(!tooLongResp.isValid());

    QModbusExceptionResponse exception(QModbusPdu::WriteFileRecord,
                                       QModbusExceptionResponse::IllegalDataValue);
    WriteFileRecordResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::maskWriteRegister()
{
    MaskWriteRegisterRequest req(request(QModbusPdu::MaskWriteRegister, be16(4) + be16(0x00F2) + be16(0x0025)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.address(), quint16(4));
    QCOMPARE(req.andMask(), quint16(0x00F2));
    QCOMPARE(req.orMask(), quint16(0x0025));

    MaskWriteRegisterRequest bad(request(QModbusPdu::MaskWriteRegister, be16(4) + be16(0)), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    MaskWriteRegisterResponse resp(response(QModbusPdu::MaskWriteRegister, be16(4) + be16(0x00F2) + be16(0x0025)), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.address(), quint16(4));
    QCOMPARE(resp.andMask(), quint16(0x00F2));
    QCOMPARE(resp.orMask(), quint16(0x0025));

    MaskWriteRegisterResponse badResp(response(QModbusPdu::MaskWriteRegister, be16(4)), kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());

    QModbusExceptionResponse exception(QModbusPdu::MaskWriteRegister,
                                       QModbusExceptionResponse::IllegalDataValue);
    MaskWriteRegisterResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::readWriteMultipleRegisters()
{
    const QByteArray writeValues = QByteArray::fromHex("00FF00FF");
    const QByteArray payload = be16(0x0003) + be16(6) + be16(0x000E) + be16(2)
                               + QByteArray(1, char(writeValues.size())) + writeValues;
    ReadWriteMultipleRegistersRequest req(request(QModbusPdu::ReadWriteMultipleRegisters, payload), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.readStartAddress(), quint16(0x0003));
    QCOMPARE(req.readLength(), quint16(6));
    QCOMPARE(req.writeStartAddress(), quint16(0x000E));
    QCOMPARE(req.writeLength(), quint16(2));
    QCOMPARE(req.writeByteCount(), quint8(4));
    QCOMPARE(req.writeValues(), writeValues);

    ReadWriteMultipleRegistersRequest zeroRead(request(QModbusPdu::ReadWriteMultipleRegisters,
                                                       be16(0) + be16(0) + be16(0) + be16(1) + QByteArray::fromHex("020001")),
                                               kProto, 1, 0, kTs);
    QVERIFY(!zeroRead.isValid());

    ReadWriteMultipleRegistersRequest tooMuchRead(request(QModbusPdu::ReadWriteMultipleRegisters,
                                                          be16(0) + be16(0x7E) + be16(0) + be16(1) + QByteArray::fromHex("020001")),
                                                  kProto, 1, 0, kTs);
    QVERIFY(!tooMuchRead.isValid());

    ReadWriteMultipleRegistersRequest zeroWrite(request(QModbusPdu::ReadWriteMultipleRegisters,
                                                        be16(0) + be16(1) + be16(0) + be16(0) + QByteArray::fromHex("00")),
                                                kProto, 1, 0, kTs);
    QVERIFY(!zeroWrite.isValid());

    ReadWriteMultipleRegistersRequest tooMuchWrite(request(QModbusPdu::ReadWriteMultipleRegisters,
                                                           be16(0) + be16(1) + be16(0) + be16(0x7A) + QByteArray::fromHex("020001")),
                                                   kProto, 1, 0, kTs);
    QVERIFY(!tooMuchWrite.isValid());

    ReadWriteMultipleRegistersRequest zeroBytes(request(QModbusPdu::ReadWriteMultipleRegisters,
                                                        be16(0) + be16(1) + be16(0) + be16(1) + QByteArray::fromHex("00")),
                                                kProto, 1, 0, kTs);
    QVERIFY(!zeroBytes.isValid());

    ReadWriteMultipleRegistersRequest mismatch(request(QModbusPdu::ReadWriteMultipleRegisters,
                                                       be16(0) + be16(1) + be16(0) + be16(1) + QByteArray::fromHex("040001")),
                                               kProto, 1, 0, kTs);
    QVERIFY(!mismatch.isValid());

    const QByteArray maxWriteValues(0xF2, '\1');
    ReadWriteMultipleRegistersRequest maxBoundary(request(QModbusPdu::ReadWriteMultipleRegisters,
                                                          be16(0) + be16(0x7D) + be16(0) + be16(0x79)
                                                              + QByteArray(1, char(maxWriteValues.size())) + maxWriteValues),
                                                  kProto, 1, 0, kTs);
    QVERIFY(maxBoundary.isValid());
    QCOMPARE(maxBoundary.writeByteCount(), quint8(0xF2));

    ReadWriteMultipleRegistersRequest extraBytes(request(QModbusPdu::ReadWriteMultipleRegisters,
                                                         be16(0) + be16(1) + be16(0) + be16(1)
                                                             + QByteArray::fromHex("020001FF")),
                                                 kProto, 1, 0, kTs);
    QVERIFY(!extraBytes.isValid());

    ReadWriteMultipleRegistersResponse resp(response(QModbusPdu::ReadWriteMultipleRegisters, QByteArray::fromHex("0400FF00FF")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(4));
    QCOMPARE(resp.values(), QByteArray::fromHex("00FF00FF"));

    ReadWriteMultipleRegistersResponse zeroByteResponse(response(QModbusPdu::ReadWriteMultipleRegisters, QByteArray::fromHex("00")),
                                                        kProto, 1, 0, kTs);
    QVERIFY(!zeroByteResponse.isValid());

    ReadWriteMultipleRegistersResponse badResp(response(QModbusPdu::ReadWriteMultipleRegisters, QByteArray::fromHex("0400FF")),
                                               kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());

    QModbusExceptionResponse exception(QModbusPdu::ReadWriteMultipleRegisters,
                                       QModbusExceptionResponse::IllegalDataAddress);
    ReadWriteMultipleRegistersResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

void TestModbusMessages::readFifoQueue()
{
    ReadFifoQueueRequest req(request(QModbusPdu::ReadFifoQueue, be16(0x04DE)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.fifoAddress(), quint16(0x04DE));

    ReadFifoQueueRequest bad(request(QModbusPdu::ReadFifoQueue, be16(0x04DE) + QByteArray(1, '\0')), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    ReadFifoQueueRequest emptyReq(request(QModbusPdu::ReadFifoQueue, QByteArray()), kProto, 1, 0, kTs);
    QVERIFY(!emptyReq.isValid());

    const QByteArray fifo = QByteArray::fromHex("01020304");
    const QByteArray payload = be16(0x0006) + be16(fifo.size()) + fifo;
    ReadFifoQueueResponse resp(response(QModbusPdu::ReadFifoQueue, payload), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint16(6));
    QCOMPARE(resp.fifoCount(), quint16(4));
    QCOMPARE(resp.fifoValue(), fifo);

    ReadFifoQueueResponse tooLong(response(QModbusPdu::ReadFifoQueue, be16(0x0024) + be16(32) + QByteArray(32, '\1')),
                                  kProto, 1, 0, kTs);
    QVERIFY(!tooLong.isValid());

    ReadFifoQueueResponse mismatch(response(QModbusPdu::ReadFifoQueue, be16(0x0006) + be16(6) + fifo), kProto, 1, 0, kTs);
    QVERIFY(!mismatch.isValid());

    QModbusExceptionResponse exception(QModbusPdu::ReadFifoQueue,
                                       QModbusExceptionResponse::IllegalDataAddress);
    ReadFifoQueueResponse exceptionResp(exception, kProto, 1, 0, kTs);
    QVERIFY(exceptionResp.isValid());
}

QTEST_GUILESS_MAIN(TestModbusMessages)
#include "test_modbusmessages.moc"
