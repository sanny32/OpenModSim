// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbusmultiserver.cpp
/// \brief Unit tests for ModbusMultiServer metadata updates, bulk writes
/// and change-notification batching.
///

#include <QDateTime>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QUuid>

#include <initializer_list>

#include "modbusmultiserver.h"

namespace {

///
/// \brief Creates a metadata map key.
/// \param deviceId Modbus device identifier.
/// \param type Register type.
/// \param address Register address.
/// \return The initialized key.
///
ItemMapKey key(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 address)
{
    return {deviceId, type, address};
}

///
/// \brief Creates a data unit populated with register values.
/// \param type Register type.
/// \param address Starting address.
/// \param values Register values.
/// \return The initialized data unit.
///
QModbusDataUnit unit(QModbusDataUnit::RegisterType type, quint16 address,
                     std::initializer_list<quint16> values)
{
    QModbusDataUnit result(type, address, static_cast<quint16>(values.size()));
    result.setValues(QVector<quint16>(values));
    return result;
}

///
/// \brief Reserves and releases an available local TCP port.
/// \return The selected port, or zero when no port is available.
///
quint16 availablePort()
{
    QTcpServer probe;
    return probe.listen(QHostAddress::LocalHost, 0) ? probe.serverPort() : 0;
}

///
/// \brief Creates a Modbus TCP application data unit.
/// \param transactionId Transaction identifier.
/// \param address Modbus unit identifier.
/// \param request Request PDU.
/// \return The serialized TCP frame.
///
QByteArray tcpFrame(quint16 transactionId, quint8 address, const QModbusRequest& request)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream << transactionId << quint16(0) << quint16(request.size() + 1) << address << request;
    return result;
}

}

class TestModbusMultiServer : public QObject
{
    Q_OBJECT

private slots:
    void mergesDescriptionMaps();
    void replacesDescriptionMaps();
    void mergesTimestampMaps();
    void replacesTimestampMaps();
    void tracksDeviceIdReferences();
    void registersAndRemovesUnitMaps();
    void normalizesDefinitions();
    void readsWritesAndClearsData();
    void rejectsDataForUnknownDevice();
    void refreshesTimestampsForClientWrites();
    void managesIndividualMetadata();
    void filtersAndClearsMetadata();
    void roundTripsNumericTypes();
    void typedRoundTrips_data();
    void typedRoundTrips();
    void writesRegisterVariants();
    void writesUnsignedRegisterValues();
    void reportsDisconnectedState();
    void connectsTransportBackends();

    void bulkWriteEmitsSingleDataChanged();
    void bulkWriteEmitsSingleTimestampsChanged();
    void singleWriteEmitsPerAddressTimestamp();
    void unchangedValueEmitsNothing();
    void unknownDeviceIdReportsError();
    void bulkWriteAppliesByteOrder();
    void bulkWriteStoresEveryValue();

    void benchmarkSingleWrites();
    void benchmarkBulkWrite();

private:
    static void prepare(ModbusMultiServer& server, quint8 deviceId, quint16 length = 300);
};

void TestModbusMultiServer::mergesDescriptionMaps()
{
    ModbusMultiServer server;
    const auto retained = key(1, QModbusDataUnit::HoldingRegisters, 10);
    const auto updated = key(1, QModbusDataUnit::HoldingRegisters, 11);
    const auto added = key(2, QModbusDataUnit::Coils, 12);

    server.setDescriptionMap({{retained, QStringLiteral("retained")},
                              {updated, QStringLiteral("old")}},
                             WriteSource::ProjectLoad);
    server.setDescriptionMap({{updated, QStringLiteral("new")},
                              {added, QStringLiteral("added")}},
                             WriteSource::ProjectLoad, false);

    const auto descriptions = server.descriptionMap();
    QCOMPARE(descriptions.size(), 3);
    QCOMPARE(descriptions.value(retained), QStringLiteral("retained"));
    QCOMPARE(descriptions.value(updated), QStringLiteral("new"));
    QCOMPARE(descriptions.value(added), QStringLiteral("added"));
}

void TestModbusMultiServer::replacesDescriptionMaps()
{
    ModbusMultiServer server;
    const auto removed = key(1, QModbusDataUnit::HoldingRegisters, 10);
    const auto replacement = key(2, QModbusDataUnit::InputRegisters, 20);

    server.setDescriptionMap({{removed, QStringLiteral("old")}}, WriteSource::ProjectLoad);
    server.setDescriptionMap({{replacement, QStringLiteral("new")}}, WriteSource::ProjectLoad, true);

    const auto descriptions = server.descriptionMap();
    QCOMPARE(descriptions.size(), 1);
    QVERIFY(!descriptions.contains(removed));
    QCOMPARE(descriptions.value(replacement), QStringLiteral("new"));
}

