// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_projectaddressspacexml.cpp
/// \brief Unit tests for the project AddressSpace element reader, writer and applier.
///

#include <QBuffer>
#include <QDateTime>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "datasimulator.h"
#include "modbusmultiserver.h"
#include "projectaddressspacexml.h"

class TestProjectAddressSpaceXml : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsAddressSpace();
    void readsHandWrittenXml();
    void skipsMalformedEntries();
    void writeOmitsZeroAndDuplicateValues();
    void writeOmitsInactiveSimulations();
    void applyRespectsReplaceFlag();
    void applyHonorsMissingSections();
    void applyBatchesContiguousValues();
    void applySplitsRunsOnGaps();
    void applyKeepsMappedPartOfOverflowingRun();
    void omitsTimestampsWhenDisabled();
    void writesConfiguredValuesInsteadOfSimulatedOnes();
    void keepsUserEditsWhenRuntimeValuesDisabled();
};

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
/// \brief Parses the given XML and reads its AddressSpace root element into a payload.
/// \param xml The document text with an AddressSpace root element.
/// \return The parsed payload.
///
ProjectAddressSpacePayload payloadFromXml(const QString& xml)
{
    QXmlStreamReader reader(xml);
    ProjectAddressSpacePayload payload;
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String("AddressSpace"))
            readProjectAddressSpace(reader, payload);
        else
            reader.skipCurrentElement();
    }
    return payload;
}

}

void TestProjectAddressSpaceXml::roundTripsAddressSpace()
{
    ModbusMultiServer source;
    source.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 20);
    source.setData(1, [] {
        QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, 5, 2);
        unit.setValue(0, 123);
        unit.setValue(1, 456);
        return unit;
    }());
    source.setDescription(1, QModbusDataUnit::HoldingRegisters, 5, QStringLiteral("pump"), WriteSource::User);
    const auto timestamp = QDateTime::fromString(QStringLiteral("2026-07-15T10:20:30Z"), Qt::ISODate);
    source.setTimestamp(1, QModbusDataUnit::HoldingRegisters, 5, timestamp);

    ModbusSimulationParams simParams;
    simParams.Mode = SimulationMode::Increment;
    simParams.DataMode = DataType::UInt16;
    ModbusSimulationMap2 simulations;
    simulations.insert({1, QModbusDataUnit::HoldingRegisters, 6}, simParams);

    const ProjectAddressSpaceRanges ranges = {{1, QModbusDataUnit::HoldingRegisters, 0, 20}};

    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QXmlStreamWriter w(&buffer);
    w.writeStartDocument();
    writeProjectAddressSpace(w, source, simulations, ranges);
    w.writeEndDocument();
    buffer.close();

    const auto payload = payloadFromXml(QString::fromUtf8(buffer.data()));
    QVERIFY(payload.HasDescriptions);
    QVERIFY(payload.HasTimestamps);
    QCOMPARE(payload.Values.size(), 2);
    QCOMPARE(payload.Simulations.size(), 1);

    ModbusMultiServer target;
    target.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 20);
    DataSimulator simulator;
    applyProjectAddressSpace(payload, target, &simulator, true);

    QCOMPARE(target.data(1, QModbusDataUnit::HoldingRegisters, 5, 1).value(0), quint16(123));
    QCOMPARE(target.data(1, QModbusDataUnit::HoldingRegisters, 6, 1).value(0), quint16(456));
    QCOMPARE(target.description(1, QModbusDataUnit::HoldingRegisters, 5), QStringLiteral("pump"));
    QCOMPARE(target.timestamp(1, QModbusDataUnit::HoldingRegisters, 5), timestamp);
    QCOMPARE(simulator.simulationMap().size(), 1);
    QCOMPARE(simulator.simulationMap().value({1, QModbusDataUnit::HoldingRegisters, 6}).Mode,
             SimulationMode::Increment);
}

