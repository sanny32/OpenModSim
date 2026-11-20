// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbusserver.cpp
/// \brief Unit tests for the common Modbus server request processor.
///

#include <QBuffer>
#include <QMetaEnum>
#include <QModbusDeviceIdentification>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryDir>

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
    void enronAddressSpace_data();
    void enronAddressSpace();
    void enronRequests_data();
    void enronRequests();
    void enronLimitsAndMapping();
    void persistsEnronMode();
    void managesAddressesStateAndErrors();
    void validatesOptions();
    void readsAndWritesMapData();
    void processesBasicRequests();
    void processesMultipleAndCombinedRequests();
    void processesDiagnosticsAndIdentification();
    void rejectsInvalidRequests();
    void autoAddsRequestedRanges();
    void rejectsRequestsToGapsBetweenUnitMaps();
    void invokesCustomHandler();
    void exercisesHeaderUtilities();
};

/// \brief Covers both address capacities outside the fixed 32-bit Enron ranges.
void TestModbusServer::enronAddressSpace_data()
{
    QTest::addColumn<AddressSpace>("space");
    QTest::addColumn<int>("address");
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("allowed");
    for (auto space : {AddressSpace::Addr5Digits, AddressSpace::Addr6Digits}) {
        const int capacity = space == AddressSpace::Addr5Digits ? 9999 : 65536;
        for (int address : {0, 4000, 6000, 8000, 9998, 9999, 10000, 65534, 65535}) {
            for (int count : {1, 2}) {
                const auto name = QString("%1-%2-%3").arg(capacity).arg(address).arg(count);
                QTest::newRow(qPrintable(name)) << space << address << count << (address + count <= capacity);
            }
        }
    }
}

/// \brief Applies the selected address capacity consistently to register reads and writes.
void TestModbusServer::enronAddressSpace()
{
    QFETCH(AddressSpace, space);
    QFETCH(int, address);
    QFETCH(int, count);
    QFETCH(bool, allowed);
    TestableModbusServer server;
    server.addServerAddress(1);
    ModbusDefinitions definitions;
    definitions.EnronModbus = true;
    definitions.AddrSpace = space;
    server.setDefinitions(definitions);
    ModbusDataUnitMap map;
    for (auto type : {QModbusDataUnit::HoldingRegisters, QModbusDataUnit::InputRegisters})
        map.addUnitMap(QUuid::createUuid(), type, address, qMin(count, 65536 - address));
    server.setMap(map, 1);

    for (auto code : {QModbusPdu::ReadHoldingRegisters, QModbusPdu::ReadInputRegisters}) {
        const auto response = server.processRequest(QModbusRequest(code, quint16(address), quint16(count)), 1);
        QCOMPARE(!response.isException(), allowed);
        if (allowed)
            QCOMPARE(response.data(), QByteArray(1, char(count * 2)) + QByteArray(count * 2, '\0'));
        else
            QCOMPARE(response.exceptionCode(), QModbusPdu::IllegalDataAddress);
    }

    const QByteArray writePayload = words({quint16(address), quint16(count)})
        + QByteArray(1, char(count * 2)) + QByteArray(count * 2, '\0');
    const auto multiple = server.processRequest(QModbusRequest(QModbusPdu::WriteMultipleRegisters, writePayload), 1);
    QCOMPARE(!multiple.isException(), allowed);
    if (!allowed)
        QCOMPARE(multiple.exceptionCode(), QModbusPdu::IllegalDataAddress);

    const auto combinedPayload = words({quint16(address), quint16(count), quint16(address), quint16(count)})
        + QByteArray(1, char(count * 2)) + QByteArray(count * 2, '\0');
    const auto combined = server.processRequest(QModbusRequest(QModbusPdu::ReadWriteMultipleRegisters, combinedPayload), 1);
    QCOMPARE(!combined.isException(), allowed);
    if (!allowed)
        QCOMPARE(combined.exceptionCode(), QModbusPdu::IllegalDataAddress);

    if (count == 1) {
        const auto single = server.processRequest(QModbusRequest(QModbusPdu::WriteSingleRegister, quint16(address), quint16(0x1234)), 1);
        QCOMPARE(!single.isException(), allowed);
        if (allowed) {
            QModbusDataUnit stored(QModbusDataUnit::HoldingRegisters, address, 1);
            QVERIFY(server.data(&stored, 1));
            QCOMPARE(stored.value(0), quint16(0x1234));
        } else {
            QCOMPARE(single.exceptionCode(), QModbusPdu::IllegalDataAddress);
        }
        const auto mask = server.processRequest(QModbusRequest(QModbusPdu::MaskWriteRegister,
            quint16(address), quint16(0xffff), quint16(0)), 1);
        QCOMPARE(!mask.isException(), allowed);
        if (!allowed)
            QCOMPARE(mask.exceptionCode(), QModbusPdu::IllegalDataAddress);
    }

    if (count == 2 && address + count <= 65536) {
        QVERIFY(server.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, address, {1, 0x55}), 1));
        const auto fifo = server.processRequest(QModbusRequest(QModbusPdu::ReadFifoQueue, quint16(address)), 1);
        QCOMPARE(!fifo.isException(), allowed);
        if (allowed)
            QCOMPARE(fifo.data(), QByteArray::fromHex("000400010055"));
        else
            QCOMPARE(fifo.exceptionCode(), QModbusPdu::IllegalDataAddress);
    }

    if (space == AddressSpace::Addr5Digits && address == 10000 && count == 1) {
        definitions.AddrSpace = AddressSpace::Addr6Digits;
        server.setDefinitions(definitions);
        const QModbusRequest read(QModbusPdu::ReadHoldingRegisters, quint16(address), quint16(1));
        QVERIFY(!server.processRequest(read, 1).isException());
        definitions.AddrSpace = AddressSpace::Addr5Digits;
        server.setDefinitions(definitions);
        QCOMPARE(server.processRequest(read, 1).exceptionCode(), QModbusPdu::IllegalDataAddress);
    }
}