void TestModbusMultiServer::mergesTimestampMaps()
{
    ModbusMultiServer server;
    const auto retained = key(1, QModbusDataUnit::HoldingRegisters, 10);
    const auto updated = key(1, QModbusDataUnit::HoldingRegisters, 11);
    const auto added = key(2, QModbusDataUnit::DiscreteInputs, 12);
    const auto first = QDateTime::fromString(QStringLiteral("2026-01-01T01:00:00Z"), Qt::ISODate);
    const auto second = QDateTime::fromString(QStringLiteral("2026-02-02T02:00:00Z"), Qt::ISODate);
    const auto third = QDateTime::fromString(QStringLiteral("2026-03-03T03:00:00Z"), Qt::ISODate);

    server.setTimestampMap({{retained, first}, {updated, first}});
    server.setTimestampMap({{updated, second}, {added, third}}, false);

    const auto timestamps = server.timestampMap();
    QCOMPARE(timestamps.size(), 3);
    QCOMPARE(timestamps.value(retained), first);
    QCOMPARE(timestamps.value(updated), second);
    QCOMPARE(timestamps.value(added), third);
}

void TestModbusMultiServer::replacesTimestampMaps()
{
    ModbusMultiServer server;
    const auto removed = key(1, QModbusDataUnit::HoldingRegisters, 10);
    const auto replacement = key(2, QModbusDataUnit::InputRegisters, 20);
    const auto oldTimestamp = QDateTime::fromString(QStringLiteral("2026-01-01T01:00:00Z"), Qt::ISODate);
    const auto newTimestamp = QDateTime::fromString(QStringLiteral("2026-02-02T02:00:00Z"), Qt::ISODate);

    server.setTimestampMap({{removed, oldTimestamp}});
    server.setTimestampMap({{replacement, newTimestamp}}, true);

    const auto timestamps = server.timestampMap();
    QCOMPARE(timestamps.size(), 1);
    QVERIFY(!timestamps.contains(removed));
    QCOMPARE(timestamps.value(replacement), newTimestamp);
}

void TestModbusMultiServer::tracksDeviceIdReferences()
{
    ModbusMultiServer server;
    QSignalSpy changedSpy(&server, &ModbusMultiServer::deviceIdsChanged);
    QSignalSpy addedSpy(&server, &ModbusMultiServer::deviceIdAdded);
    QSignalSpy removedSpy(&server, &ModbusMultiServer::deviceIdRemoved);

    server.addDeviceId(7);
    server.addDeviceId(7);
    server.addDeviceId(2);

    QCOMPARE(addedSpy.count(), 2);
    QCOMPARE(changedSpy.count(), 2);
    QCOMPARE(changedSpy.last().at(0).value<QList<int>>(), QList<int>({2, 7}));

    server.removeDeviceId(7);
    QCOMPARE(removedSpy.count(), 0);
    server.removeDeviceId(7);
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(changedSpy.last().at(0).value<QList<int>>(), QList<int>({2}));
    server.removeDeviceId(42);
    QCOMPARE(removedSpy.count(), 1);
}

void TestModbusMultiServer::registersAndRemovesUnitMaps()
{
    ModbusMultiServer server;
    QSignalSpy addedSpy(&server, &ModbusMultiServer::unitMapAdded);
    QSignalSpy removedSpy(&server, &ModbusMultiServer::unitMapRemoved);
    const auto id = QUuid::createUuid();

    QVERIFY(!server.useGlobalUnitMap());
    server.addUnitMap(id, 3, QModbusDataUnit::HoldingRegisters, 10, 4);
    QCOMPARE(server.registeredDeviceIds(), QList<int>({3}));
    QCOMPARE(addedSpy.count(), 1);
    QVERIFY(server.data(3, QModbusDataUnit::HoldingRegisters, 10, 4).isValid());

    server.addUnitMap(id, 3, QModbusDataUnit::HoldingRegisters, 10, 4);
    QCOMPARE(addedSpy.count(), 1);
    server.removeUnitMap(QUuid::createUuid(), 3);
    QCOMPARE(removedSpy.count(), 0);
    server.removeUnitMap(id, 3);
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.first().at(1).toInt(), 3);
}

