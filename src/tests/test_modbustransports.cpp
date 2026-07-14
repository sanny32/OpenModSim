// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbustransports.cpp
/// \brief Integration tests for Modbus server transports.
///

#include <QSignalSpy>
#include <QTcpSocket>
#include <QTest>

#include "modbusrtuserialserver.h"
#include "modbusrtutcpserver.h"
#include "modbustcpserver.h"
#include "qmodbusadurtu.h"

namespace {

quint16 availablePort()
{
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, 0))
        return 0;
    return probe.serverPort();
}

ModbusDataUnitMap holdingMap()
{
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 0, 64);
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::Coils, 0, 64);
    return map;
}

QByteArray tcpFrame(quint16 transactionId, quint8 address, const QModbusRequest& request)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream << transactionId << quint16(0) << quint16(request.size() + 1) << address << request;
    return result;
}

QByteArray rtuFrame(quint8 address, const QModbusRequest& request)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream << address << request;
    stream << QModbusAduRtu::calculateCRC(result.constData(), result.size());
    return result;
}

QByteArray receive(QTcpSocket& socket)
{
    QElapsedTimer timer;
    timer.start();
    while (!socket.bytesAvailable() && timer.elapsed() < 1000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QTest::qWait(1);
    }
    QByteArray result = socket.readAll();
    timer.restart();
    while (timer.elapsed() < 10) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        if (!socket.bytesAvailable())
            continue;
        result.append(socket.readAll());
        timer.restart();
    }
    return result;
}

template <typename Server>
quint16 startServer(Server& server)
{
    const quint16 port = availablePort();
    if (!port)
        return 0;
    server.setConnectionParameter(QModbusDevice::NetworkAddressParameter,
                                  QStringLiteral("127.0.0.1"));
    server.setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
    server.addServerAddress(1);
    server.setMap(holdingMap(), 1);
    if (!server.connectDevice())
        return 0;
    return port;
}

}

class TestModbusTransports : public QObject
{
    Q_OBJECT

private slots:
    void servesModbusTcpRequests();
    void simulatesModbusTcpErrors();
    void rejectsOccupiedTcpPort();
    void servesRtuOverTcpRequests();
    void recoversRtuStreamAndProcessesBroadcast();
    void simulatesRtuOverTcpErrors();
    void configuresSerialTransport();
};

void TestModbusTransports::servesModbusTcpRequests()
{
    ModbusTcpServer server;
    const quint16 port = startServer(server);
    QVERIFY(port != 0);
    QCOMPARE(server.connectionParameter(QModbusDevice::NetworkPortParameter).toUInt(), uint(port));
    QCOMPARE(server.connectionParameter(QModbusDevice::NetworkAddressParameter).toString(),
             QStringLiteral("127.0.0.1"));
    QVERIFY(!server.connectionParameter(QModbusDevice::SerialPortNameParameter).isValid());

    QSignalSpy connectedSpy(&server, &ModbusTcpServer::modbusClientConnected);
    QSignalSpy disconnectedSpy(&server, &ModbusTcpServer::modbusClientDisconnected);
    QSignalSpy requestSpy(&server, &ModbusServer::modbusRequest);
    QSignalSpy responseSpy(&server, &ModbusServer::modbusResponse);
    QSignalSpy receivedSpy(&server, &ModbusServer::rawDataReceived);
    QSignalSpy sentSpy(&server, &ModbusServer::rawDataSended);

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(socket.waitForConnected(1000));
    QTRY_COMPARE(server.connectedClientCount(), 1);
    QCOMPARE(connectedSpy.count(), 1);

    const QByteArray frame = tcpFrame(
        0x1234, 1,
        QModbusRequest(QModbusRequest::ReadHoldingRegisters, quint16(0), quint16(2)));
    QCOMPARE(socket.write(frame.left(4)), qint64(4));
    QVERIFY(socket.waitForBytesWritten(1000));
    QTest::qWait(5);
    QCOMPARE(socket.write(frame.mid(4)), qint64(frame.size() - 4));
    const QByteArray response = receive(socket);
    QVERIFY(response.size() >= 13);
    QCOMPARE(quint8(response.at(6)), quint8(1));
    QCOMPARE(quint8(response.at(7)), quint8(QModbusRequest::ReadHoldingRegisters));
    QCOMPARE(requestSpy.count(), 1);
    QCOMPARE(responseSpy.count(), 1);
    QVERIFY(receivedSpy.count() >= 2);
    QCOMPARE(sentSpy.count(), 1);

    const QByteArray twoFrames = tcpFrame(
        2, 1, QModbusRequest(QModbusRequest::WriteSingleRegister,
                             quint16(3), quint16(0x4567)))
        + tcpFrame(3, 9, QModbusRequest(QModbusRequest::ReadHoldingRegisters,
                                       quint16(0), quint16(1)));
    QCOMPARE(socket.write(twoFrames), qint64(twoFrames.size()));
    QVERIFY(!receive(socket).isEmpty());
    quint16 value = 0;
    QVERIFY(server.data(QModbusDataUnit::HoldingRegisters, 3, &value, 1));
    QCOMPARE(value, quint16(0x4567));

    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState)
        QVERIFY(socket.waitForDisconnected(1000));
    QTRY_COMPARE(server.connectedClientCount(), 0);
    QCOMPARE(disconnectedSpy.count(), 1);
    server.disconnectDevice();
    QCOMPARE(server.state(), QModbusDevice::UnconnectedState);
}

