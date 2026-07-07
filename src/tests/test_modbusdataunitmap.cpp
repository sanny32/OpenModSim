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
    void addUnitMapDetectsRangeShapeChanges();
    void removeUnitMap();
    void containsRangeBounds();
    void globalContainsRangeAndAddressSpaceNoop();
    void setAndGetDataRoundTrip();
    void setDataUpdatesGlobalEvenOutsideLocalMap();
    void timestampStorage();
    void descriptionStorage();
    void ensureRangeExpandsLocalMap();
    void addressSpaceResizesGlobalMap();
    void addressSpaceCanResizeBackToSixDigits();
    void unknownAddressSpaceUsesFiveDigitGlobalSize();
    void missingUnitAndOutOfRangeData();
    void filteredTimestampMap();
    void filteredDescriptionMap();
    void updateDataUnitMapMergesRangesAndKeepsValues();
    void zeroLengthUnitMapDoesNotCreateLocalRange();
    void globalMapIterationAndAccess();
    void globalContainsRangeRejectsUnknownRegisterType();
    void localMapIterationAndAccess();
    void emptyLocalMapAccessors();
    void timestampFallbackForLocalUnitMap();
    void ensureRangeMergesWithExistingRange();
    void setDataTimestampUpdateModes();
    void addAndRemoveDifferentRegisterTypesReportMapShapeChanges();
    void setDataUpdatesLocalRangeWhenAddressIsMapped();
    void filteredMetadataUsesHalfOpenEndAddress();
    void convertsCurrentViewToQtMap();
    void replacingUnitMapCanMoveRegisterType();
    void setDataPartiallyOverlappingLocalRange();
    void timestampOverrideFallsBackToLocalRangeTimestamp();
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

void TestModbusDataUnitMap::addUnitMapDetectsRangeShapeChanges()
{
    ModbusDataUnitMap map;
    const QUuid id = QUuid::createUuid();

    QVERIFY(map.addUnitMap(id, QModbusDataUnit::HoldingRegisters, 10, 2));
    QVERIFY(map.addUnitMap(id, QModbusDataUnit::HoldingRegisters, 10, 3));
    QVERIFY(map.addUnitMap(id, QModbusDataUnit::HoldingRegisters, 9, 3));

    const auto unit = map.value(QModbusDataUnit::HoldingRegisters);
    QCOMPARE(unit.startAddress(), 9);
    QCOMPARE(int(unit.valueCount()), 3);
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

void TestModbusDataUnitMap::globalContainsRangeAndAddressSpaceNoop()
{
    ModbusDataUnitMap map;
    map.setGlobalMap(true);

    QVERIFY(map.contains(QModbusDataUnit::Coils));
    QVERIFY(map.containsRange(QModbusDataUnit::Coils, 0, 1));
    QVERIFY(map.containsRange(QModbusDataUnit::Coils, 65534, 1));
    QVERIFY(!map.containsRange(QModbusDataUnit::Coils, 65535, 1));

    map.setAddressSpace(AddressSpace::Addr6Digits);
    QCOMPARE(int(map.value(QModbusDataUnit::Coils).valueCount()), 65535);
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

void TestModbusDataUnitMap::setDataUpdatesGlobalEvenOutsideLocalMap()
{
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 10, 1);

    map.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 20, QVector<quint16>({77})));

    QCOMPARE(map.getData(QModbusDataUnit::HoldingRegisters, 20, 1).value(0), quint16(77));
    QCOMPARE(map.value(QModbusDataUnit::HoldingRegisters).value(0), quint16(0));
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

void TestModbusDataUnitMap::addressSpaceCanResizeBackToSixDigits()
{
    ModbusDataUnitMap map;
    map.setGlobalMap(true);

    map.setAddressSpace(AddressSpace::Addr5Digits);
    map.setAddressSpace(AddressSpace::Addr6Digits);

    QCOMPARE(map.addressSpace(), AddressSpace::Addr6Digits);
    QCOMPARE(int(map.value(QModbusDataUnit::Coils).valueCount()), 65535);
    QCOMPARE(int(map.value(QModbusDataUnit::DiscreteInputs).valueCount()), 65535);
    QCOMPARE(int(map.value(QModbusDataUnit::InputRegisters).valueCount()), 65535);
    QCOMPARE(int(map.value(QModbusDataUnit::HoldingRegisters).valueCount()), 65535);
}

