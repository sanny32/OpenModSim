// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbusmultiserver.cpp
/// \brief Unit tests for ModbusMultiServer metadata updates.
///

#include <QDateTime>
#include <QTest>

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

}

class TestModbusMultiServer : public QObject
{
    Q_OBJECT

private slots:
    void mergesDescriptionMaps();
    void replacesDescriptionMaps();
    void mergesTimestampMaps();
    void replacesTimestampMaps();
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

QTEST_GUILESS_MAIN(TestModbusMultiServer)
#include "test_modbusmultiserver.moc"