void TestModbusTransports::simulatesModbusTcpErrors()
{
    ModbusTcpServer server;
    const quint16 port = startServer(server);
    QVERIFY(port != 0);
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(socket.waitForConnected(1000));
    QTRY_COMPARE(server.connectedClientCount(), 1);

    ModbusDefinitions definitions;
    definitions.ErrorSimulations.setResponseIncorrectId(true);
    server.setDefinitions(definitions);
    socket.write(tcpFrame(1, 1, QModbusRequest(QModbusRequest::ReadHoldingRegisters,
                                               quint16(0), quint16(1))));
    QByteArray response = receive(socket);
    QVERIFY(response.size() > 7);
    QCOMPARE(quint8(response.at(6)), quint8(2));

    definitions.ErrorSimulations.setResponseIncorrectId(false);
    definitions.ErrorSimulations.setResponseIllegalFunction(true);
    server.setDefinitions(definitions);
    socket.write(tcpFrame(2, 1, QModbusRequest(QModbusRequest::ReadHoldingRegisters,
                                               quint16(0), quint16(1))));
    response = receive(socket);
    QVERIFY(response.size() > 8);
    QVERIFY(quint8(response.at(7)) & QModbusPdu::ExceptionByte);

    definitions.ErrorSimulations.setResponseIllegalFunction(false);
    definitions.ErrorSimulations.setResponseDeviceBusy(true);
    server.setDefinitions(definitions);
    socket.write(tcpFrame(3, 1, QModbusRequest(QModbusRequest::ReadHoldingRegisters,
                                               quint16(0), quint16(1))));
    response = receive(socket);
    QVERIFY(response.size() > 8);
    QCOMPARE(quint8(response.at(8)), quint8(QModbusExceptionResponse::ServerDeviceBusy));

    definitions.ErrorSimulations.setResponseDeviceBusy(false);
    definitions.ErrorSimulations.setResponseDelay(true);
    definitions.ErrorSimulations.setResponseDelayTime(20);
    server.setDefinitions(definitions);
    socket.write(tcpFrame(4, 1, QModbusRequest(QModbusRequest::ReadHoldingRegisters,
                                               quint16(0), quint16(1))));
    response = receive(socket);
    QVERIFY(!response.isEmpty());

    definitions.ErrorSimulations.setResponseDelay(false);
    definitions.ErrorSimulations.setNoResponse(true);
    server.setDefinitions(definitions);
    socket.write(tcpFrame(5, 1, QModbusRequest(QModbusRequest::ReadHoldingRegisters,
                                               quint16(0), quint16(1))));
    QVERIFY(socket.waitForBytesWritten(1000));
    QTest::qWait(30);
    QCOMPARE(socket.bytesAvailable(), qint64(0));
}