void TestProjectAddressSpaceXml::readsHandWrittenXml()
{
    const QString xml = QStringLiteral(
        "<AddressSpace>"
        "<ModbusDataValues>"
        "<Value DeviceId=\"2\" Type=\"4\" Address=\"7\">777</Value>"
        "</ModbusDataValues>"
        "</AddressSpace>");

    const auto payload = payloadFromXml(xml);
    QVERIFY(!payload.HasDescriptions);
    QVERIFY(!payload.HasTimestamps);
    QCOMPARE(payload.Values.size(), 1);
    QCOMPARE(payload.Values.first().DeviceId, quint8(2));
    QCOMPARE(payload.Values.first().Type, QModbusDataUnit::HoldingRegisters);
    QCOMPARE(payload.Values.first().Address, quint16(7));
    QCOMPARE(payload.Values.first().RegisterValue, quint16(777));
    QVERIFY(payload.Simulations.isEmpty());
}

void TestProjectAddressSpaceXml::skipsMalformedEntries()
{
    const QString xml = QStringLiteral(
        "<AddressSpace>"
        "<ModbusDataValues>"
        "<Value DeviceId=\"abc\" Type=\"4\" Address=\"7\">1</Value>"
        "<Value DeviceId=\"1\" Type=\"4\">2</Value>"
        "<Value DeviceId=\"1\" Type=\"4\" Address=\"8\">oops</Value>"
        "<Value DeviceId=\"1\" Type=\"4\" Address=\"9\">9</Value>"
        "</ModbusDataValues>"
        "<ModbusSimulationMap>"
        "<Simulation DeviceId=\"1\" Type=\"4\" Address=\"5\"/>"
        "<Simulation DeviceId=\"nope\" Type=\"4\" Address=\"5\"/>"
        "</ModbusSimulationMap>"
        "<Unknown/>"
        "</AddressSpace>");

    const auto payload = payloadFromXml(xml);
    QCOMPARE(payload.Values.size(), 1);
    QCOMPARE(payload.Values.first().Address, quint16(9));
    QVERIFY(payload.Simulations.isEmpty());
}

void TestProjectAddressSpaceXml::writeOmitsZeroAndDuplicateValues()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 20);
    server.setData(1, [] {
        QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, 3, 3);
        unit.setValue(0, 30);
        unit.setValue(1, 0);
        unit.setValue(2, 50);
        return unit;
    }());

    // Overlapping ranges must serialize each register only once, zero values never.
    const ProjectAddressSpaceRanges ranges = {
        {1, QModbusDataUnit::HoldingRegisters, 0, 10},
        {1, QModbusDataUnit::HoldingRegisters, 3, 3}
    };

    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QXmlStreamWriter w(&buffer);
    w.writeStartDocument();
    writeProjectAddressSpace(w, server, {}, ranges);
    w.writeEndDocument();
    buffer.close();

    const auto payload = payloadFromXml(QString::fromUtf8(buffer.data()));
    QCOMPARE(payload.Values.size(), 2);
    QCOMPARE(payload.Values.at(0).Address, quint16(3));
    QCOMPARE(payload.Values.at(0).RegisterValue, quint16(30));
    QCOMPARE(payload.Values.at(1).Address, quint16(5));
    QCOMPARE(payload.Values.at(1).RegisterValue, quint16(50));
}

void TestProjectAddressSpaceXml::writeOmitsInactiveSimulations()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 20);

    ModbusSimulationParams active;
    active.Mode = SimulationMode::Random;
    ModbusSimulationParams off;
    off.Mode = SimulationMode::Off;
    ModbusSimulationParams disabled;
    disabled.Mode = SimulationMode::Disabled;

    ModbusSimulationMap2 simulations;
    simulations.insert({1, QModbusDataUnit::HoldingRegisters, 1}, active);
    simulations.insert({1, QModbusDataUnit::HoldingRegisters, 2}, off);
    simulations.insert({1, QModbusDataUnit::HoldingRegisters, 3}, disabled);

    const ProjectAddressSpaceRanges ranges = {{1, QModbusDataUnit::HoldingRegisters, 0, 20}};

    QBuffer buffer;
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QXmlStreamWriter w(&buffer);
    w.writeStartDocument();
    writeProjectAddressSpace(w, server, simulations, ranges);
    w.writeEndDocument();
    buffer.close();

    const auto payload = payloadFromXml(QString::fromUtf8(buffer.data()));
    QCOMPARE(payload.Simulations.size(), 1);
    QCOMPARE(payload.Simulations.first().Address, quint16(1));
    QCOMPARE(payload.Simulations.first().Params.Mode, SimulationMode::Random);
}

