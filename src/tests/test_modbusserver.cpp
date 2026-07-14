// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbusserver.cpp
/// \brief Unit tests for the common Modbus server request processor.
///

#include <QBuffer>
#include <QModbusDeviceIdentification>
#include <QSignalSpy>
#include <QTest>

#include "modbusserver.h"

namespace {

QByteArray words(std::initializer_list<quint16> values)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    for (quint16 value : values)
        stream << value;
    return result;
}

ModbusDataUnitMap completeMap()
{
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::Coils, 0, 128);
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::DiscreteInputs, 0, 128);
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::InputRegisters, 0, 128);
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 0, 128);
    return map;
}

bool isException(const QModbusResponse& response)
{
    return response.isException();
}

}

class TestableModbusServer : public ModbusServer
{
public:
    using ModbusServer::clearCurrentRequestClient;
    using ModbusServer::matchingServerAddress;
    using ModbusServer::processRequest;
    using ModbusServer::setCurrentRequestClient;
    using ModbusServer::setError;
    using ModbusServer::storeModbusCommEvent;

    QVariant connectionParameter(QModbusDevice::ConnectionParameter parameter) const override
    {
        return _parameters.value(parameter);
    }

    void setConnectionParameter(QModbusDevice::ConnectionParameter parameter,
                                const QVariant& value) override
    {
        _parameters.insert(parameter, value);
    }

    QIODevice* device() const override { return const_cast<QBuffer*>(&_device); }

    void setOpenResult(bool result) { _openResult = result; }

protected:
    bool open() override
    {
        if (_openResult)
            setState(QModbusDevice::ConnectedState);
        return _openResult;
    }

    void close() override { setState(QModbusDevice::UnconnectedState); }

private:
    QHash<int, QVariant> _parameters;
    mutable QBuffer _device;
    bool _openResult = true;
};

class TestModbusServer : public QObject
{
    Q_OBJECT

private slots:
    void managesAddressesStateAndErrors();
    void validatesOptions();
    void readsAndWritesMapData();
    void processesBasicRequests();
    void processesMultipleAndCombinedRequests();
    void processesDiagnosticsAndIdentification();
    void rejectsInvalidRequests();
    void autoAddsRequestedRanges();
    void invokesCustomHandler();
};

void TestModbusServer::managesAddressesStateAndErrors()
{
    TestableModbusServer server;
    QSignalSpy stateSpy(&server, &ModbusServer::stateChanged);
    QSignalSpy errorSpy(&server, &ModbusServer::errorOccurred);

    server.addServerAddress(1);
    server.addServerAddress(1);
    server.addServerAddress(2);
    QVERIFY(server.hasServerAddress(1));
    QCOMPARE(server.serverAddresses().size(), 2);
    QVERIFY(server.matchingServerAddress(2));
    QVERIFY(!server.matchingServerAddress(3));

    server.setConnectionParameter(QModbusDevice::NetworkPortParameter, 1502);
    QCOMPARE(server.connectionParameter(QModbusDevice::NetworkPortParameter).toInt(), 1502);
    QVERIFY(server.connectDevice());
    QCOMPARE(server.state(), QModbusDevice::ConnectedState);
    QVERIFY(!server.connectDevice());
    server.disconnectDevice();
    QCOMPARE(server.state(), QModbusDevice::UnconnectedState);
    server.disconnectDevice();

    server.setOpenResult(false);
    QVERIFY(!server.connectDevice());
    QCOMPARE(server.state(), QModbusDevice::UnconnectedState);
    QVERIFY(stateSpy.count() >= 6);

    server.setError(QStringLiteral("failure"), QModbusDevice::ReadError, 1);
    QCOMPARE(server.error(1), QModbusDevice::ReadError);
    QCOMPARE(server.errorString(1), QStringLiteral("failure"));
    QCOMPARE(errorSpy.count(), 1);

    server.removeServerAddress(1);
    QVERIFY(server.hasServerAddress(1));
    server.removeServerAddress(1);
    QVERIFY(!server.hasServerAddress(1));
    QCOMPARE(server.error(1), QModbusDevice::NoError);
    server.removeAllServerAddresses();
    QVERIFY(server.serverAddresses().isEmpty());
}