void TestModbusTransports::rejectsOccupiedTcpPort()
{
    QTcpServer occupied;
    QVERIFY(occupied.listen(QHostAddress::LocalHost, 0));

    ModbusTcpServer tcp;
    tcp.setConnectionParameter(QModbusDevice::NetworkAddressParameter,
                               QStringLiteral("127.0.0.1"));
    tcp.setConnectionParameter(QModbusDevice::NetworkPortParameter, occupied.serverPort());
    QVERIFY(!tcp.connectDevice());
    QCOMPARE(tcp.error(0), QModbusDevice::ConnectionError);

    ModbusRtuTcpServer rtu;
    rtu.setConnectionParameter(QModbusDevice::NetworkAddressParameter,
                               QStringLiteral("127.0.0.1"));
    rtu.setConnectionParameter(QModbusDevice::NetworkPortParameter, occupied.serverPort());
    QVERIFY(!rtu.connectDevice());
    QCOMPARE(rtu.error(0), QModbusDevice::ConnectionError);
}

void TestModbusTransports::servesRtuOverTcpRequests()
{
    ModbusRtuTcpServer server;
    const quint16 port = startServer(server);
    QVERIFY(port != 0);
    QCOMPARE(server.connectionParameter(QModbusDevice::NetworkPortParameter).toUInt(), uint(port));
    QVERIFY(!server.connectionParameter(QModbusDevice::SerialPortNameParameter).isValid());

    QSignalSpy connectedSpy(&server, &ModbusRtuTcpServer::modbusClientConnected);
    QSignalSpy requestSpy(&server, &ModbusServer::modbusRequest);
    QSignalSpy responseSpy(&server, &ModbusServer::modbusResponse);
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(socket.waitForConnected(1000));
    QTRY_COMPARE(server.connectedClientCount(), 1);
    QCOMPARE(connectedSpy.count(), 1);

    const QByteArray frame = rtuFrame(
        1, QModbusRequest(QModbusRequest::ReadHoldingRegisters, quint16(0), quint16(2)));
    socket.write(frame.left(3));
    QVERIFY(socket.waitForBytesWritten(1000));
    QTest::qWait(5);
    socket.write(frame.mid(3));
    const QByteArray response = receive(socket);
    QVERIFY(response.size() >= 7);
    QCOMPARE(quint8(response.at(0)), quint8(1));
    QVERIFY(QModbusAduRtu(response).matchingChecksum());
    QCOMPARE(requestSpy.count(), 1);
    QCOMPARE(responseSpy.count(), 1);

    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState)
        QVERIFY(socket.waitForDisconnected(1000));
    QTRY_COMPARE(server.connectedClientCount(), 0);
}

void TestModbusTransports::recoversRtuStreamAndProcessesBroadcast()
{
    ModbusRtuTcpServer server;
    const quint16 port = startServer(server);
    QVERIFY(port != 0);
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(socket.waitForConnected(1000));
    QTRY_COMPARE(server.connectedClientCount(), 1);

    QByteArray damaged = rtuFrame(
        1, QModbusRequest(QModbusRequest::ReadHoldingRegisters, quint16(0), quint16(1)));
    damaged[damaged.size() - 1] = char(quint8(damaged.at(damaged.size() - 1)) ^ 0xff);
    const QByteArray valid = rtuFrame(
        1, QModbusRequest(QModbusRequest::WriteSingleRegister,
                          quint16(4), quint16(0x7788)));
    QByteArray stream;
    stream.append(char(0xff));
    stream.append(char(0));
    stream.append(damaged);
    stream.append(valid);
    socket.write(stream);
    QVERIFY(!receive(socket).isEmpty());
    quint16 value = 0;
    QVERIFY(server.data(QModbusDataUnit::HoldingRegisters, 4, &value, 1));
    QCOMPARE(value, quint16(0x7788));

    const QByteArray broadcast = rtuFrame(
        0, QModbusRequest(QModbusRequest::WriteSingleRegister,
                          quint16(5), quint16(0x3344)));
    socket.write(broadcast);
    QVERIFY(socket.waitForBytesWritten(1000));
    QTest::qWait(30);
    QCOMPARE(socket.bytesAvailable(), qint64(0));
    QTRY_VERIFY(server.data(QModbusDataUnit::HoldingRegisters, 5, &value, 1));
    QCOMPARE(value, quint16(0x3344));
}

