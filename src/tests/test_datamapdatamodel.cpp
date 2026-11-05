// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_datamapdatamodel.cpp
/// \brief Unit tests for DataMapDataModel registration of Map View rows in the Modbus server.
///

#include <QSignalSpy>
#include <QTest>

#include "datamapdatamodel.h"

namespace {

///
/// \brief Creates a holding register key of device 1.
/// \param address Register address.
/// \return The initialized key.
///
ItemMapKey holdingKey(quint16 address)
{
    return {1, QModbusDataUnit::HoldingRegisters, address};
}

///
/// \brief Writes a single holding register value of device 1 as a user edit.
///
void writeHolding(ModbusMultiServer& server, quint16 address, quint16 value)
{
    server.setData(1, QModbusDataUnit(QModbusDataUnit::HoldingRegisters, address, QVector<quint16>{value}),
                   WriteSource::User);
}

///
/// \brief Reads a single holding register value of device 1.
///
quint16 readHolding(const ModbusMultiServer& server, quint16 address)
{
    return static_cast<quint16>(server.data(1, QModbusDataUnit::HoldingRegisters, address, 1).value(0));
}

}

class TestDataMapDataModel : public QObject
{
    Q_OBJECT

private slots:
    void editsEnronRegisters_data();
    void editsEnronRegisters();
    void filtersEnronGroups();
    void removingRowReleasesDeviceId();
    void removingRowAfterDataTypeChangeReleasesDeviceId();
    void removingDuplicateKeyRowsReleasesDeviceId();
    void removingRowResetsStoredData();
    void removingRowKeepsDataCoveredByAnotherMap();
    void removingMultiRegisterRowResetsEveryRegister();
    void removingRowKeepsDataCoveredByDataViewRange();
};

/// \brief Covers both Enron groups independently of the ordinary address base.
void TestDataMapDataModel::editsEnronRegisters_data()
{
    QTest::addColumn<int>("group");
    QTest::addColumn<bool>("zeroBased");
    QTest::newRow("5x-base0") << 1 << true;
    QTest::newRow("5x-base1") << 1 << false;
    QTest::newRow("7x-base0") << 2 << true;
    QTest::newRow("7x-base1") << 2 << false;
}

