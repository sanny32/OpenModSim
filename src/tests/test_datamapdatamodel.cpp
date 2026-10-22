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
    void removingRowReleasesDeviceId();
    void removingRowAfterDataTypeChangeReleasesDeviceId();
    void removingDuplicateKeyRowsReleasesDeviceId();
    void removingRowResetsStoredData();
    void removingRowKeepsDataCoveredByAnotherMap();
    void removingMultiRegisterRowResetsEveryRegister();
    void removingRowKeepsDataCoveredByDataViewRange();
};

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