void TestModbusTransports::simulatesRtuOverTcpErrors()
{
    ModbusRtuTcpServer server;
    const quint16 port = startServer(server);
    QVERIFY(port != 0);
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(socket.waitForConnected(1000));

    ModbusDefinitions definitions;
    definitions.ErrorSimulations.setResponseIncorrectId(true);
    definitions.ErrorSimulations.setResponseIncorrectCrc(true);
    definitions.ErrorSimulations.setResponseDelay(true);
    definitions.ErrorSimulations.setResponseDelayTime(20);
    server.setDefinitions(definitions);
    socket.write(rtuFrame(1, QModbusRequest(QModbusRequest::ReadHoldingRegisters,
                                            quint16(0), quint16(1))));
    QByteArray response = receive(socket);
    QVERIFY(response.size() >= 7);
    QCOMPARE(quint8(response.at(0)), quint8(2));
    QVERIFY(!QModbusAduRtu(response).matchingChecksum());

    definitions.ErrorSimulations.setResponseIncorrectId(false);
    definitions.ErrorSimulations.setResponseIncorrectCrc(false);
    definitions.ErrorSimulations.setResponseDelay(false);
    definitions.ErrorSimulations.setResponseDeviceBusy(true);
    server.setDefinitions(definitions);
    socket.write(rtuFrame(1, QModbusRequest(QModbusRequest::ReadHoldingRegisters,
                                            quint16(0), quint16(1))));
    response = receive(socket);
    QVERIFY(response.size() >= 5);
    QVERIFY(quint8(response.at(1)) & QModbusPdu::ExceptionByte);

    definitions.ErrorSimulations.setResponseDeviceBusy(false);
    definitions.ErrorSimulations.setNoResponse(true);
    server.setDefinitions(definitions);
    socket.write(rtuFrame(1, QModbusRequest(QModbusRequest::ReadHoldingRegisters,
                                            quint16(0), quint16(1))));
    QVERIFY(socket.waitForBytesWritten(1000));
    QTest::qWait(30);
    QCOMPARE(socket.bytesAvailable(), qint64(0));
}

void TestModbusTransports::configuresSerialTransport()
{
    ModbusRtuSerialServer server;
    QVERIFY(!server.processesBroadcast());
    QCOMPARE(server.interFrameDelay(), 2000);
    server.setInterFrameDelay(10001);
    QCOMPARE(server.interFrameDelay(), 11000);

    server.setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                                  QStringLiteral("nonexistent-openmodsim-port"));
    server.setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data7);
    server.setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::OddParity);
    server.setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QSerialPort::TwoStop);
    server.setConnectionParameter(QModbusDevice::SerialBaudRateParameter, QSerialPort::Baud9600);
    QCOMPARE(server.connectionParameter(QModbusDevice::SerialPortNameParameter).toString(),
             QStringLiteral("nonexistent-openmodsim-port"));
    QCOMPARE(server.connectionParameter(QModbusDevice::SerialDataBitsParameter).toInt(),
             int(QSerialPort::Data7));
    QCOMPARE(server.connectionParameter(QModbusDevice::SerialParityParameter).toInt(),
             int(QSerialPort::OddParity));
    QCOMPARE(server.connectionParameter(QModbusDevice::SerialStopBitsParameter).toInt(),
             int(QSerialPort::TwoStop));
    QCOMPARE(server.connectionParameter(QModbusDevice::SerialBaudRateParameter).toInt(), 9600);
    QVERIFY(!server.connectionParameter(QModbusDevice::NetworkPortParameter).isValid());
    QVERIFY(!server.connectDevice());
    QCOMPARE(server.state(), QModbusDevice::UnconnectedState);
    QCOMPARE(server.error(0), QModbusDevice::ConnectionError);
    QVERIFY(server.interFrameDelay() >= 11000);
}

QTEST_MAIN(TestModbusTransports)
#include "test_modbustransports.moc"