void TestModbusMultiServer::normalizesDefinitions()
{
    ModbusMultiServer server;
    QSignalSpy definitionsSpy(&server, &ModbusMultiServer::definitionsChanged);
    ModbusDefinitions definitions;
    definitions.UseGlobalUnitMap = true;
    definitions.AutoAddRegistersOnRequest = true;

    server.setModbusDefinitions(definitions);

    QCOMPARE(definitionsSpy.count(), 1);
    const auto stored = server.getModbusDefinitions();
    QVERIFY(stored.AutoAddRegistersOnRequest);
    QVERIFY(!stored.UseGlobalUnitMap);

    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::Coils, 0, 2);
    server.setUseGlobalUnitMap(true);
    QVERIFY(server.useGlobalUnitMap());
    server.setUseGlobalUnitMap(false);
    QVERIFY(!server.useGlobalUnitMap());
}

void TestModbusMultiServer::readsWritesAndClearsData()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 10, 3);
    QSignalSpy dataSpy(&server, &ModbusMultiServer::dataChanged);

    const auto values = unit(QModbusDataUnit::HoldingRegisters, 10, {11, 22, 33});
    server.setData(1, values, WriteSource::Simulator);

    QCOMPARE(dataSpy.count(), 1);
    const auto stored = server.data(1, QModbusDataUnit::HoldingRegisters, 10, 3);
    QCOMPARE(int(stored.valueCount()), 3);
    QCOMPARE(stored.value(0), quint16(11));
    QCOMPARE(stored.value(1), quint16(22));
    QCOMPARE(stored.value(2), quint16(33));
    server.setData(1, values, WriteSource::Simulator);
    QCOMPARE(dataSpy.count(), 1);

    server.clearAddressSpace();
    QVERIFY(!server.data(1, QModbusDataUnit::HoldingRegisters, 10, 1).isValid());
    QVERIFY(server.registeredDeviceIds().isEmpty());
}

void TestModbusMultiServer::rejectsDataForUnknownDevice()
{
    ModbusMultiServer server;
    QSignalSpy errorSpy(&server, &ModbusMultiServer::errorOccured);

    server.setData(9, unit(QModbusDataUnit::HoldingRegisters, 0, {1}));

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.first().at(0).toInt(), 9);
    QVERIFY(errorSpy.first().at(1).toString().contains(QStringLiteral("9")));
}

void TestModbusMultiServer::refreshesTimestampsForClientWrites()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 1);
    const auto values = unit(QModbusDataUnit::HoldingRegisters, 0, {15});
    server.setData(1, values);
    const auto previous = server.timestamp(1, QModbusDataUnit::HoldingRegisters, 0);
    QSignalSpy dataSpy(&server, &ModbusMultiServer::dataChanged);
    QSignalSpy timestampSpy(&server, &ModbusMultiServer::timestampChanged);

    QTest::qWait(2);
    server.setData(1, values, WriteSource::ModbusClient);

    QCOMPARE(dataSpy.count(), 1);
    QVERIFY(server.timestamp(1, QModbusDataUnit::HoldingRegisters, 0).isValid());
    QVERIFY(server.timestamp(1, QModbusDataUnit::HoldingRegisters, 0) > previous);
    QCOMPARE(timestampSpy.count(), 1);
}

void TestModbusMultiServer::managesIndividualMetadata()
{
    ModbusMultiServer server;
    QSignalSpy descriptionSpy(&server, &ModbusMultiServer::descriptionChanged);
    QSignalSpy timestampSpy(&server, &ModbusMultiServer::timestampChanged);
    const auto timestamp = QDateTime::fromString(QStringLiteral("2026-04-05T06:07:08Z"), Qt::ISODate);

    server.setDescription(4, QModbusDataUnit::InputRegisters, 12,
                          QStringLiteral("pressure"), WriteSource::User);
    server.setDescription(4, QModbusDataUnit::InputRegisters, 12,
                          QStringLiteral("pressure"), WriteSource::Simulator);
    server.setTimestamp(4, QModbusDataUnit::InputRegisters, 12, timestamp);
    server.setTimestamp(4, QModbusDataUnit::InputRegisters, 12, timestamp);

    QCOMPARE(descriptionSpy.count(), 1);
    QCOMPARE(timestampSpy.count(), 1);
    QCOMPARE(server.description(4, QModbusDataUnit::InputRegisters, 12), QStringLiteral("pressure"));
    QCOMPARE(server.timestamp(4, QModbusDataUnit::InputRegisters, 12), timestamp);
    QVERIFY(server.description(99, QModbusDataUnit::Coils, 0).isEmpty());
    QVERIFY(!server.timestamp(99, QModbusDataUnit::Coils, 0).isValid());
}