/// \brief Keeps protocol addresses, native widths and values consistent across edits and protocol toggles.
void TestDataMapDataModel::editsEnronRegisters()
{
    QFETCH(int, group);
    QFETCH(bool, zeroBased);
    ModbusMultiServer server;
    auto definitions = server.getModbusDefinitions();
    definitions.EnronModbus = true;
    server.setModbusDefinitions(definitions);
    DataMapDataModel model(server);
    model.setZeroBased(zeroBased);
    model.addEntry(holdingKey(0), {});
    const auto enron = static_cast<EnronModbus::RegisterType>(group);
    const int base = EnronModbus::baseAddress(enron);
    QSignalSpy mapAdded(&server, &ModbusMultiServer::unitMapAdded);
    const auto typeIndex = model.index(0, ColType);
    QVERIFY(model.setData(typeIndex, group, DataMapRole::EnronType));
    QCOMPARE(model.data(typeIndex, DataMapRole::EnronType).toInt(), group);
    QTRY_COMPARE(mapAdded.last().last().value<EnronModbus::RegisterType>(), enron);
    QCOMPARE(model.data(typeIndex).toString(), group == 1 ? QString("Enron Registers Int32") : QString("Enron Registers Float32"));
    QCOMPARE(model.keyForRow(0).Type, QModbusDataUnit::HoldingRegisters);
    QCOMPARE(model.keyForRow(0).Address, quint16(base));
    QCOMPARE(model.data(model.index(0, ColAddress)).toInt(), base);
    QVERIFY(server.data(1, QModbusDataUnit::HoldingRegisters, base, 2).isValid());
    QVERIFY(model.setData(model.index(0, ColAddress), QString::number(base + 1)));
    QCOMPARE(model.keyForRow(0).Address, quint16(base + 2));
    QCOMPARE(model.data(model.index(0, ColAddress)).toInt(), base + 1);
    QVERIFY(model.setData(model.index(0, ColDataType), "UInt32"));
    QVERIFY(model.setData(model.index(0, ColValue), uint(0x12345678)));
    QCOMPARE(server.data(1, QModbusDataUnit::HoldingRegisters, base + 2, 2).values(),
        QVector<quint16>({0x1234, 0x5678}));
    QVERIFY(model.setData(model.index(0, ColDataType), "UInt16"));
    QVERIFY(server.data(1, QModbusDataUnit::HoldingRegisters, base + 2, 2).isValid());
    QVERIFY(!model.setData(model.index(0, ColAddress), QString::number(base - 1)));
    QVERIFY(!model.setData(model.index(0, ColAddress), QString::number(base + 999)));
    QVERIFY(!model.setData(model.index(0, ColAddress), "invalid"));
    QVERIFY(!model.setData(typeIndex, 99, DataMapRole::EnronType));
    QCOMPARE(model.keyForRow(0).Address, quint16(base + 2));
    QCOMPARE(server.data(1, QModbusDataUnit::HoldingRegisters, base + 2, 2).values(),
        QVector<quint16>({0x1234, 0x5678}));
    definitions.EnronModbus = false;
    server.setModbusDefinitions(definitions);
    QTRY_VERIFY(!model.data(typeIndex, DataMapRole::EnronEnabled).toBool());
    QCOMPARE(model.data(typeIndex, DataMapRole::EnronType).toInt(), 0);
    QTRY_COMPARE(mapAdded.last().last().value<EnronModbus::RegisterType>(), EnronModbus::RegisterType::None);
    QVERIFY(!model.setData(typeIndex, group, DataMapRole::EnronType));
    definitions.EnronModbus = true;
    server.setModbusDefinitions(definitions);
    QTRY_COMPARE(model.data(typeIndex, DataMapRole::EnronType).toInt(), group);
    QTRY_COMPARE(mapAdded.last().last().value<EnronModbus::RegisterType>(), enron);
    QCOMPARE(model.data(model.index(0, ColAddress)).toInt(), base + 1);
    QCOMPARE(model.data(model.index(0, ColDataType)).toString(), QStringLiteral("UInt16"));
    QCOMPARE(server.data(1, QModbusDataUnit::HoldingRegisters, base + 2, 2).values(),
        QVector<quint16>({0x1234, 0x5678}));
    QVERIFY(model.setData(typeIndex, int(QModbusDataUnit::HoldingRegisters), DataMapRole::TypeValue));
    QCOMPARE(model.data(typeIndex, DataMapRole::EnronType).toInt(), 0);
    QCOMPARE(model.data(model.index(0, ColAddress)).toInt(), base + 2 + (zeroBased ? 0 : 1));
}

/// \brief Filters Enron groups separately while retaining ordinary table filters.
void TestDataMapDataModel::filtersEnronGroups()
{
    ModbusMultiServer server;
    auto definitions = server.getModbusDefinitions();
    definitions.EnronModbus = true;
    server.setModbusDefinitions(definitions);
    DataMapDataModel model(server);
    for (auto group : {EnronModbus::RegisterType::Group5x, EnronModbus::RegisterType::Group7x}) {
        DataMapEntry entry;
        entry.EnronType = group;
        model.addEntry(holdingKey(EnronModbus::baseAddress(group)), entry);
    }
    model.addEntry(holdingKey(10), {});
    DataMapFilterProxy proxy;
    proxy.setSourceModel(&model);
    QCOMPARE(proxy.rowCount(), 3);
    proxy.setFilterTypeIndex(5);
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, ColType).data(DataMapRole::EnronType).toInt(), 1);
    proxy.setFilterTypeIndex(6);
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, ColType).data(DataMapRole::EnronType).toInt(), 2);
    proxy.setFilterTypeIndex(4);
    QCOMPARE(proxy.rowCount(), 1);
    definitions.EnronModbus = false;
    server.setModbusDefinitions(definitions);
    QTRY_COMPARE(proxy.rowCount(), 3);
    proxy.setFilterTypeIndex(5);
    QCOMPARE(proxy.rowCount(), 0);
    proxy.setFilterTypeIndex(-1);
    QCOMPARE(proxy.rowCount(), 3);
}