void TestModbusServer::validatesOptions()
{
    TestableModbusServer server;
    server.setMap(completeMap(), 1);

    QCOMPARE(server.value(ModbusServer::DiagnosticRegister, 1).toUInt(), 0u);
    QCOMPARE(server.value(ModbusServer::AsciiInputDelimiter, 1).toChar(), QChar('\n'));
    QCOMPARE(server.value(ModbusServer::ServerIdentifier, 1).toUInt(), 10u);
    QCOMPARE(server.value(ModbusServer::RunIndicatorStatus, 1).toUInt(), 255u);
    QCOMPARE(server.value(ModbusServer::AdditionalData, 1).toByteArray(),
             QByteArray("Qt Modbus Server"));
    QVERIFY(!server.value(99, 1).isValid());

    QVERIFY(server.setValue(ModbusServer::DiagnosticRegister, 7, 1));
    QVERIFY(!server.setValue(ModbusServer::DiagnosticRegister, QStringLiteral("7"), 1));
    QVERIFY(server.setValue(ModbusServer::ExceptionStatusOffset, 4, 1));
    QVERIFY(!server.setValue(ModbusServer::ExceptionStatusOffset, 200, 2));
    QVERIFY(server.setValue(ModbusServer::DeviceBusy, 0xffff, 1));
    QVERIFY(!server.setValue(ModbusServer::DeviceBusy, 1, 1));
    QVERIFY(server.setValue(ModbusServer::AsciiInputDelimiter, 0x7f, 1));
    QVERIFY(!server.setValue(ModbusServer::AsciiInputDelimiter, 0x100, 1));
    QVERIFY(server.setValue(ModbusServer::ListenOnlyMode, true, 1));
    QVERIFY(!server.setValue(ModbusServer::ListenOnlyMode, 1, 1));
    QVERIFY(server.setValue(ModbusServer::ServerIdentifier, 42, 1));
    QVERIFY(server.setValue(ModbusServer::RunIndicatorStatus, 0, 1));
    QVERIFY(!server.setValue(ModbusServer::RunIndicatorStatus, 1, 1));
    QVERIFY(server.setValue(ModbusServer::AdditionalData, QByteArray("data"), 1));
    QVERIFY(!server.setValue(ModbusServer::AdditionalData, QByteArray(250, 'x'), 1));
    QVERIFY(!server.setValue(ModbusServer::AdditionalData, QStringLiteral("data"), 1));
    QVERIFY(server.setValue(ModbusServer::UserOption + 1, QStringLiteral("custom"), 1));
    QCOMPARE(server.value(ModbusServer::UserOption + 1, 1).toString(), QStringLiteral("custom"));
    QVERIFY(!server.setValue(99, 1, 1));
}

void TestModbusServer::readsAndWritesMapData()
{
    TestableModbusServer server;
    server.setMap(completeMap(), 1);
    QSignalSpy writtenSpy(&server, &ModbusServer::dataWritten);

    QVERIFY(server.setData(QModbusDataUnit::HoldingRegisters, 5, 0x1234, 1));
    quint16 value = 0;
    QVERIFY(server.data(QModbusDataUnit::HoldingRegisters, 5, &value, 1));
    QCOMPARE(value, quint16(0x1234));
    QVERIFY(!server.data(QModbusDataUnit::HoldingRegisters, 200, &value, 1));
    QVERIFY(!server.data(QModbusDataUnit::HoldingRegisters, 5, nullptr, 1));
    QVERIFY(!server.setData(QModbusDataUnit::Invalid, 0, 1, 1));

    QModbusDataUnit range(QModbusDataUnit::HoldingRegisters, 4,
                         QVector<quint16>{1, 2, 3});
    QVERIFY(server.setData(range, 1));
    QModbusDataUnit readRange(QModbusDataUnit::HoldingRegisters, 4, 3);
    QVERIFY(server.data(&readRange, 1));
    QCOMPARE(readRange.value(0), quint16(1));
    QCOMPARE(readRange.value(2), quint16(3));

    QModbusDataUnit entire(QModbusDataUnit::HoldingRegisters);
    entire.setStartAddress(-1);
    QVERIFY(server.data(&entire, 1));
    QCOMPARE(entire.valueCount(), qsizetype(128));
    QVERIFY(!server.data(nullptr, 1));
    QModbusDataUnit outside(QModbusDataUnit::HoldingRegisters, 127, 2);
    QVERIFY(!server.data(&outside, 1));
    QVERIFY(!server.setData(outside, 1));
    QCOMPARE(writtenSpy.count(), 2);

    server.setCurrentRequestClient(QStringLiteral("127.0.0.1"), 5000);
    QVERIFY(server.setData(QModbusDataUnit::HoldingRegisters, 4, 1, 1));
    server.clearCurrentRequestClient();
}