void TestModbusMultiServer::filtersAndClearsMetadata()
{
    ModbusMultiServer server;
    const auto first = key(1, QModbusDataUnit::HoldingRegisters, 10);
    const auto second = key(1, QModbusDataUnit::HoldingRegisters, 11);
    const auto other = key(2, QModbusDataUnit::Coils, 10);
    const auto timestamp = QDateTime::fromString(QStringLiteral("2026-05-06T07:08:09Z"), Qt::ISODate);
    server.setDescriptionMap({{first, QStringLiteral("first")},
                              {second, QStringLiteral("second")},
                              {other, QStringLiteral("other")},
                              {key(3, QModbusDataUnit::Coils, 1), QString()}},
                             WriteSource::ProjectLoad);
    server.setTimestampMap({{first, timestamp}, {second, timestamp}, {other, timestamp},
                            {key(3, QModbusDataUnit::Coils, 1), QDateTime()}});

    QCOMPARE(server.descriptionMap(1, QModbusDataUnit::HoldingRegisters, 10, 1).size(), 1);
    QCOMPARE(server.timestampMap(1, QModbusDataUnit::HoldingRegisters, 10, 2).size(), 2);
    QVERIFY(server.descriptionMap(99, QModbusDataUnit::Coils, 0, 1).isEmpty());
    QVERIFY(server.timestampMap(99, QModbusDataUnit::Coils, 0, 1).isEmpty());

    QSignalSpy timestampSpy(&server, &ModbusMultiServer::timestampsChanged);
    server.clearDescriptions();
    server.clearTimestamps();
    QVERIFY(server.descriptionMap().isEmpty());
    QVERIFY(server.timestampMap().isEmpty());
    QCOMPARE(timestampSpy.count(), 1);
}

void TestModbusMultiServer::roundTripsNumericTypes()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 64);

    server.writeValue(1, QModbusDataUnit::HoldingRegisters, 0, 0x1234, ByteOrder::Swapped);
    QCOMPARE(server.data(1, QModbusDataUnit::HoldingRegisters, 0, 1).value(0), quint16(0x3412));

    server.writeInt32(1, QModbusDataUnit::HoldingRegisters, 2, -1234567, ByteOrder::Direct, false);
    QCOMPARE(server.readInt32(1, QModbusDataUnit::HoldingRegisters, 2, ByteOrder::Direct, false), qint32(-1234567));
    server.writeUInt32(1, QModbusDataUnit::HoldingRegisters, 4, 0xFEDCBA98u, ByteOrder::Swapped, true);
    QCOMPARE(server.readUInt32(1, QModbusDataUnit::HoldingRegisters, 4, ByteOrder::Swapped, true), quint32(0xFEDCBA98u));
    server.writeInt64(1, QModbusDataUnit::HoldingRegisters, 8, -0x123456789LL, ByteOrder::Direct, true);
    QCOMPARE(server.readInt64(1, QModbusDataUnit::HoldingRegisters, 8, ByteOrder::Direct, true), qint64(-0x123456789LL));
    server.writeUInt64(1, QModbusDataUnit::HoldingRegisters, 12, 0xFEDCBA9876543210ULL, ByteOrder::Swapped, false);
    QCOMPARE(server.readUInt64(1, QModbusDataUnit::HoldingRegisters, 12, ByteOrder::Swapped, false), quint64(0xFEDCBA9876543210ULL));
    server.writeFloat(1, QModbusDataUnit::HoldingRegisters, 20, 123.25f, ByteOrder::Direct, true);
    QCOMPARE(server.readFloat(1, QModbusDataUnit::HoldingRegisters, 20, ByteOrder::Direct, true), 123.25f);
    server.writeDouble(1, QModbusDataUnit::HoldingRegisters, 24, -9876.5, ByteOrder::Swapped, false);
    QCOMPARE(server.readDouble(1, QModbusDataUnit::HoldingRegisters, 24, ByteOrder::Swapped, false), -9876.5);
}

void TestModbusMultiServer::typedRoundTrips_data()
{
    QTest::addColumn<ByteOrder>("order");
    QTest::addColumn<bool>("swapped");

    QTest::newRow("direct-msrf") << ByteOrder::Direct << false;
    QTest::newRow("direct-lsrf") << ByteOrder::Direct << true;
    QTest::newRow("swapped-msrf") << ByteOrder::Swapped << false;
    QTest::newRow("swapped-lsrf") << ByteOrder::Swapped << true;
}