void TestProjectAddressSpaceXml::applyRespectsReplaceFlag()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 20);
    server.setDescription(1, QModbusDataUnit::HoldingRegisters, 1, QStringLiteral("existing"), WriteSource::User);

    ProjectAddressSpacePayload payload;
    payload.HasDescriptions = true;
    payload.Descriptions.insert(key(1, QModbusDataUnit::HoldingRegisters, 2), QStringLiteral("merged"));

    applyProjectAddressSpace(payload, server, nullptr, false);
    QCOMPARE(server.description(1, QModbusDataUnit::HoldingRegisters, 1), QStringLiteral("existing"));
    QCOMPARE(server.description(1, QModbusDataUnit::HoldingRegisters, 2), QStringLiteral("merged"));

    applyProjectAddressSpace(payload, server, nullptr, true);
    QVERIFY(server.description(1, QModbusDataUnit::HoldingRegisters, 1).isEmpty());
    QCOMPARE(server.description(1, QModbusDataUnit::HoldingRegisters, 2), QStringLiteral("merged"));
}

void TestProjectAddressSpaceXml::applyHonorsMissingSections()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 20);
    server.setDescription(1, QModbusDataUnit::HoldingRegisters, 1, QStringLiteral("kept"), WriteSource::User);
    const auto timestamp = QDateTime::fromString(QStringLiteral("2026-07-15T10:20:30Z"), Qt::ISODate);
    server.setTimestamp(1, QModbusDataUnit::HoldingRegisters, 1, timestamp);

    // A payload without description/timestamp sections must not touch existing metadata,
    // even with replace semantics.
    ProjectAddressSpacePayload payload;
    applyProjectAddressSpace(payload, server, nullptr, true);

    QCOMPARE(server.description(1, QModbusDataUnit::HoldingRegisters, 1), QStringLiteral("kept"));
    QCOMPARE(server.timestamp(1, QModbusDataUnit::HoldingRegisters, 1), timestamp);
}

///
/// \brief Regression for issue #126: a contiguous block of values must notify
/// views once per run, not once per register.
///
void TestProjectAddressSpaceXml::applyBatchesContiguousValues()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 300);

    ProjectAddressSpacePayload payload;
    for (quint16 a = 0; a < 200; ++a)
        payload.Values.append({1, QModbusDataUnit::HoldingRegisters, a, quint16(a + 1)});

    QSignalSpy dataSpy(&server, &ModbusMultiServer::dataChanged);
    applyProjectAddressSpace(payload, server, nullptr, true);

    QCOMPARE(dataSpy.count(), 1);
    const auto stored = server.data(1, QModbusDataUnit::HoldingRegisters, 0, 200);
    for (quint16 a = 0; a < 200; ++a)
        QCOMPARE(stored.value(a), quint16(a + 1));
}

///
/// \brief Gaps must split the block so unwritten addresses stay untouched.
///
void TestProjectAddressSpaceXml::applySplitsRunsOnGaps()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 100);

    ProjectAddressSpacePayload payload;
    for (quint16 a : {quint16(0), quint16(1), quint16(2), quint16(50), quint16(51)})
        payload.Values.append({1, QModbusDataUnit::HoldingRegisters, a, quint16(a + 7)});

    QSignalSpy dataSpy(&server, &ModbusMultiServer::dataChanged);
    applyProjectAddressSpace(payload, server, nullptr, true);

    QCOMPARE(dataSpy.count(), 2);
    QCOMPARE(server.data(1, QModbusDataUnit::HoldingRegisters, 2, 1).value(0), quint16(9));
    QCOMPARE(server.data(1, QModbusDataUnit::HoldingRegisters, 3, 1).value(0), quint16(0));
    QCOMPARE(server.data(1, QModbusDataUnit::HoldingRegisters, 50, 1).value(0), quint16(57));
}

