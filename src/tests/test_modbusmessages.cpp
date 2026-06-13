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

    ReadCoilsRequest bad(request(QModbusPdu::ReadCoils, be16(0) + be16(0)), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    ReadCoilsResponse resp(response(QModbusPdu::ReadCoils, QByteArray::fromHex("02FF01")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(2));
    QCOMPARE(resp.coilStatus(), QByteArray::fromHex("FF01"));

    ReadCoilsResponse badResp(response(QModbusPdu::ReadCoils, QByteArray::fromHex("05FF")), kProto, 1, 0, kTs);
    QVERIFY(!badResp.isValid());
}

void TestModbusMessages::readDiscreteInputs()
{
    ReadDiscreteInputsRequest req(request(QModbusPdu::ReadDiscreteInputs, be16(7) + be16(3)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.startAddress(), quint16(7));
    QCOMPARE(req.length(), quint16(3));

    ReadDiscreteInputsResponse resp(response(QModbusPdu::ReadDiscreteInputs, QByteArray::fromHex("0105")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(1));
    QCOMPARE(resp.inputStatus(), QByteArray::fromHex("05"));
}

void TestModbusMessages::readHoldingRegisters()
{
    ReadHoldingRegistersRequest req(request(QModbusPdu::ReadHoldingRegisters, be16(0x0100) + be16(2)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.startAddress(), quint16(0x0100));
    QCOMPARE(req.length(), quint16(2));

    ReadHoldingRegistersRequest bad(request(QModbusPdu::ReadHoldingRegisters, be16(0) + be16(0x7E)), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    ReadHoldingRegistersResponse ok(response(QModbusPdu::ReadHoldingRegisters, QByteArray::fromHex("0400010002")), kProto, 1, 0, kTs);
    QVERIFY(ok.isValid());
    QCOMPARE(ok.byteCount(), quint8(4));
    QCOMPARE(ok.registerValue(), QByteArray::fromHex("00010002"));
}

void TestModbusMessages::readInputRegisters()
{
    ReadInputRegistersRequest req(request(QModbusPdu::ReadInputRegisters, be16(0) + be16(1)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.length(), quint16(1));

    ReadInputRegistersResponse resp(response(QModbusPdu::ReadInputRegisters, QByteArray::fromHex("020ABC")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(2));
    QCOMPARE(resp.registerValue(), QByteArray::fromHex("0ABC"));
}

void TestModbusMessages::writeSingleCoil()
{
    WriteSingleCoilRequest on(request(QModbusPdu::WriteSingleCoil, be16(3) + be16(0xFF00)), kProto, 1, 0, kTs);
    QVERIFY(on.isValid());
    QCOMPARE(on.address(), quint16(3));
    QCOMPARE(on.value(), quint16(0xFF00));

    WriteSingleCoilRequest bad(request(QModbusPdu::WriteSingleCoil, be16(3) + be16(0x1234)), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    WriteSingleCoilResponse resp(response(QModbusPdu::WriteSingleCoil, be16(3) + be16(0)), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.value(), quint16(0));
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

    WriteMultipleCoilsResponse resp(response(QModbusPdu::WriteMultipleCoils, be16(0x0013) + be16(10)), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.quantity(), quint16(10));
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

    WriteMultipleRegistersResponse resp(response(QModbusPdu::WriteMultipleRegisters, be16(0x0001) + be16(2)), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.startAddress(), quint16(0x0001));
    QCOMPARE(resp.quantity(), quint16(2));
}

void TestModbusMessages::readExceptionStatus()
{
    ReadExceptionStatusResponse resp(response(QModbusPdu::ReadExceptionStatus, QByteArray::fromHex("6D")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.outputData(), quint8(0x6D));

    ReadExceptionStatusResponse bad(response(QModbusPdu::ReadExceptionStatus, QByteArray::fromHex("6D6D")), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());
}

void TestModbusMessages::diagnostics()
{
    DiagnosticsRequest req(request(QModbusPdu::Diagnostics, be16(0) + QByteArray::fromHex("A537")), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.subfunc(), quint16(0));
    QCOMPARE(req.data(), QByteArray::fromHex("A537"));

    DiagnosticsResponse resp(response(QModbusPdu::Diagnostics, be16(0) + QByteArray::fromHex("A537")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.subfunc(), quint16(0));
}

void TestModbusMessages::getCommEventCounter()
{
    GetCommEventCounterResponse resp(response(QModbusPdu::GetCommEventCounter, be16(0xFFFF) + be16(0x0108)), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.status(), quint16(0xFFFF));
    QCOMPARE(resp.eventCount(), quint16(0x0108));

    GetCommEventCounterResponse bad(response(QModbusPdu::GetCommEventCounter, be16(0)), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());
}

void TestModbusMessages::getCommEventLog()
{
    const QByteArray events = QByteArray::fromHex("00112233445566778899");
    const QByteArray payload = QByteArray(1, char(events.size() - 6))
                               + be16(0x0000) + be16(0x0108) + be16(0x0121) + events;
    GetCommEventLogResponse resp(response(QModbusPdu::GetCommEventLog, payload), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.status(), quint16(0x0000));
    QCOMPARE(resp.eventCount(), quint16(0x0108));
    QCOMPARE(resp.messageCount(), quint16(0x0121));
    QCOMPARE(resp.events(), events);
}

void TestModbusMessages::reportServerId()
{
    ReportServerIdResponse resp(response(QModbusPdu::ReportServerId, QByteArray::fromHex("02FF01")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(2));
    QCOMPARE(resp.data(), QByteArray::fromHex("FF01"));
}

void TestModbusMessages::readFileRecord()
{
    ReadFileRecordRequest req(request(QModbusPdu::ReadFileRecord, QByteArray::fromHex("07") + QByteArray(7, '\0')), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.byteCount(), quint8(0x07));

    ReadFileRecordRequest bad(request(QModbusPdu::ReadFileRecord, QByteArray::fromHex("06") + QByteArray(6, '\0')), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    ReadFileRecordResponse resp(response(QModbusPdu::ReadFileRecord, QByteArray::fromHex("07") + QByteArray(7, '\1')), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
}

void TestModbusMessages::writeFileRecord()
{
    WriteFileRecordRequest req(request(QModbusPdu::WriteFileRecord, QByteArray::fromHex("09") + QByteArray(9, '\0')), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.length(), quint8(0x09));

    WriteFileRecordRequest bad(request(QModbusPdu::WriteFileRecord, QByteArray::fromHex("08") + QByteArray(8, '\0')), kProto, 1, 0, kTs);
    QVERIFY(!bad.isValid());

    WriteFileRecordResponse resp(response(QModbusPdu::WriteFileRecord, QByteArray::fromHex("09") + QByteArray(9, '\1')), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
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

    ReadWriteMultipleRegistersResponse resp(response(QModbusPdu::ReadWriteMultipleRegisters, QByteArray::fromHex("0400FF00FF")), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.byteCount(), quint8(4));
}

void TestModbusMessages::readFifoQueue()
{
    ReadFifoQueueRequest req(request(QModbusPdu::ReadFifoQueue, be16(0x04DE)), kProto, 1, 0, kTs);
    QVERIFY(req.isValid());
    QCOMPARE(req.fifoAddress(), quint16(0x04DE));

    const QByteArray fifo = QByteArray::fromHex("01020304");
    const QByteArray payload = be16(0x0006) + be16(fifo.size()) + fifo;
    ReadFifoQueueResponse resp(response(QModbusPdu::ReadFifoQueue, payload), kProto, 1, 0, kTs);
    QVERIFY(resp.isValid());
    QCOMPARE(resp.fifoCount(), quint16(4));
    QCOMPARE(resp.fifoValue(), fifo);
}

QTEST_GUILESS_MAIN(TestModbusMessages)
#include "test_modbusmessages.moc"