void TestModbusDataUnitMap::unknownAddressSpaceUsesFiveDigitGlobalSize()
{
    ModbusDataUnitMap map;
    map.setGlobalMap(true);

    map.setAddressSpace(static_cast<AddressSpace>(99));

    QCOMPARE(map.addressSpace(), static_cast<AddressSpace>(99));
    QCOMPARE(int(map.value(QModbusDataUnit::Coils).valueCount()), 9999);
    QCOMPARE(int(map.value(QModbusDataUnit::DiscreteInputs).valueCount()), 9999);
    QCOMPARE(int(map.value(QModbusDataUnit::InputRegisters).valueCount()), 9999);
    QCOMPARE(int(map.value(QModbusDataUnit::HoldingRegisters).valueCount()), 9999);
}

void TestModbusDataUnitMap::missingUnitAndOutOfRangeData()
{
    ModbusDataUnitMap map;
    QModbusDataUnit unit;

    QVERIFY(!map.unitMap(QUuid::createUuid(), unit));

    QModbusDataUnit source(QModbusDataUnit::HoldingRegisters, 10, {11, 22});
    map.setData(source);

    QCOMPARE(map.getData(QModbusDataUnit::InputRegisters, 10, 1).value(0), quint16(0));
    QCOMPARE(map.getData(QModbusDataUnit::HoldingRegisters, 9, 1).value(0), quint16(0));
    QCOMPARE(map.getData(QModbusDataUnit::HoldingRegisters, 12, 1).value(0), quint16(0));
}

void TestModbusDataUnitMap::filteredTimestampMap()
{
    ModbusDataUnitMap map;
    const auto first = QDateTime::currentDateTime();
    const auto second = first.addSecs(1);
    const auto otherType = first.addSecs(2);

    map.setTimestamp(QModbusDataUnit::HoldingRegisters, 10, first);
    map.setTimestamp(QModbusDataUnit::HoldingRegisters, 12, second);
    map.setTimestamp(QModbusDataUnit::InputRegisters, 11, otherType);

    QCOMPARE(map.timestampMap().size(), 3);
    QCOMPARE(map.timestampMap(QModbusDataUnit::HoldingRegisters, 10, 0).size(), 0);

    const auto filtered = map.timestampMap(QModbusDataUnit::HoldingRegisters, 10, 2);
    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered.constBegin().key().Address, quint16(10));
    QCOMPARE(filtered.constBegin().value(), first);

    map.clearTimestamps();
    QVERIFY(map.timestampMap().isEmpty());
}

void TestModbusDataUnitMap::filteredDescriptionMap()
{
    ModbusDataUnitMap map;
    map.setDescription(QModbusDataUnit::HoldingRegisters, 10, QStringLiteral("inside"));
    map.setDescription(QModbusDataUnit::HoldingRegisters, 12, QStringLiteral("outside"));
    map.setDescription(QModbusDataUnit::InputRegisters, 11, QStringLiteral("other"));

    QCOMPARE(map.descriptionMap(QModbusDataUnit::HoldingRegisters, 10, 0).size(), 0);

    const auto filtered = map.descriptionMap(QModbusDataUnit::HoldingRegisters, 10, 2);
    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered.constBegin().key().Address, quint16(10));
    QCOMPARE(filtered.constBegin().value(), QStringLiteral("inside"));
}