///
/// \brief A run crossing the end of the unit map must still apply the mapped part
/// to the unit map instead of being dropped as a whole. Note that no transport
/// server is attached here, so this covers the unit map only.
///
void TestProjectAddressSpaceXml::applyKeepsMappedPartOfOverflowingRun()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 10);

    ProjectAddressSpacePayload payload;
    for (quint16 a = 0; a < 20; ++a)
        payload.Values.append({1, QModbusDataUnit::HoldingRegisters, a, quint16(a + 1)});

    applyProjectAddressSpace(payload, server, nullptr, true);

    const auto stored = server.data(1, QModbusDataUnit::HoldingRegisters, 0, 10);
    for (quint16 a = 0; a < 10; ++a)
        QCOMPARE(stored.value(a), quint16(a + 1));
}

namespace {

///
/// \brief Writes the AddressSpace of the given server and parses it back.
/// \param server The server to serialize.
/// \param ranges The ranges to serialize.
/// \param options The write options under test.
/// \return The parsed payload.
///
ProjectAddressSpacePayload writeAndReadBack(ModbusMultiServer& server,
                                            const ProjectAddressSpaceRanges& ranges,
                                            const ProjectAddressSpaceWriteOptions& options)
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QXmlStreamWriter w(&buffer);
    w.writeStartDocument();
    writeProjectAddressSpace(w, server, {}, ranges, options);
    w.writeEndDocument();
    buffer.close();

    return payloadFromXml(QString::fromUtf8(buffer.data()));
}

///
/// \brief Builds a single-register data unit.
/// \param address Register address.
/// \param value Register value.
/// \return The data unit.
///
QModbusDataUnit holdingRegister(quint16 address, quint16 value)
{
    QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, address, 1);
    unit.setValue(0, value);
    return unit;
}

}

/// \brief Verifies the timestamp map is left out when the preference is disabled.
void TestProjectAddressSpaceXml::omitsTimestampsWhenDisabled()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 20);
    server.setData(1, holdingRegister(5, 123), WriteSource::ProjectLoad);
    server.setTimestamp(1, QModbusDataUnit::HoldingRegisters, 5, QDateTime::currentDateTime());

    const ProjectAddressSpaceRanges ranges = {{1, QModbusDataUnit::HoldingRegisters, 0, 20}};

    QVERIFY(writeAndReadBack(server, ranges, {true, true}).HasTimestamps);
    QVERIFY(!writeAndReadBack(server, ranges, {false, true}).HasTimestamps);
}

/// \brief Verifies a running simulation does not reach the file when runtime values are
/// disabled: the value loaded with the project is written instead, and a register the
/// simulation alone produced is left out.
void TestProjectAddressSpaceXml::writesConfiguredValuesInsteadOfSimulatedOnes()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 20);
    server.setData(1, holdingRegister(5, 100), WriteSource::ProjectLoad);
    server.setData(1, holdingRegister(5, 999), WriteSource::Simulator);
    server.setData(1, holdingRegister(6, 777), WriteSource::Simulator);

    const ProjectAddressSpaceRanges ranges = {{1, QModbusDataUnit::HoldingRegisters, 0, 20}};

    const auto runtime = writeAndReadBack(server, ranges, {true, true});
    QCOMPARE(runtime.Values.size(), 2);

    const auto configured = writeAndReadBack(server, ranges, {true, false});
    QCOMPARE(configured.Values.size(), 1);
    QCOMPARE(configured.Values.at(0).Address, quint16(5));
    QCOMPARE(configured.Values.at(0).RegisterValue, quint16(100));
}

/// \brief Verifies a register edited by hand counts as configuration, including when the
/// entered value matches what a simulation had already produced.
void TestProjectAddressSpaceXml::keepsUserEditsWhenRuntimeValuesDisabled()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 20);
    server.setData(1, holdingRegister(5, 42), WriteSource::User);
    server.setData(1, holdingRegister(6, 55), WriteSource::Simulator);
    server.setData(1, holdingRegister(6, 55), WriteSource::User);

    const ProjectAddressSpaceRanges ranges = {{1, QModbusDataUnit::HoldingRegisters, 0, 20}};
    const auto configured = writeAndReadBack(server, ranges, {true, false});

    QCOMPARE(configured.Values.size(), 2);
    QCOMPARE(configured.Values.at(0).RegisterValue, quint16(42));
    QCOMPARE(configured.Values.at(1).RegisterValue, quint16(55));
}

QTEST_GUILESS_MAIN(TestProjectAddressSpaceXml)
#include "test_projectaddressspacexml.moc"