void TestModbusServer::processesBasicRequests()
{
    TestableModbusServer server;
    server.setMap(completeMap(), 1);
    server.setData(QModbusDataUnit(QModbusDataUnit::Coils, 0,
                                  QVector<quint16>{1, 0, 1, 1, 0, 0, 1, 0, 1}), 1);
    server.setData(QModbusDataUnit(QModbusDataUnit::DiscreteInputs, 0,
                                  QVector<quint16>{0, 1, 1}), 1);
    server.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 0,
                                  QVector<quint16>{10, 20, 30, 40}), 1);
    server.setData(QModbusDataUnit(QModbusDataUnit::InputRegisters, 0,
                                  QVector<quint16>{50, 60, 70}), 1);

    const QList<QModbusRequest> reads = {
        QModbusRequest(QModbusRequest::ReadCoils, quint16(0), quint16(9)),
        QModbusRequest(QModbusRequest::ReadDiscreteInputs, quint16(0), quint16(3)),
        QModbusRequest(QModbusRequest::ReadHoldingRegisters, quint16(0), quint16(4)),
        QModbusRequest(QModbusRequest::ReadInputRegisters, quint16(0), quint16(3))
    };
    for (const QModbusRequest& request : reads) {
        const QModbusResponse response = server.processRequest(request, 1);
        QVERIFY(response.isValid());
        QVERIFY(!isException(response));
    }

    QModbusResponse response = server.processRequest(
        QModbusRequest(QModbusRequest::WriteSingleCoil, quint16(1), quint16(Coil::On)), 1);
    QVERIFY(!isException(response));
    quint16 value = 0;
    QVERIFY(server.data(QModbusDataUnit::Coils, 1, &value, 1));
    QCOMPARE(value, quint16(Coil::On));

    response = server.processRequest(
        QModbusRequest(QModbusRequest::WriteSingleRegister, quint16(2), quint16(0xabcd)), 1);
    QVERIFY(!isException(response));
    QVERIFY(server.data(QModbusDataUnit::HoldingRegisters, 2, &value, 1));
    QCOMPARE(value, quint16(0xabcd));

    QVERIFY(server.setValue(ModbusServer::ExceptionStatusOffset, 0, 1));
    response = server.processRequest(QModbusRequest(QModbusRequest::ReadExceptionStatus), 1);
    QVERIFY(!isException(response));
    response = server.processRequest(QModbusRequest(QModbusRequest::ReportServerId), 1);
    QVERIFY(!isException(response));
}