void TestModbusDataUnitMap::updateDataUnitMapMergesRangesAndKeepsValues()
{
    ModbusDataUnitMap map;
    const QUuid first = QUuid::createUuid();
    const QUuid second = QUuid::createUuid();

    map.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 10, {100, 101, 102, 103}));
    QVERIFY(map.addUnitMap(first, QModbusDataUnit::HoldingRegisters, 12, 2));
    QVERIFY(map.addUnitMap(second, QModbusDataUnit::HoldingRegisters, 10, 1));

    const auto unit = map.value(QModbusDataUnit::HoldingRegisters);
    QCOMPARE(unit.startAddress(), 10);
    QCOMPARE(int(unit.valueCount()), 4);
    QCOMPARE(unit.value(0), quint16(100));
    QCOMPARE(unit.value(2), quint16(102));

    QVERIFY(map.removeUnitMap(first));
    const auto shrunk = map.value(QModbusDataUnit::HoldingRegisters);
    QCOMPARE(shrunk.startAddress(), 10);
    QCOMPARE(int(shrunk.valueCount()), 1);
    QCOMPARE(shrunk.value(0), quint16(100));
}

void TestModbusDataUnitMap::zeroLengthUnitMapDoesNotCreateLocalRange()
{
    ModbusDataUnitMap map;
    const QUuid id = QUuid::createUuid();

    QVERIFY(!map.addUnitMap(id, QModbusDataUnit::Coils, 5, 0));
    QVERIFY(!map.contains(QModbusDataUnit::Coils));

    QModbusDataUnit stored;
    QVERIFY(map.unitMap(id, stored));
    QCOMPARE(stored.startAddress(), 5);
    QCOMPARE(int(stored.valueCount()), 0);
}

void TestModbusDataUnitMap::globalMapIterationAndAccess()
{
    ModbusDataUnitMap map;
    map.setGlobalMap(true);

    QCOMPARE(map.isGlobalMap(), true);
    QVERIFY(map.end() != map.begin());
    int unitCount = 0;
    for(auto it = map.begin(); it != map.end(); ++it)
        ++unitCount;
    QCOMPARE(unitCount, 4);

    map[QModbusDataUnit::Coils].setValue(0, 1);
    QCOMPARE(map.getData(QModbusDataUnit::Coils, 0, 1).value(0), quint16(1));
}

void TestModbusDataUnitMap::globalContainsRangeRejectsUnknownRegisterType()
{
    ModbusDataUnitMap map;
    map.setGlobalMap(true);

    const auto unknownType = static_cast<QModbusDataUnit::RegisterType>(999);

    QVERIFY(map.contains(unknownType));
    QVERIFY(!map.containsRange(unknownType, 0, 1));
    QVERIFY(!map.containsRange(unknownType, 0, 0));
    QCOMPARE(map.getData(unknownType, 0, 1).value(0), quint16(0));
}

void TestModbusDataUnitMap::localMapIterationAndAccess()
{
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::Coils, 1, 2);
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 10, 1);

    int unitCount = 0;
    for(auto it = map.begin(); it != map.end(); ++it)
        ++unitCount;

    QCOMPARE(unitCount, 2);
    QVERIFY(map.end() != map.begin());

    map[QModbusDataUnit::Coils].setValue(0, 1);
    QCOMPARE(map.value(QModbusDataUnit::Coils).value(0), quint16(1));
}

void TestModbusDataUnitMap::emptyLocalMapAccessors()
{
    ModbusDataUnitMap map;

    QCOMPARE(map.isGlobalMap(), false);
    QVERIFY(map.begin() == map.end());
    QVERIFY(!map.contains(QModbusDataUnit::Coils));
    QVERIFY(!map.value(QModbusDataUnit::Coils).isValid());
}

void TestModbusDataUnitMap::timestampFallbackForLocalUnitMap()
{
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::Coils, 10, 2);

    QVERIFY(map.timestamp(QModbusDataUnit::Coils, 10).isValid());
    QVERIFY(map.timestamp(QModbusDataUnit::Coils, 11).isValid());
    QCOMPARE(map.timestamp(QModbusDataUnit::Coils, 9), QDateTime());
    QCOMPARE(map.timestamp(QModbusDataUnit::Coils, 12), QDateTime());
    QCOMPARE(map.timestamp(QModbusDataUnit::HoldingRegisters, 10), QDateTime());
}