void TestDataMapDataModel::removingRowReleasesDeviceId()
{
    ModbusMultiServer server;
    QSignalSpy removedSpy(&server, &ModbusMultiServer::deviceIdRemoved);
    DataMapDataModel model(server);

    model.addEntry(holdingKey(0), {});
    model.removeEntries({0});

    QCOMPARE(removedSpy.count(), 1);
}

void TestDataMapDataModel::removingRowAfterDataTypeChangeReleasesDeviceId()
{
    ModbusMultiServer server;
    QSignalSpy removedSpy(&server, &ModbusMultiServer::deviceIdRemoved);
    DataMapDataModel model(server);

    model.addEntry(holdingKey(0), {});
    QVERIFY(model.setData(model.index(0, ColDataType), QStringLiteral("Float32")));
    QVERIFY(model.setData(model.index(0, ColDataType), QStringLiteral("Int16")));
    model.removeEntries({0});

    QCOMPARE(removedSpy.count(), 1);
}

void TestDataMapDataModel::removingDuplicateKeyRowsReleasesDeviceId()
{
    ModbusMultiServer server;
    QSignalSpy removedSpy(&server, &ModbusMultiServer::deviceIdRemoved);
    DataMapDataModel model(server);

    model.addEntry(holdingKey(0), {});
    model.addEntry(holdingKey(1), {});
    QVERIFY(model.setData(model.index(1, ColAddress), QStringLiteral("1")));
    QCOMPARE(model.keyForRow(1).Address, quint16(0));

    model.removeEntries({0});
    QCOMPARE(removedSpy.count(), 0);
    model.removeEntries({0});
    QCOMPARE(removedSpy.count(), 1);
}

void TestDataMapDataModel::removingRowResetsStoredData()
{
    ModbusMultiServer server;
    DataMapDataModel model(server);

    DataMapEntry entry;
    entry.comment = QStringLiteral("setpoint");
    model.addEntry(holdingKey(0), entry);
    writeHolding(server, 0, 111);
    QCOMPARE(readHolding(server, 0), quint16(111));

    model.removeEntries({0});

    QCOMPARE(readHolding(server, 0), quint16(0));
    QVERIFY(server.description(1, QModbusDataUnit::HoldingRegisters, 0).isEmpty());
    QVERIFY(!server.timestamp(1, QModbusDataUnit::HoldingRegisters, 0).isValid());
    QVERIFY(server.configuredValueMap(1, QModbusDataUnit::HoldingRegisters, 0, 1).isEmpty());
}

void TestDataMapDataModel::removingRowKeepsDataCoveredByAnotherMap()
{
    ModbusMultiServer server;
    DataMapDataModel first(server);
    DataMapDataModel second(server);

    first.addEntry(holdingKey(0), {});
    second.addEntry(holdingKey(0), {});
    writeHolding(server, 0, 111);

    first.removeEntries({0});

    QCOMPARE(readHolding(server, 0), quint16(111));
}

void TestDataMapDataModel::removingMultiRegisterRowResetsEveryRegister()
{
    ModbusMultiServer server;
    DataMapDataModel model(server);

    DataMapEntry entry;
    entry.type = DataType::Float32;
    model.addEntry(holdingKey(4), entry);
    server.setData(1, QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 4, QVector<quint16>{7, 8}));

    model.removeEntries({0});

    QCOMPARE(readHolding(server, 4), quint16(0));
    QCOMPARE(readHolding(server, 5), quint16(0));
}

void TestDataMapDataModel::removingRowKeepsDataCoveredByDataViewRange()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 100);
    DataMapDataModel model(server);

    model.addEntry(holdingKey(5), {});
    model.addEntry(holdingKey(150), {});
    writeHolding(server, 5, 55);
    writeHolding(server, 150, 150);

    model.removeEntries({0, 1});

    QCOMPARE(readHolding(server, 5), quint16(55));
    QCOMPARE(readHolding(server, 150), quint16(0));
}

QTEST_MAIN(TestDataMapDataModel)
#include "test_datamapdatamodel.moc"
