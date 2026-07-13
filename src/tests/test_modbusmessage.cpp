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

namespace {

class InspectableModbusMessage : public ModbusMessage
{
public:
    using ModbusMessage::ModbusMessage;

    int exposedDataSize() const
    {
        return dataSize();
    }

    quint8 exposedAt(int idx) const
    {
        return at(idx);
    }

    QByteArray exposedData(int idx, int len = -1) const
    {
        return data(idx, len);
    }
};

}

class TestModbusMessage : public QObject
{
    Q_OBJECT

private slots:
    void tcpRequestRoundTrip();
    void rtuRequestHasValidCrc();
    void functionCodeStripsExceptionBit();
    void exceptionResponseIsFlagged();
    void toStringProducesHex();
    void accessorsExposeRawAduAndException();
    void rtuAduRejectsShortAndBadChecksum();
    void tcpAduRejectsShortAndLengthMismatch();
    void directRawConstructorsKeepProtocolAndPayload();
    void protectedPayloadHelpersExposePduData();
    void createDispatchesFunctionCodes_data();
    void createDispatchesFunctionCodes();
    void createDispatchesExceptionFunctionCodes_data();
    void createDispatchesExceptionFunctionCodes();
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

void TestModbusMessage::accessorsExposeRawAduAndException()
{
    const QDateTime timestamp = QDateTime::fromString(QStringLiteral("2026-07-07T10:00:00.000"), Qt::ISODateWithMs);
    const QModbusExceptionResponse exception(QModbusPdu::WriteSingleCoil,
                                             QModbusExceptionResponse::IllegalDataValue);
    const auto message = ModbusMessage::create(exception, ModbusMessage::Rtu, 9, 3, timestamp, false);

    QVERIFY(message->isValid());
    QCOMPARE(message->timestamp(), timestamp);
    QCOMPARE(static_cast<int>(message->exception()), int(QModbusExceptionResponse::IllegalDataValue));
    QVERIFY(message->adu() != nullptr);
    QCOMPARE(QByteArray(*message), message->rawData());
    QCOMPARE(message->toString(DataType::UInt16, false), formatUInt8Array(DataType::UInt16, false, message->rawData()));
}

void TestModbusMessage::rtuAduRejectsShortAndBadChecksum()
{
    QModbusAduRtu empty{QByteArray()};
    QVERIFY(!empty.isValid());
    QCOMPARE(empty.serverAddress(), quint8(0));
    QCOMPARE(empty.checksum(), quint16(0));
    QCOMPARE(empty.calcChecksum(), quint16(0));
    QCOMPARE(empty.functionCode(), QModbusPdu::Invalid);

    QModbusAduRtu shortFrame(QByteArray::fromHex("0103FF"));
    QVERIFY(!shortFrame.isValid());
    QCOMPARE(shortFrame.serverAddress(), quint8(1));
    QCOMPARE(shortFrame.checksum(), quint16(0));

    const QModbusRequest request(QModbusPdu::ReadCoils, quint16(0), quint16(8));
    const auto message = ModbusMessage::create(request, ModbusMessage::Rtu, 1, 0, QDateTime::currentDateTime(), true);
    QByteArray badChecksum = message->rawData();
    badChecksum[badChecksum.size() - 1] = char(quint8(badChecksum[badChecksum.size() - 1]) ^ 0xFF);

    QModbusAduRtu bad(badChecksum);
    QVERIFY(!bad.matchingChecksum());
    QVERIFY(!bad.isValid());
    QCOMPARE(QModbusAduRtu::calculateCRC(nullptr, 1), quint16(0));
    QCOMPARE(QModbusAduRtu::calculateCRC("x", 0), quint16(0));
}

void TestModbusMessage::tcpAduRejectsShortAndLengthMismatch()
{
    QModbusAduTcp empty{QByteArray()};
    QVERIFY(!empty.isValid());
    QCOMPARE(empty.transactionId(), quint16(0));
    QCOMPARE(empty.protocolId(), quint16(0));
    QCOMPARE(empty.length(), quint16(0));
    QCOMPARE(empty.serverAddress(), quint8(0));
    QCOMPARE(empty.functionCode(), QModbusPdu::Invalid);

    QModbusAduTcp shortFrame(QByteArray::fromHex("00010000000101"));
    QVERIFY(!shortFrame.isValid());
    QCOMPARE(shortFrame.length(), quint16(0));

    const QModbusRequest request(QModbusPdu::ReadHoldingRegisters, quint16(0x0010), quint16(5));
    const auto message = ModbusMessage::create(request, ModbusMessage::Tcp, 17, 0x1234, QDateTime::currentDateTime(), true);
    QModbusAduTcp valid(message->rawData());
    QVERIFY(valid.isValid());
    QCOMPARE(valid.transactionId(), quint16(0x1234));
    QCOMPARE(valid.protocolId(), quint16(0));
    QCOMPARE(valid.serverAddress(), quint8(17));

    QByteArray badLength = message->rawData();
    badLength[4] = char(0);
    badLength[5] = char(1);
    QModbusAduTcp mismatch(badLength);
    QVERIFY(!mismatch.isValid());
}

void TestModbusMessage::directRawConstructorsKeepProtocolAndPayload()
{
    const QDateTime timestamp = QDateTime::fromString(QStringLiteral("2026-07-08T09:00:00.000"), Qt::ISODateWithMs);
    const QModbusRequest request(QModbusPdu::ReadCoils, quint16(0x0010), quint16(8));

    const InspectableModbusMessage tcpSource(request, ModbusMessage::Tcp, 17, 0x1234, timestamp, true);
    const InspectableModbusMessage tcpParsed(tcpSource.rawData(), ModbusMessage::Tcp, timestamp, false);
    QVERIFY(tcpParsed.isValid());
    QCOMPARE(tcpParsed.protocolType(), ModbusMessage::Tcp);
    QCOMPARE(tcpParsed.deviceId(), 17);
    QCOMPARE(tcpParsed.functionCode(), QModbusPdu::ReadCoils);
    QVERIFY(!tcpParsed.isRequest());

    const InspectableModbusMessage rtuSource(request, ModbusMessage::Rtu, 9, 0, timestamp, true);
    const InspectableModbusMessage rtuParsed(rtuSource.rawData(), ModbusMessage::Rtu, timestamp, true);
    QVERIFY(rtuParsed.isValid());
    QCOMPARE(rtuParsed.protocolType(), ModbusMessage::Rtu);
    QCOMPARE(rtuParsed.deviceId(), 9);
    QCOMPARE(rtuParsed.functionCode(), QModbusPdu::ReadCoils);
    QVERIFY(rtuParsed.isRequest());
}

void TestModbusMessage::protectedPayloadHelpersExposePduData()
{
    const QDateTime timestamp = QDateTime::fromString(QStringLiteral("2026-07-08T09:30:00.000"), Qt::ISODateWithMs);
    const QModbusRequest request(QModbusPdu::WriteSingleRegister, quint16(0x1234), quint16(0xABCD));
    const InspectableModbusMessage message(request, ModbusMessage::Tcp, 4, 0x2211, timestamp, true);

    QVERIFY(message.isValid());
    QCOMPARE(message.exposedDataSize(), 4);
    QCOMPARE(message.exposedAt(0), quint8(0x12));
    QCOMPARE(message.exposedAt(1), quint8(0x34));
    QCOMPARE(message.exposedAt(2), quint8(0xAB));
    QCOMPARE(message.exposedAt(3), quint8(0xCD));
    QCOMPARE(message.exposedAt(4), quint8(0));
    QCOMPARE(message.exposedData(1, 2), QByteArray::fromHex("34AB"));
    QCOMPARE(message.exposedData(2), QByteArray::fromHex("ABCD"));
}

void TestModbusMessage::createDispatchesFunctionCodes_data()
{
    QTest::addColumn<int>("code");
    QTest::addColumn<int>("protocol");
    QTest::addColumn<bool>("request");

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
            for (const bool request : {true, false})
                QTest::addRow("fc%02X-%s-%s", int(code), protocol == ModbusMessage::Rtu ? "rtu" : "tcp",
                              request ? "request" : "response")
                    << int(code) << int(protocol) << request;
}

void TestModbusMessage::createDispatchesFunctionCodes()
{
    QFETCH(int, code);
    QFETCH(int, protocol);
    QFETCH(bool, request);
    const auto fc = static_cast<QModbusPdu::FunctionCode>(code);
    const auto proto = static_cast<ModbusMessage::ProtocolType>(protocol);

    const auto message = request
        ? ModbusMessage::create(QModbusRequest(fc, QByteArray::fromHex("00000001")), proto, 1, 0,
                                QDateTime::currentDateTime(), request)
        : ModbusMessage::create(QModbusResponse(fc, QByteArray::fromHex("00000001")), proto, 1, 0,
                                QDateTime::currentDateTime(), request);
    QCOMPARE(message->functionCode(), fc);
    QCOMPARE(message->protocolType(), proto);
    QCOMPARE(message->isRequest(), request);

    const auto parsed = ModbusMessage::create(message->rawData(), proto, QDateTime::currentDateTime(), request);
    QCOMPARE(parsed->functionCode(), fc);
    QCOMPARE(parsed->isRequest(), request);
}

void TestModbusMessage::createDispatchesExceptionFunctionCodes_data()
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