/// \brief Covers Enron wire payloads and validation independently of the transport.
void TestModbusServer::enronRequests_data()
{
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<QByteArray>("requestHex");
    QTest::addColumn<QByteArray>("responseHex");

    QTest::newRow("standard-default") << false << QByteArray("0313890002") << QByteArray("030411223344");
    QTest::newRow("long-pair") << true << QByteArray("0313890002") << QByteArray("03081122334455667788");
    QTest::newRow("second-long") << true << QByteArray("03138a0001") << QByteArray("030455667788");
    QTest::newRow("float-pair") << true << QByteArray("031b590002") << QByteArray("03083f80000040000000");
    QTest::newRow("input-registers") << true << QByteArray("0413890001") << QByteArray("040400000000");
    QTest::newRow("short-registers") << true << QByteArray("030bb90002") << QByteArray("030412345678");
    QTest::newRow("single-long") << true << QByteArray("06138a12345678") << QByteArray("06138a12345678");
    QTest::newRow("multiple-floats") << true << QByteArray("101b590002083f80000040000000") << QByteArray("101b590002");
    QTest::newRow("combined") << true << QByteArray("1713890002138a000104abcdef01") << QByteArray("170811223344abcdef01");
    QTest::newRow("single-short") << true << QByteArray("060bb9abcd") << QByteArray("060bb9abcd");
    QTest::newRow("multiple-short") << true << QByteArray("100bb900020412345678") << QByteArray("100bb90002");
    QTest::newRow("missing-long-word") << true << QByteArray("0613891234") << QByteArray("8603");
    QTest::newRow("trailing-single-data") << true << QByteArray("0613891234567800") << QByteArray("8603");
    QTest::newRow("wrong-byte-count") << true << QByteArray("1013890001021234") << QByteArray("9003");
    QTest::newRow("truncated-multiple") << true << QByteArray("1013890001041234") << QByteArray("9003");
    QTest::newRow("crosses-long-start") << true << QByteArray("0313880002") << QByteArray("8302");
    QTest::newRow("crosses-long-end") << true << QByteArray("03176f0002") << QByteArray("8302");
    QTest::newRow("crosses-float-end") << true << QByteArray("031f3f0002") << QByteArray("8302");
    QTest::newRow("crosses-write-end") << true << QByteArray("10176f0002080000000000000000") << QByteArray("9002");
    QTest::newRow("crosses-combined-read-end") << true << QByteArray("17176f0002138900010400000000") << QByteArray("9702");
    QTest::newRow("zero-write-quantity") << true << QByteArray("101389000000") << QByteArray("9003");
    QTest::newRow("zero-quantity") << true << QByteArray("0313890000") << QByteArray("8303");
    QTest::newRow("read-over-limit") << true << QByteArray("031389003f") << QByteArray("8303");
    QTest::newRow("mask-long") << true << QByteArray("161389ffff0000") << QByteArray("9601");
    QTest::newRow("fifo-long") << true << QByteArray("181389") << QByteArray("9801");
    QTest::newRow("coil-address-unaffected") << true << QByteArray("0113890001") << QByteArray("010100");
}