void TestModbusMultiServer::typedRoundTrips()
{
    QFETCH(ByteOrder, order);
    QFETCH(bool, swapped);

    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 64);

    server.writeInt32(1, QModbusDataUnit::HoldingRegisters, 0, -1234567, order, swapped);
    QCOMPARE(server.readInt32(1, QModbusDataUnit::HoldingRegisters, 0, order, swapped), qint32(-1234567));

    server.writeUInt32(1, QModbusDataUnit::HoldingRegisters, 4, 0xFEDCBA98u, order, swapped);
    QCOMPARE(server.readUInt32(1, QModbusDataUnit::HoldingRegisters, 4, order, swapped), quint32(0xFEDCBA98u));

    server.writeInt64(1, QModbusDataUnit::HoldingRegisters, 8, -0x123456789LL, order, swapped);
    QCOMPARE(server.readInt64(1, QModbusDataUnit::HoldingRegisters, 8, order, swapped), qint64(-0x123456789LL));

    server.writeUInt64(1, QModbusDataUnit::HoldingRegisters, 12, Q_UINT64_C(0xFEDCBA9876543210), order, swapped);
    QCOMPARE(server.readUInt64(1, QModbusDataUnit::HoldingRegisters, 12, order, swapped), Q_UINT64_C(0xFEDCBA9876543210));

    server.writeFloat(1, QModbusDataUnit::HoldingRegisters, 20, 123.25f, order, swapped);
    QCOMPARE(server.readFloat(1, QModbusDataUnit::HoldingRegisters, 20, order, swapped), 123.25f);

    server.writeDouble(1, QModbusDataUnit::HoldingRegisters, 24, -9876.5, order, swapped);
    QCOMPARE(server.readDouble(1, QModbusDataUnit::HoldingRegisters, 24, order, swapped), -9876.5);
}

void TestModbusMultiServer::writesRegisterVariants()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 64);
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::Coils, 0, 8);
    ModbusWriteParams params{};
    params.DeviceId = 1;
    params.Address = 1;
    params.ZeroBasedAddress = false;
    params.Order = ByteOrder::Direct;
    params.RegOrder = RegisterOrder::MSRF;

    params.Value = true;
    server.writeRegister(QModbusDataUnit::Coils, params);
    QCOMPARE(server.data(1, QModbusDataUnit::Coils, 0, 1).value(0), quint16(1));

    params.Order = ByteOrder::Swapped;
    params.Value = QVariant::fromValue(QVector<quint16>({0x1234, 0xABCD}));
    server.writeRegister(QModbusDataUnit::HoldingRegisters, params);
    const auto written = server.data(1, QModbusDataUnit::HoldingRegisters, 0, 2);
    QCOMPARE(int(written.valueCount()), 2);
    QCOMPARE(written.value(0), quint16(0x3412));
    QCOMPARE(written.value(1), quint16(0xCDAB));

    params.Order = ByteOrder::Direct;
    const QList<QPair<DataType, QVariant>> cases = {
        {DataType::UInt16, QVariant::fromValue(42u)},
        {DataType::Float32, QVariant::fromValue(12.5f)},
        {DataType::Float64, QVariant::fromValue(24.5)},
        {DataType::Int32, QVariant::fromValue(-12345)},
        {DataType::Int64, QVariant::fromValue(qint64(-123456789))}
    };
    quint16 address = 10;
    for (const auto &testCase : cases) {
        params.Address = address;
        params.ZeroBasedAddress = true;
        params.DataMode = testCase.first;
        params.Value = testCase.second;
        params.RegOrder = RegisterOrder::LSRF;
        server.writeRegister(QModbusDataUnit::HoldingRegisters, params, WriteSource::Simulator);
        address += 5;
    }

    params.Value = 1;
    server.writeRegister(QModbusDataUnit::Invalid, params);
}

