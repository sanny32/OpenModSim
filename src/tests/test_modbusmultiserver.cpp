// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbusmultiserver.cpp
/// \brief Unit tests for bulk writes and change-notification batching.
///

#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "modbusmultiserver.h"

class TestModbusMultiServer : public QObject
{
    Q_OBJECT

private slots:
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

QTEST_MAIN(TestModbusMultiServer)
#include "test_modbusmultiserver.moc"