/// \brief Checks that Enron quantities preserve independent 32-bit values.
void TestModbusServer::enronRequests()
{
    QFETCH(bool, enabled);
    QFETCH(QByteArray, requestHex);
    QFETCH(QByteArray, responseHex);
    TestableModbusServer server;
    server.addServerAddress(1);
    ModbusDataUnitMap map;
    for (auto type : {QModbusDataUnit::HoldingRegisters, QModbusDataUnit::InputRegisters, QModbusDataUnit::Coils})
        map.addUnitMap(QUuid::createUuid(), type, 0, 10000);
    server.setMap(map, 1);
    ModbusDefinitions definitions;
    definitions.EnronModbus = enabled;
    server.setDefinitions(definitions);
    QVERIFY(server.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 5001, {0x1122, 0x3344, 0x5566, 0x7788}), 1));
    QVERIFY(server.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 7001, {0x3f80, 0, 0x4000, 0}), 1));
    QVERIFY(server.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 3001, {0x1234, 0x5678}), 1));

    const QByteArray bytes = QByteArray::fromHex(requestHex);
    const auto code = QModbusPdu::FunctionCode(quint8(bytes.at(0)));
    const auto response = server.processRequest(QModbusRequest(code, bytes.mid(1)), 1);
    QByteArray actual(1, char(response.functionCode() | (response.isException() ? 0x80 : 0)));
    actual += response.data();
    QCOMPARE(actual.toHex(), responseHex);
    if (!response.isException() && code == QModbusPdu::WriteSingleRegister && bytes.size() == 7) {
        QModbusDataUnit stored(QModbusDataUnit::HoldingRegisters, 5001, 4);
        QVERIFY(server.data(&stored, 1));
        QCOMPARE(stored.values(), QVector<quint16>({0x1122, 0x3344, 0x1234, 0x5678}));
    }
}

/// \brief Checks PDU limits, range endpoints, missing maps and runtime mode changes.
void TestModbusServer::enronLimitsAndMapping()
{
    TestableModbusServer server;
    server.addServerAddress(1);
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 5001, 1998);
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 7001, 1998);
    server.setMap(map, 1);
    ModbusDefinitions definitions;
    definitions.EnronModbus = true;
    server.setDefinitions(definitions);
    for (quint16 address : {5001, 5999, 7001, 7999}) {
        const auto response = server.processRequest(QModbusRequest(QModbusPdu::WriteSingleRegister, address, quint16(0x1234), quint16(0x5678)), 1);
        QVERIFY(!response.isException());
        QCOMPARE(server.processRequest(QModbusRequest(QModbusPdu::ReadHoldingRegisters, address, quint16(1)), 1).data(), QByteArray::fromHex("0412345678"));
    }
    const QModbusRequest read(QModbusPdu::ReadHoldingRegisters, quint16(5001), quint16(62));
    QCOMPARE(server.processRequest(read, 1).dataSize(), 249);
    const QModbusRequest write(QModbusPdu::WriteMultipleRegisters, QByteArray::fromHex("1389003df4") + QByteArray(244, '\0'));
    QVERIFY(!server.processRequest(write, 1).isException());
    const QModbusRequest tooLarge(QModbusPdu::WriteMultipleRegisters, QByteArray::fromHex("1389003ef8") + QByteArray(248, '\0'));
    QCOMPARE(server.processRequest(tooLarge, 1).exceptionCode(), QModbusPdu::IllegalDataValue);
    const QModbusRequest missing(QModbusPdu::ReadHoldingRegisters, quint16(9000), quint16(1));
    QCOMPARE(server.processRequest(missing, 1).exceptionCode(), QModbusPdu::IllegalDataAddress);
    definitions.EnronModbus = false;
    server.setDefinitions(definitions);
    QCOMPARE(server.processRequest(read, 1).dataSize(), 125);
}