void TestModbusMultiServer::writesUnsignedRegisterValues()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 64);

    ModbusWriteParams params{};
    params.DeviceId = 1;
    params.ZeroBasedAddress = true;
    params.Order = ByteOrder::Direct;
    params.RegOrder = RegisterOrder::LSRF;

    params.Address = 0;
    params.DataMode = DataType::UInt32;
    params.Value = QVariant::fromValue(quint32(0xFEDCBA98u)); // above INT32_MAX
    server.writeRegister(QModbusDataUnit::HoldingRegisters, params);
    QCOMPARE(server.readUInt32(1, QModbusDataUnit::HoldingRegisters, 0, ByteOrder::Direct, true), quint32(0xFEDCBA98u));

    params.Address = 4;
    params.DataMode = DataType::UInt64;
    params.Value = QVariant::fromValue(Q_UINT64_C(0xFEDCBA9876543210)); // above INT64_MAX
    server.writeRegister(QModbusDataUnit::HoldingRegisters, params);
    QCOMPARE(server.readUInt64(1, QModbusDataUnit::HoldingRegisters, 4, ByteOrder::Direct, true), Q_UINT64_C(0xFEDCBA9876543210));

    // String-encoded values above the signed maximum: toInt()/toLongLong() fail on
    // these and return 0, so they regress if the write path goes through signed conversion.
    params.Address = 10;
    params.DataMode = DataType::UInt32;
    params.Value = QStringLiteral("4275878552");
    server.writeRegister(QModbusDataUnit::HoldingRegisters, params);
    QCOMPARE(server.readUInt32(1, QModbusDataUnit::HoldingRegisters, 10, ByteOrder::Direct, true), quint32(4275878552u));

    params.Address = 14;
    params.DataMode = DataType::UInt64;
    params.Value = QStringLiteral("18364758544493064720");
    server.writeRegister(QModbusDataUnit::HoldingRegisters, params);
    QCOMPARE(server.readUInt64(1, QModbusDataUnit::HoldingRegisters, 14, ByteOrder::Direct, true), Q_UINT64_C(18364758544493064720));
}

void TestModbusMultiServer::reportsDisconnectedState()
{
    ModbusMultiServer server;

    QVERIFY(!server.isConnected());
    QVERIFY(!server.isConnected(ConnectionType::Tcp, QStringLiteral("127.0.0.1:502")));
    QCOMPARE(server.state(ConnectionType::Serial, QStringLiteral("missing")),
             QModbusDevice::UnconnectedState);
    QVERIFY(server.connections().isEmpty());
    QCOMPARE(server.connectedClientCount(), 0);
    server.disconnectDevice(ConnectionType::Tcp, QStringLiteral("127.0.0.1:502"));
    server.closeConnections();
}