void TestModbusDataUnitMap::ensureRangeMergesWithExistingRange()
{
    ModbusDataUnitMap map;
    map.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 3, {30, 40, 50, 60}));

    QVERIFY(map.ensureRange(QModbusDataUnit::HoldingRegisters, 5, 2));
    QVERIFY(map.ensureRange(QModbusDataUnit::HoldingRegisters, 3, 2));

    const auto unit = map.value(QModbusDataUnit::HoldingRegisters);
    QCOMPARE(unit.startAddress(), 3);
    QCOMPARE(int(unit.valueCount()), 4);
    QCOMPARE(unit.value(0), quint16(30));
    QCOMPARE(unit.value(2), quint16(50));

    map.setGlobalMap(true);
    QVERIFY(!map.ensureRange(QModbusDataUnit::HoldingRegisters, 0, 1));
}

void TestModbusDataUnitMap::setDataTimestampUpdateModes()
{
    ModbusDataUnitMap map;
    QModbusDataUnit first(QModbusDataUnit::HoldingRegisters, 0, 1);
    first.setValue(0, 1);

    map.setData(first);
    const auto original = map.timestamp(QModbusDataUnit::HoldingRegisters, 0);
    QVERIFY(original.isValid());

    map.setTimestamp(QModbusDataUnit::HoldingRegisters, 0, QDateTime());
    map.setData(first, false);
    QCOMPARE(map.timestamp(QModbusDataUnit::HoldingRegisters, 0), QDateTime());

    map.setData(first, true);
    QVERIFY(map.timestamp(QModbusDataUnit::HoldingRegisters, 0).isValid());
}

void TestModbusDataUnitMap::addAndRemoveDifferentRegisterTypesReportMapShapeChanges()
{
    ModbusDataUnitMap map;
    const QUuid coils = QUuid::createUuid();
    const QUuid registers = QUuid::createUuid();

    QVERIFY(map.addUnitMap(coils, QModbusDataUnit::Coils, 0, 2));
    QVERIFY(map.addUnitMap(registers, QModbusDataUnit::HoldingRegisters, 10, 1));
    QVERIFY(map.contains(QModbusDataUnit::Coils));
    QVERIFY(map.contains(QModbusDataUnit::HoldingRegisters));

    QVERIFY(map.removeUnitMap(coils));
    QVERIFY(!map.contains(QModbusDataUnit::Coils));
    QVERIFY(map.contains(QModbusDataUnit::HoldingRegisters));
}

void TestModbusDataUnitMap::setDataUpdatesLocalRangeWhenAddressIsMapped()
{
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 10, 3);

    map.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 11, QVector<quint16>({77})));

    const auto local = map.value(QModbusDataUnit::HoldingRegisters);
    QCOMPARE(local.startAddress(), 10);
    QCOMPARE(int(local.valueCount()), 3);
    QCOMPARE(local.value(0), quint16(0));
    QCOMPARE(local.value(1), quint16(77));
    QCOMPARE(local.value(2), quint16(0));
}