void TestModbusServer::processesMultipleAndCombinedRequests()
{
    TestableModbusServer server;
    server.setMap(completeMap(), 1);

    QByteArray data = words({0, 10});
    data.append(char(2));
    data.append(char(0x55));
    data.append(char(0x03));
    QModbusResponse response = server.processRequest(
        QModbusRequest(QModbusRequest::WriteMultipleCoils, data), 1);
    QVERIFY(!isException(response));

    data = words({4, 3});
    data.append(char(6));
    data.append(words({0x1111, 0x2222, 0x3333}));
    response = server.processRequest(
        QModbusRequest(QModbusRequest::WriteMultipleRegisters, data), 1);
    QVERIFY(!isException(response));

    response = server.processRequest(
        QModbusRequest(QModbusRequest::MaskWriteRegister, quint16(4), quint16(0xff00),
                       quint16(0x00aa)), 1);
    QVERIFY(!isException(response));

    data = words({4, 3, 8, 2});
    data.append(char(4));
    data.append(words({0xaaaa, 0xbbbb}));
    response = server.processRequest(
        QModbusRequest(QModbusRequest::ReadWriteMultipleRegisters, data), 1);
    QVERIFY(!isException(response));

    server.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 20,
                                  QVector<quint16>{3, 7, 8, 9}), 1);
    response = server.processRequest(
        QModbusRequest(QModbusRequest::ReadFifoQueue, quint16(20)), 1);
    QVERIFY(!isException(response));
}

void TestModbusServer::processesDiagnosticsAndIdentification()
{
    TestableModbusServer server;
    server.setMap(completeMap(), 1);
    server.setValue(ModbusServer::DeviceBusy, 0, 1);

    const QList<quint16> diagnostics = {
        Diagnostics::ReturnQueryData,
        Diagnostics::ReturnDiagnosticRegister,
        Diagnostics::ForceListenOnlyMode,
        Diagnostics::ClearCountersAndDiagnosticRegister,
        Diagnostics::ReturnBusMessageCount,
        Diagnostics::ReturnBusCommunicationErrorCount,
        Diagnostics::ReturnBusExceptionErrorCount,
        Diagnostics::ReturnServerMessageCount,
        Diagnostics::ReturnServerNoResponseCount,
        Diagnostics::ReturnServerNAKCount,
        Diagnostics::ReturnServerBusyCount,
        Diagnostics::ReturnBusCharacterOverrunCount,
        Diagnostics::ClearOverrunCounterAndFlag
    };
    for (quint16 subFunction : diagnostics) {
        const QModbusResponse response = server.processRequest(
            QModbusRequest(QModbusRequest::Diagnostics, subFunction, quint16(0)), 1);
        QVERIFY(!isException(response));
    }

    QVERIFY(!isException(server.processRequest(
        QModbusRequest(QModbusRequest::GetCommEventCounter), 1)));
    server.storeModbusCommEvent(QModbusCommEvent(QModbusCommEvent::ReceiveEvent));
    QVERIFY(!isException(server.processRequest(
        QModbusRequest(QModbusRequest::GetCommEventLog), 1)));

    QModbusDeviceIdentification identification;
    identification.setConformityLevel(QModbusDeviceIdentification::BasicConformityLevel);
    identification.insert(QModbusDeviceIdentification::VendorNameObjectId, "OpenModSim");
    identification.insert(QModbusDeviceIdentification::ProductCodeObjectId, "OMS");
    identification.insert(QModbusDeviceIdentification::MajorMinorRevisionObjectId, "1.0");
    QVERIFY(server.setValue(ModbusServer::DeviceIdentification,
                            QVariant::fromValue(identification), 1));

    for (quint8 code : {quint8(QModbusDeviceIdentification::BasicReadDeviceIdCode),
                        quint8(QModbusDeviceIdentification::RegularReadDeviceIdCode),
                        quint8(QModbusDeviceIdentification::ExtendedReadDeviceIdCode),
                        quint8(QModbusDeviceIdentification::IndividualReadDeviceIdCode)}) {
        QByteArray requestData;
        requestData.append(char(EncapsulatedInterfaceTransport::ReadDeviceIdentification));
        requestData.append(char(code));
        requestData.append(char(QModbusDeviceIdentification::VendorNameObjectId));
        const QModbusResponse response = server.processRequest(
            QModbusRequest(QModbusRequest::EncapsulatedInterfaceTransport, requestData), 1);
        QVERIFY(!isException(response));
    }
}