void TestModbusMultiServer::connectsTransportBackends()
{
    ModbusMultiServer server;
    server.addDeviceId(1);
    server.addUnitMap(QUuid::createUuid(), 1,
                      QModbusDataUnit::HoldingRegisters, 0, 16);
    server.setData(1, unit(QModbusDataUnit::HoldingRegisters, 0, {1, 2, 3}));

    bool handlerInvoked = false;
    server.setRequestHandler(RequestHandlerPtr::create(
        [&handlerInvoked](const QModbusPdu&, int, QModbusResponse&) {
            handlerInvoked = true;
            return false;
        }));

    QSignalSpy connectedSpy(&server, &ModbusMultiServer::connected);
    QSignalSpy disconnectedSpy(&server, &ModbusMultiServer::disconnected);
    QSignalSpy clientConnectedSpy(&server, &ModbusMultiServer::clientConnected);
    QSignalSpy clientDisconnectedSpy(&server, &ModbusMultiServer::clientDisconnected);
    QSignalSpy requestSpy(&server, &ModbusMultiServer::request);
    QSignalSpy responseSpy(&server, &ModbusMultiServer::response);
    QSignalSpy receivedSpy(&server, &ModbusMultiServer::rawDataReceived);
    QSignalSpy sentSpy(&server, &ModbusMultiServer::rawDataSended);
    QSignalSpy changedSpy(&server, &ModbusMultiServer::dataChanged);

    ConnectionDetails tcp;
    tcp.Type = ConnectionType::Tcp;
    tcp.TcpParams.IPAddress = QStringLiteral("127.0.0.1");
    tcp.TcpParams.ServicePort = availablePort();
    QVERIFY(tcp.TcpParams.ServicePort != 0);
    server.connectDevice(tcp);
    QTRY_VERIFY(server.isConnected(ConnectionType::Tcp,
                                   QStringLiteral("127.0.0.1:%1")
                                       .arg(tcp.TcpParams.ServicePort)));
    QVERIFY(server.isConnected());
    QCOMPARE(server.connections().size(), 1);
    QCOMPARE(connectedSpy.count(), 1);

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, tcp.TcpParams.ServicePort);
    QVERIFY(socket.waitForConnected(1000));
    QTRY_COMPARE(server.connectedClientCount(), 1);
    QTRY_COMPARE(clientConnectedSpy.count(), 1);

    const QByteArray request = tcpFrame(
        1, 1, QModbusRequest(QModbusRequest::WriteSingleRegister,
                             quint16(2), quint16(0x4567)));
    QCOMPARE(socket.write(request), qint64(request.size()));
    QVERIFY(socket.waitForBytesWritten(1000));
    QVERIFY(socket.waitForReadyRead(1000));
    QVERIFY(!socket.readAll().isEmpty());
    QTRY_VERIFY(handlerInvoked);
    QTRY_COMPARE(requestSpy.count(), 1);
    QTRY_COMPARE(responseSpy.count(), 1);
    QTRY_VERIFY(receivedSpy.count() >= 1);
    QTRY_COMPARE(sentSpy.count(), 1);
    QTRY_VERIFY(changedSpy.count() >= 1);
    QCOMPARE(server.data(1, QModbusDataUnit::HoldingRegisters, 2, 1).value(0),
             quint16(0x4567));

    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState)
        QVERIFY(socket.waitForDisconnected(1000));
    QTRY_COMPARE(server.connectedClientCount(), 0);
    QTRY_COMPARE(clientDisconnectedSpy.count(), 1);

    server.disconnectDevice(ConnectionType::Tcp,
                            QStringLiteral("127.0.0.1:%1")
                                .arg(tcp.TcpParams.ServicePort));
    QTRY_VERIFY(!server.isConnected());
    QTRY_COMPARE(disconnectedSpy.count(), 1);

    ConnectionDetails rtuTcp;
    rtuTcp.Type = ConnectionType::RtuTcp;
    rtuTcp.TcpParams.IPAddress = QStringLiteral("127.0.0.1");
    rtuTcp.TcpParams.ServicePort = availablePort();
    QVERIFY(rtuTcp.TcpParams.ServicePort != 0);
    server.connectDevice(rtuTcp);
    QTRY_VERIFY(server.isConnected(ConnectionType::RtuTcp,
                                   QStringLiteral("127.0.0.1:%1")
                                       .arg(rtuTcp.TcpParams.ServicePort)));
    server.closeConnections();
    QTRY_VERIFY(!server.isConnected());
    server.disconnectDevice(ConnectionType::RtuTcp,
                            QStringLiteral("127.0.0.1:%1")
                                .arg(rtuTcp.TcpParams.ServicePort));

    ConnectionDetails serial;
    serial.Type = ConnectionType::Serial;
    serial.SerialParams.PortName = QStringLiteral("nonexistent-openmodsim-port");
    server.connectDevice(serial);
    QVERIFY(!server.isConnected(ConnectionType::Serial, serial.SerialParams.PortName));
    server.disconnectDevice(ConnectionType::Serial, serial.SerialParams.PortName);
}

///
/// \brief Registers a device and a unit map so writes have somewhere to land.
///
void TestModbusMultiServer::prepare(ModbusMultiServer& server, quint8 deviceId, quint16 length)
{
    server.addDeviceId(deviceId);
    server.addUnitMap(QUuid::createUuid(), deviceId, QModbusDataUnit::HoldingRegisters, 0, length);
    server.addUnitMap(QUuid::createUuid(), deviceId, QModbusDataUnit::InputRegisters, 0, length);
}

///
/// \brief Regression for issue #126: writing a range must notify views once,
/// not once per register.
///
void TestModbusMultiServer::bulkWriteEmitsSingleDataChanged()
{
    ModbusMultiServer server;
    prepare(server, 1);

    QSignalSpy spy(&server, &ModbusMultiServer::dataChanged);

    QVector<quint16> values(100);
    for (int i = 0; i < values.size(); ++i)
        values[i] = static_cast<quint16>(i + 1);

    server.writeValues(1, QModbusDataUnit::HoldingRegisters, 0, values, ByteOrder::Direct);

    QTRY_COMPARE(spy.count(), 1);
}

///
/// \brief Regression for issue #126: the per-address timestamp storm is replaced
/// by one aggregated notification.
///
void TestModbusMultiServer::bulkWriteEmitsSingleTimestampsChanged()
{
    ModbusMultiServer server;
    prepare(server, 1);

    QSignalSpy aggregated(&server, &ModbusMultiServer::timestampsChanged);
    QSignalSpy perAddress(&server, &ModbusMultiServer::timestampChanged);

    QVector<quint16> values(100);
    for (int i = 0; i < values.size(); ++i)
        values[i] = static_cast<quint16>(i + 1);

    server.writeValues(1, QModbusDataUnit::HoldingRegisters, 0, values, ByteOrder::Direct);

    QTRY_COMPARE(aggregated.count(), 1);
    QCOMPARE(perAddress.count(), 0);
}

