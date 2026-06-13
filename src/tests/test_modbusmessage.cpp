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

QTEST_GUILESS_MAIN(TestModbusMessage)
#include "test_modbusmessage.moc"