/// \brief Keeps Enron mode across settings and XML round trips while defaulting legacy projects to standard Modbus.
void TestModbusServer::persistsEnronMode()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings settings(dir.filePath("settings.ini"), QSettings::IniFormat);
    ModbusDefinitions original;
    QVERIFY(!original.EnronModbus);
    original.EnronModbus = true;
    settings << original;
    ModbusDefinitions loaded;
    settings >> loaded;
    QVERIFY(loaded.EnronModbus);

    QByteArray bytes;
    QXmlStreamWriter writer(&bytes);
    writer << original;
    QXmlStreamReader reader(bytes);
    QVERIFY(reader.readNextStartElement());
    loaded.EnronModbus = false;
    reader >> loaded;
    QVERIFY(!reader.hasError());
    QVERIFY(loaded.EnronModbus);
    QXmlStreamReader legacy("<ModbusDefinitions/>");
    QVERIFY(legacy.readNextStartElement());
    legacy >> loaded;
    QVERIFY(!loaded.EnronModbus);
    settings.clear();
    loaded.EnronModbus = true;
    settings >> loaded;
    QVERIFY(!loaded.EnronModbus);
}

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

void TestModbusServer::rejectsRequestsToGapsBetweenUnitMaps()
{
    TestableModbusServer server;
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 0, 1);
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 10, 1);
    server.addServerAddress(1);
    server.setMap(map, 1);

    QModbusResponse response = server.processRequest(
        QModbusRequest(QModbusRequest::ReadHoldingRegisters, quint16(10), quint16(1)), 1);
    QVERIFY(!isException(response));

    response = server.processRequest(
        QModbusRequest(QModbusRequest::ReadHoldingRegisters, quint16(5), quint16(1)), 1);
    QVERIFY(isException(response));
    QCOMPARE(QModbusExceptionResponse(response).exceptionCode(), QModbusExceptionResponse::IllegalDataAddress);

    response = server.processRequest(
        QModbusRequest(QModbusRequest::ReadHoldingRegisters, quint16(0), quint16(11)), 1);
    QCOMPARE(QModbusExceptionResponse(response).exceptionCode(), QModbusExceptionResponse::IllegalDataAddress);

    response = server.processRequest(
        QModbusRequest(QModbusRequest::WriteSingleRegister, quint16(5), quint16(42)), 1);
    QCOMPARE(QModbusExceptionResponse(response).exceptionCode(), QModbusExceptionResponse::IllegalDataAddress);

    QVERIFY(server.setData(QModbusDataUnit::HoldingRegisters, 5, 42, 1));
    quint16 value = 0;
    QVERIFY(server.data(QModbusDataUnit::HoldingRegisters, 5, &value, 1));
    QCOMPARE(value, quint16(42));
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

void TestModbusServer::exercisesHeaderUtilities()
{
    TestableModbusServer server;
    QVERIFY(!server.processesBroadcast());
    QVERIFY(server.metaObject()->indexOfEnumerator("Option") >= 0);
    QVERIFY(QMetaEnum::fromType<ModbusServer::Option>().isValid());

    QModbusCommEvent event = QModbusCommEvent::ReceiveEvent;
    event |= QModbusCommEvent::ReceiveFlag::BroadcastReceived;
    QCOMPARE(quint8(event), quint8(0xc0));
    QCOMPARE(static_cast<QModbusCommEvent::EventByte>(event),
             QModbusCommEvent::EventByte(0xc0));

    event = QModbusCommEvent::SentEvent;
    event |= QModbusCommEvent::SendFlag::ServerBusyExceptionSent;
    QCOMPARE(quint8(event), quint8(0x44));
    QCOMPARE(quint8(QModbusCommEvent::ReceiveEvent
                    | QModbusCommEvent::ReceiveFlag::CommunicationError), quint8(0x82));
    QCOMPARE(quint8(QModbusCommEvent::ReceiveFlag::CharacterOverrun
                    | QModbusCommEvent::ReceiveEvent), quint8(0x90));
    QCOMPARE(quint8(QModbusCommEvent::SentEvent
                    | QModbusCommEvent::SendFlag::ReadExceptionSent), quint8(0x41));
    QCOMPARE(quint8(QModbusCommEvent::SendFlag::ServerAbortExceptionSent
                    | QModbusCommEvent::SentEvent), quint8(0x42));
}

QTEST_MAIN(TestModbusServer)
#include "test_modbusserver.moc"