///
/// \brief A single-register write keeps the cheaper per-address signal.
///
void TestModbusMultiServer::singleWriteEmitsPerAddressTimestamp()
{
    ModbusMultiServer server;
    prepare(server, 1);

    QSignalSpy aggregated(&server, &ModbusMultiServer::timestampsChanged);
    QSignalSpy perAddress(&server, &ModbusMultiServer::timestampChanged);

    server.writeValue(1, QModbusDataUnit::HoldingRegisters, 5, 42, ByteOrder::Direct);

    QTRY_COMPARE(perAddress.count(), 1);
    QCOMPARE(aggregated.count(), 0);
}

void TestModbusMultiServer::unchangedValueEmitsNothing()
{
    ModbusMultiServer server;
    prepare(server, 1);

    server.writeValue(1, QModbusDataUnit::HoldingRegisters, 5, 42, ByteOrder::Direct);
    QTest::qWait(20);

    QSignalSpy spy(&server, &ModbusMultiServer::dataChanged);
    server.writeValue(1, QModbusDataUnit::HoldingRegisters, 5, 42, ByteOrder::Direct);

    QTest::qWait(50);
    QCOMPARE(spy.count(), 0);
}

void TestModbusMultiServer::unknownDeviceIdReportsError()
{
    ModbusMultiServer server;
    prepare(server, 1);

    QSignalSpy spy(&server, &ModbusMultiServer::errorOccured);
    server.writeValue(99, QModbusDataUnit::HoldingRegisters, 0, 1, ByteOrder::Direct);

    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toUInt(), 99u);
}

///
/// \brief createDataUnit used to drop the byte-order conversion for ranges.
///
void TestModbusMultiServer::bulkWriteAppliesByteOrder()
{
    ModbusMultiServer server;
    prepare(server, 1);

    server.writeValues(1, QModbusDataUnit::HoldingRegisters, 0, { 0x1234 }, ByteOrder::Swapped);

    const auto swapped = server.data(1, QModbusDataUnit::HoldingRegisters, 0, 1);
    QCOMPARE(swapped.value(0), 0x3412);

    server.writeValues(1, QModbusDataUnit::InputRegisters, 0, { 0x1234 }, ByteOrder::Direct);

    const auto direct = server.data(1, QModbusDataUnit::InputRegisters, 0, 1);
    QCOMPARE(direct.value(0), 0x1234);
}

void TestModbusMultiServer::bulkWriteStoresEveryValue()
{
    ModbusMultiServer server;
    prepare(server, 1);

    QVector<quint16> values(50);
    for (int i = 0; i < values.size(); ++i)
        values[i] = static_cast<quint16>(1000 + i);

    server.writeValues(1, QModbusDataUnit::HoldingRegisters, 10, values, ByteOrder::Direct);

    const auto stored = server.data(1, QModbusDataUnit::HoldingRegisters, 10, values.size());
    QCOMPARE(static_cast<int>(stored.valueCount()), values.size());
    for (int i = 0; i < values.size(); ++i)
        QCOMPARE(stored.value(i), values[i]);
}

///
/// \brief Guards against regressing back to a per-register round trip storm.
///
void TestModbusMultiServer::benchmarkSingleWrites()
{
    ModbusMultiServer server;
    prepare(server, 1);

    quint16 value = 0;
    QBENCHMARK {
        for (int i = 0; i < 300; ++i)
            server.writeValue(1, QModbusDataUnit::HoldingRegisters, static_cast<quint16>(i), ++value, ByteOrder::Direct);
    }
}

void TestModbusMultiServer::benchmarkBulkWrite()
{
    ModbusMultiServer server;
    prepare(server, 1);

    quint16 seed = 0;
    QBENCHMARK {
        QVector<quint16> values(300);
        for (int i = 0; i < values.size(); ++i)
            values[i] = static_cast<quint16>(i + seed);
        ++seed;

        server.writeValues(1, QModbusDataUnit::HoldingRegisters, 0, values, ByteOrder::Direct);
    }
}

QTEST_GUILESS_MAIN(TestModbusMultiServer)
#include "test_modbusmultiserver.moc"