    for (const auto code : codes) {
        for (const auto protocol : {ModbusMessage::Rtu, ModbusMessage::Tcp}) {
            QTest::addRow("fc%02X-%s-exception", int(code), protocol == ModbusMessage::Rtu ? "rtu" : "tcp")
                << int(code) << int(protocol);
        }
    }
}

void TestModbusMessage::createDispatchesExceptionFunctionCodes()
{
    QFETCH(int, code);
    QFETCH(int, protocol);
    const auto fc = static_cast<QModbusPdu::FunctionCode>(code);
    const auto proto = static_cast<ModbusMessage::ProtocolType>(protocol);
    const QModbusExceptionResponse exception(fc, QModbusExceptionResponse::IllegalDataValue);

    const auto message = ModbusMessage::create(exception, proto, 1, 0, QDateTime::currentDateTime(), false);

    QCOMPARE(message->functionCode(), fc);
    QCOMPARE(message->protocolType(), proto);
    QVERIFY(message->isException());
    QVERIFY(!message->isRequest());

    const auto parsed = ModbusMessage::create(message->rawData(), proto, QDateTime::currentDateTime(), false);
    QCOMPARE(parsed->functionCode(), fc);
    QVERIFY(parsed->isException());
    QVERIFY(!parsed->isRequest());
}

void TestModbusMessage::createFallsBackForUnknownFunctionCode()
{
    const QModbusRequest request(static_cast<QModbusPdu::FunctionCode>(0x65), QByteArray::fromHex("0102"));
    const auto message = ModbusMessage::create(request, ModbusMessage::Tcp, 1, 0, QDateTime::currentDateTime(), true);
    QCOMPARE(int(message->functionCode()), 0x65);
    QVERIFY(message->isRequest());

    const auto parsedTcp = ModbusMessage::create(message->rawData(), ModbusMessage::Tcp, QDateTime::currentDateTime(), true);
    QCOMPARE(int(parsedTcp->functionCode()), 0x65);
    QVERIFY(parsedTcp->isRequest());

    const auto rtuMessage = ModbusMessage::create(request, ModbusMessage::Rtu, 1, 0, QDateTime::currentDateTime(), false);
    const auto parsedRtu = ModbusMessage::create(rtuMessage->rawData(), ModbusMessage::Rtu, QDateTime::currentDateTime(), false);
    QCOMPARE(int(parsedRtu->functionCode()), 0x65);
    QVERIFY(!parsedRtu->isRequest());
}

QTEST_GUILESS_MAIN(TestModbusMessage)
#include "test_modbusmessage.moc"