void TestModbusDataUnitMap::filteredMetadataUsesHalfOpenEndAddress()
{
    ModbusDataUnitMap map;
    const QDateTime timestamp = QDateTime::currentDateTime();
    map.setTimestamp(QModbusDataUnit::Coils, 9, timestamp.addSecs(-1));
    map.setTimestamp(QModbusDataUnit::Coils, 10, timestamp);
    map.setTimestamp(QModbusDataUnit::Coils, 11, timestamp.addSecs(1));
    map.setTimestamp(QModbusDataUnit::Coils, 12, timestamp.addSecs(2));

    map.setDescription(QModbusDataUnit::Coils, 9, QStringLiteral("before"));
    map.setDescription(QModbusDataUnit::Coils, 10, QStringLiteral("first"));
    map.setDescription(QModbusDataUnit::Coils, 11, QStringLiteral("last"));
    map.setDescription(QModbusDataUnit::Coils, 12, QStringLiteral("after"));

    const auto timestamps = map.timestampMap(QModbusDataUnit::Coils, 10, 2);
    QCOMPARE(timestamps.size(), 2);
    QVERIFY(timestamps.contains({0, QModbusDataUnit::Coils, 10}));
    QVERIFY(timestamps.contains({0, QModbusDataUnit::Coils, 11}));
    QVERIFY(!timestamps.contains({0, QModbusDataUnit::Coils, 12}));

    const auto descriptions = map.descriptionMap(QModbusDataUnit::Coils, 10, 2);
    QCOMPARE(descriptions.size(), 2);
    QCOMPARE(descriptions.value({0, QModbusDataUnit::Coils, 10}), QStringLiteral("first"));
    QCOMPARE(descriptions.value({0, QModbusDataUnit::Coils, 11}), QStringLiteral("last"));
    QVERIFY(!descriptions.contains({0, QModbusDataUnit::Coils, 12}));
}

void TestModbusDataUnitMap::convertsCurrentViewToQtMap()
{
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::Coils, 1, 2);

    QModbusDataUnitMap local = map;
    QCOMPARE(local.size(), 1);
    QVERIFY(local.contains(QModbusDataUnit::Coils));

    map.setGlobalMap(true);
    QModbusDataUnitMap global = map;
    QCOMPARE(global.size(), 4);
    QVERIFY(global.contains(QModbusDataUnit::DiscreteInputs));
    QVERIFY(global.contains(QModbusDataUnit::InputRegisters));
}

void TestModbusDataUnitMap::replacingUnitMapCanMoveRegisterType()
{
    ModbusDataUnitMap map;
    const QUuid id = QUuid::createUuid();

    QVERIFY(map.addUnitMap(id, QModbusDataUnit::Coils, 1, 2));
    QVERIFY(map.contains(QModbusDataUnit::Coils));
    QVERIFY(map.addUnitMap(id, QModbusDataUnit::InputRegisters, 5, 3));

    QVERIFY(!map.contains(QModbusDataUnit::Coils));
    QVERIFY(map.contains(QModbusDataUnit::InputRegisters));
    QCOMPARE(map.value(QModbusDataUnit::InputRegisters).startAddress(), 5);
    QCOMPARE(int(map.value(QModbusDataUnit::InputRegisters).valueCount()), 3);
}

void TestModbusDataUnitMap::setDataPartiallyOverlappingLocalRange()
{
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 10, 3);

    map.setData(QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 9, QVector<quint16>({9, 10, 11, 12, 13})));

    const auto local = map.value(QModbusDataUnit::HoldingRegisters);
    QCOMPARE(local.startAddress(), 10);
    QCOMPARE(int(local.valueCount()), 3);
    QCOMPARE(local.value(0), quint16(10));
    QCOMPARE(local.value(1), quint16(11));
    QCOMPARE(local.value(2), quint16(12));
    QCOMPARE(map.getData(QModbusDataUnit::HoldingRegisters, 9, 5).value(4), quint16(13));
}

void TestModbusDataUnitMap::timestampOverrideFallsBackToLocalRangeTimestamp()
{
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::Coils, 10, 1);

    const QDateTime fallback = map.timestamp(QModbusDataUnit::Coils, 10);
    QVERIFY(fallback.isValid());

    const QDateTime explicitTimestamp = fallback.addSecs(10);
    map.setTimestamp(QModbusDataUnit::Coils, 10, explicitTimestamp);
    QCOMPARE(map.timestamp(QModbusDataUnit::Coils, 10), explicitTimestamp);

    map.setTimestamp(QModbusDataUnit::Coils, 10, QDateTime());
    QCOMPARE(map.timestamp(QModbusDataUnit::Coils, 10), fallback);
}

QTEST_GUILESS_MAIN(TestModbusDataUnitMap)
#include "test_modbusdataunitmap.moc"
