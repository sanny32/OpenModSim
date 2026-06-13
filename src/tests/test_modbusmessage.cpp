// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbusmessage.cpp
/// \brief Unit tests for ModbusMessage ADU build/parse round-trips.
///

#include <QModbusRequest>
#include <QModbusResponse>
#include <QTest>

#include "modbusmessage.h"

class TestModbusMessage : public QObject
{
    Q_OBJECT

private slots:
    void tcpRequestRoundTrip();
    void rtuRequestHasValidCrc();
    void functionCodeStripsExceptionBit();
    void exceptionResponseIsFlagged();
    void toStringProducesHex();
    void createDispatchesFunctionCodes_data();
    void createDispatchesFunctionCodes();
    void createFallsBackForUnknownFunctionCode();
};

void TestModbusMessage::tcpRequestRoundTrip()
{
    const QModbusRequest request(QModbusPdu::ReadHoldingRegisters, quint16(0x0010), quint16(5));
    const auto message = ModbusMessage::create(request, ModbusMessage::Tcp, 17, 1, QDateTime::currentDateTime(), true);

    QVERIFY(message->isValid());
    QVERIFY(message->isRequest());
    QCOMPARE(message->protocolType(), ModbusMessage::Tcp);
    QCOMPARE(message->deviceId(), 17);
    QCOMPARE(message->functionCode(), QModbusPdu::ReadHoldingRegisters);

    const auto parsed = ModbusMessage::create(message->rawData(), ModbusMessage::Tcp, QDateTime::currentDateTime(), true);
    QVERIFY(parsed->isValid());
    QCOMPARE(parsed->deviceId(), 17);
    QCOMPARE(parsed->functionCode(), QModbusPdu::ReadHoldingRegisters);
}

void TestModbusMessage::rtuRequestHasValidCrc()
{
    const QModbusRequest request(QModbusPdu::ReadCoils, quint16(0), quint16(8));
    const auto message = ModbusMessage::create(request, ModbusMessage::Rtu, 1, 0, QDateTime::currentDateTime(), true);

    QVERIFY(message->isValid());

    const auto parsed = ModbusMessage::create(message->rawData(), ModbusMessage::Rtu, QDateTime::currentDateTime(), true);
    QVERIFY(parsed->isValid());
    QCOMPARE(parsed->functionCode(), QModbusPdu::ReadCoils);
    QCOMPARE(parsed->deviceId(), 1);
}

void TestModbusMessage::functionCodeStripsExceptionBit()
{
    const QModbusRequest request(QModbusPdu::WriteSingleRegister, quint16(7), quint16(0x1234));
    const auto message = ModbusMessage::create(request, ModbusMessage::Tcp, 3, 9, QDateTime::currentDateTime(), true);

    QVERIFY(!message->isException());
    QCOMPARE(message->functionCode(), QModbusPdu::WriteSingleRegister);
    QCOMPARE(QString(message->function()), QStringLiteral("WRITE SINGLE REG"));
}

void TestModbusMessage::exceptionResponseIsFlagged()
{
    const QModbusExceptionResponse exception(QModbusPdu::ReadHoldingRegisters,
                                             QModbusExceptionResponse::IllegalDataAddress);
    const auto message = ModbusMessage::create(exception, ModbusMessage::Tcp, 5, 2, QDateTime::currentDateTime(), false);

    QVERIFY(message->isException());
    QVERIFY(!message->isRequest());
    QCOMPARE(message->functionCode(), QModbusPdu::ReadHoldingRegisters);
}

void TestModbusMessage::toStringProducesHex()
{
    const QModbusRequest request(QModbusPdu::ReadCoils, quint16(0), quint16(1));
    const auto message = ModbusMessage::create(request, ModbusMessage::Tcp, 1, 1, QDateTime::currentDateTime(), true);

    const QString text = message->toString(DataType::Hex);
    QVERIFY(!text.isEmpty());
    QVERIFY(text.contains(' '));
}

void TestModbusMessage::createDispatchesFunctionCodes_data()
{
    QTest::addColumn<int>("code");
    QTest::addColumn<int>("protocol");

    const QVector<QModbusPdu::FunctionCode> codes = {
        QModbusPdu::ReadCoils, QModbusPdu::ReadDiscreteInputs, QModbusPdu::ReadHoldingRegisters,
        QModbusPdu::ReadInputRegisters, QModbusPdu::WriteSingleCoil, QModbusPdu::WriteSingleRegister,
        QModbusPdu::ReadExceptionStatus, QModbusPdu::Diagnostics, QModbusPdu::GetCommEventCounter,
        QModbusPdu::GetCommEventLog, QModbusPdu::WriteMultipleCoils, QModbusPdu::WriteMultipleRegisters,
        QModbusPdu::ReportServerId, QModbusPdu::ReadFileRecord, QModbusPdu::WriteFileRecord,
        QModbusPdu::MaskWriteRegister, QModbusPdu::ReadWriteMultipleRegisters, QModbusPdu::ReadFifoQueue
    };

    for (const auto code : codes)
        for (const auto protocol : {ModbusMessage::Rtu, ModbusMessage::Tcp})
            QTest::addRow("fc%02X-%s", int(code), protocol == ModbusMessage::Rtu ? "rtu" : "tcp")
                << int(code) << int(protocol);
}

void TestModbusMessage::createDispatchesFunctionCodes()
{
    QFETCH(int, code);
    QFETCH(int, protocol);
    const auto fc = static_cast<QModbusPdu::FunctionCode>(code);
    const auto proto = static_cast<ModbusMessage::ProtocolType>(protocol);

    const QModbusRequest request(fc, QByteArray::fromHex("00000001"));
    const auto message = ModbusMessage::create(request, proto, 1, 0, QDateTime::currentDateTime(), true);
    QCOMPARE(message->functionCode(), fc);
    QCOMPARE(message->protocolType(), proto);
    QVERIFY(message->isRequest());

    const auto parsed = ModbusMessage::create(message->rawData(), proto, QDateTime::currentDateTime(), true);
    QCOMPARE(parsed->functionCode(), fc);
}

void TestModbusMessage::createFallsBackForUnknownFunctionCode()
{
    const QModbusRequest request(static_cast<QModbusPdu::FunctionCode>(0x65), QByteArray::fromHex("0102"));
    const auto message = ModbusMessage::create(request, ModbusMessage::Tcp, 1, 0, QDateTime::currentDateTime(), true);
    QCOMPARE(int(message->functionCode()), 0x65);
    QVERIFY(message->isRequest());
}

QTEST_GUILESS_MAIN(TestModbusMessage)
#include "test_modbusmessage.moc"
