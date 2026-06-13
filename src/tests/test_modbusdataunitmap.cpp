// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbusdataunitmap.cpp
/// \brief Unit tests for ModbusDataUnitMap address mapping and data storage.
///

#include <QTest>
#include <QUuid>

#include "modbusdataunitmap.h"

class TestModbusDataUnitMap : public QObject
{
    Q_OBJECT

private slots:
    void addUnitMapBuildsLocalRange();
    void addUnitMapReportsChange();
    void removeUnitMap();
    void containsRangeBounds();
    void setAndGetDataRoundTrip();
    void timestampStorage();
    void descriptionStorage();
    void ensureRangeExpandsLocalMap();
    void addressSpaceResizesGlobalMap();
};

void TestModbusDataUnitMap::addUnitMapBuildsLocalRange()
{
    ModbusDataUnitMap map;
    const QUuid id = QUuid::createUuid();
    QVERIFY(map.addUnitMap(id, QModbusDataUnit::Coils, 10, 5));

    QVERIFY(map.contains(QModbusDataUnit::Coils));
    const QModbusDataUnit unit = map.value(QModbusDataUnit::Coils);
    QCOMPARE(unit.startAddress(), 10);
    QCOMPARE(int(unit.valueCount()), 5);

    QModbusDataUnit stored;
    QVERIFY(map.unitMap(id, stored));
    QCOMPARE(stored.startAddress(), 10);
}

void TestModbusDataUnitMap::addUnitMapReportsChange()
{
    ModbusDataUnitMap map;
    const QUuid id = QUuid::createUuid();
    QVERIFY(map.addUnitMap(id, QModbusDataUnit::HoldingRegisters, 0, 4));
    QVERIFY(!map.addUnitMap(id, QModbusDataUnit::HoldingRegisters, 0, 4));
}

void TestModbusDataUnitMap::removeUnitMap()
{
    ModbusDataUnitMap map;
    const QUuid id = QUuid::createUuid();
    map.addUnitMap(id, QModbusDataUnit::Coils, 0, 8);

    QVERIFY(map.removeUnitMap(id));
    QVERIFY(!map.removeUnitMap(QUuid::createUuid()));
    QVERIFY(!map.contains(QModbusDataUnit::Coils));
}

void TestModbusDataUnitMap::containsRangeBounds()
{
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::InputRegisters, 100, 10);

    QVERIFY(map.containsRange(QModbusDataUnit::InputRegisters, 100, 10));
    QVERIFY(map.containsRange(QModbusDataUnit::InputRegisters, 105, 5));
    QVERIFY(!map.containsRange(QModbusDataUnit::InputRegisters, 100, 11));
    QVERIFY(!map.containsRange(QModbusDataUnit::InputRegisters, 99, 2));
    QVERIFY(!map.containsRange(QModbusDataUnit::InputRegisters, 100, 0));
    QVERIFY(!map.containsRange(QModbusDataUnit::Coils, 100, 1));
}

void TestModbusDataUnitMap::setAndGetDataRoundTrip()
{
    ModbusDataUnitMap map;
    QModbusDataUnit source(QModbusDataUnit::HoldingRegisters, 0, {10, 20, 30});
    map.setData(source);

    const QModbusDataUnit read = map.getData(QModbusDataUnit::HoldingRegisters, 0, 3);
    QCOMPARE(read.value(0), quint16(10));
    QCOMPARE(read.value(1), quint16(20));
    QCOMPARE(read.value(2), quint16(30));
}

void TestModbusDataUnitMap::timestampStorage()
{
    ModbusDataUnitMap map;
    QCOMPARE(map.timestamp(QModbusDataUnit::Coils, 5), QDateTime());

    const QDateTime now = QDateTime::currentDateTime();
    map.setTimestamp(QModbusDataUnit::Coils, 5, now);
    QCOMPARE(map.timestamp(QModbusDataUnit::Coils, 5), now);

    map.setTimestamp(QModbusDataUnit::Coils, 5, QDateTime());
    QCOMPARE(map.timestamp(QModbusDataUnit::Coils, 5), QDateTime());
}

void TestModbusDataUnitMap::descriptionStorage()
{
    ModbusDataUnitMap map;
    QVERIFY(map.description(QModbusDataUnit::HoldingRegisters, 1).isEmpty());

    map.setDescription(QModbusDataUnit::HoldingRegisters, 1, QStringLiteral("temperature"));
    QCOMPARE(map.description(QModbusDataUnit::HoldingRegisters, 1), QStringLiteral("temperature"));
    QCOMPARE(map.descriptionMap().size(), 1);

    map.setDescription(QModbusDataUnit::HoldingRegisters, 1, QString());
    QVERIFY(map.description(QModbusDataUnit::HoldingRegisters, 1).isEmpty());

    map.setDescription(QModbusDataUnit::HoldingRegisters, 2, QStringLiteral("setpoint"));
    map.clearDescriptions();
    QVERIFY(map.descriptionMap().isEmpty());
}

void TestModbusDataUnitMap::ensureRangeExpandsLocalMap()
{
    ModbusDataUnitMap map;
    QVERIFY(map.ensureRange(QModbusDataUnit::Coils, 0, 4));
    QVERIFY(map.containsRange(QModbusDataUnit::Coils, 0, 4));
    QVERIFY(!map.ensureRange(QModbusDataUnit::Coils, 0, 4));
    QVERIFY(!map.ensureRange(QModbusDataUnit::Coils, 0, 0));
}

void TestModbusDataUnitMap::addressSpaceResizesGlobalMap()
{
    ModbusDataUnitMap map;
    map.setGlobalMap(true);
    QCOMPARE(int(map.value(QModbusDataUnit::Coils).valueCount()), 65535);

    map.setAddressSpace(AddressSpace::Addr5Digits);
    QCOMPARE(map.addressSpace(), AddressSpace::Addr5Digits);
    QCOMPARE(int(map.value(QModbusDataUnit::Coils).valueCount()), 9999);
}

QTEST_GUILESS_MAIN(TestModbusDataUnitMap)
#include "test_modbusdataunitmap.moc"