void TestModbusServer::rejectsInvalidRequests()
{
    TestableModbusServer server;
    server.setMap(completeMap(), 1);

    const QList<QModbusRequest> requests = {
        QModbusRequest(QModbusRequest::ReadCoils, quint16(127), quint16(2)),
        QModbusRequest(QModbusRequest::ReadHoldingRegisters, quint16(127), quint16(2)),
        QModbusRequest(QModbusRequest::WriteSingleCoil, quint16(0), quint16(1)),
        QModbusRequest(QModbusRequest::WriteSingleRegister, QByteArray(2, '\0')),
        QModbusRequest(QModbusRequest::ReadExceptionStatus, QByteArray(1, '\0')),
        QModbusRequest(QModbusRequest::Diagnostics, quint16(0xffff), quint16(0)),
        QModbusRequest(QModbusRequest::WriteMultipleCoils, QByteArray(5, '\0')),
        QModbusRequest(QModbusRequest::WriteMultipleRegisters, QByteArray(5, '\0')),
        QModbusRequest(QModbusRequest::ReadFileRecord),
        QModbusRequest(QModbusRequest::WriteFileRecord),
        QModbusRequest(QModbusRequest::MaskWriteRegister, quint16(200), quint16(0), quint16(0)),
        QModbusRequest(QModbusRequest::ReadFifoQueue, quint16(200)),
        QModbusRequest(static_cast<QModbusPdu::FunctionCode>(0x41), QByteArray())
    };
    for (const QModbusRequest& request : requests) {
        const QModbusResponse response = server.processRequest(request, 1);
        QVERIFY2(isException(response),
                 qPrintable(QStringLiteral("function 0x%1 data %2 returned 0x%3")
                                .arg(int(request.functionCode()), 2, 16, QLatin1Char('0'))
                                .arg(QString::fromLatin1(request.data().toHex()))
                                .arg(int(response.functionCode()), 2, 16, QLatin1Char('0'))));
    }

    QByteArray missingIdentification;
    missingIdentification.append(char(EncapsulatedInterfaceTransport::ReadDeviceIdentification));
    missingIdentification.append(char(QModbusDeviceIdentification::BasicReadDeviceIdCode));
    missingIdentification.append(char(0));
    QVERIFY(isException(server.processRequest(
        QModbusRequest(QModbusRequest::EncapsulatedInterfaceTransport, missingIdentification), 1)));
}

void TestModbusServer::autoAddsRequestedRanges()
{
    TestableModbusServer server;
    ModbusDefinitions definitions;
    definitions.AutoAddRegistersOnRequest = true;
    server.setDefinitions(definitions);

    QVERIFY(server.matchingServerAddress(77));
    QVERIFY(server.setData(QModbusDataUnit::HoldingRegisters, 10, 0, 77));
    QModbusResponse response = server.processRequest(
        QModbusRequest(QModbusRequest::ReadHoldingRegisters, quint16(10), quint16(2)), 77);
    QVERIFY(!isException(response));
    quint16 value = 1;
    QVERIFY(server.data(QModbusDataUnit::HoldingRegisters, 11, &value, 77));
    QCOMPARE(value, quint16(0));
}

void TestModbusServer::invokesCustomHandler()
{
    TestableModbusServer server;
    bool invoked = false;
    server.setRequestHandler(RequestHandlerPtr::create(
        [&invoked](const QModbusPdu& request, int address, QModbusResponse& response) {
            invoked = true;
            if (address != 9)
                return false;
            response = QModbusResponse(request.functionCode(), QByteArray("custom"));
            return true;
        }));

    const QModbusResponse response = server.processRequest(
        QModbusRequest(static_cast<QModbusPdu::FunctionCode>(0x41)), 9);
    QVERIFY(invoked);
    QVERIFY(!isException(response));
    QCOMPARE(response.data(), QByteArray("custom"));
}

QTEST_MAIN(TestModbusServer)
#include "test_modbusserver.moc"
