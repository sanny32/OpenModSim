// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_projectaddressspacefilter.cpp
/// \brief Unit tests for project address-space filtering.
///

#include <QDateTime>
#include <QTest>

#include "projectaddressspacefilter.h"

class TestProjectAddressSpaceFilter : public QObject
{
    Q_OBJECT

private slots:
    void filtersValuesToProjectRanges();
    void filtersDescriptionsAndTimestamps();
    void keepsDataMapMultiRegisterSpan();
    void filtersSimulationsByOverlap();
    void buildsRangesForModifiedRuntimeData();
    void emptyProjectRangesDropRuntimeState();
    void zeroLengthAndMismatchedRangesDoNotMatch();
    void rangeBoundaryOverlapCases();
    void filtersMixedMapsWithOnlyOutsideItems();
    void modifiedRangesIgnoreInactiveSimulations();
};

///
/// \brief TestProjectAddressSpaceFilter::filtersValuesToProjectRanges
///
void TestProjectAddressSpaceFilter::filtersValuesToProjectRanges()
{
    const ProjectAddressSpaceRanges ranges = {
        { 1, QModbusDataUnit::HoldingRegisters, 10, 3 }
    };
    const ProjectAddressSpaceValues values = {
        { { 1, QModbusDataUnit::HoldingRegisters, 9 }, 90 },
        { { 1, QModbusDataUnit::HoldingRegisters, 10 }, 100 },
        { { 1, QModbusDataUnit::HoldingRegisters, 12 }, 120 },
        { { 1, QModbusDataUnit::HoldingRegisters, 13 }, 130 },
        { { 2, QModbusDataUnit::HoldingRegisters, 10 }, 210 }
    };

    const auto result = filterProjectAddressValues(values, ranges);

    QCOMPARE(result.size(), 2);
    QCOMPARE(result.at(0).Key.Address, quint16(10));
    QCOMPARE(result.at(0).Value, quint16(100));
    QCOMPARE(result.at(1).Key.Address, quint16(12));
    QCOMPARE(result.at(1).Value, quint16(120));
}

///
/// \brief TestProjectAddressSpaceFilter::filtersDescriptionsAndTimestamps
///
void TestProjectAddressSpaceFilter::filtersDescriptionsAndTimestamps()
{
    const ProjectAddressSpaceRanges ranges = {
        { 1, QModbusDataUnit::InputRegisters, 20, 2 }
    };
    const QDateTime timestamp = QDateTime::fromString(QStringLiteral("2026-07-01T12:00:00.000"), Qt::ISODateWithMs);
    AddressDescriptionMap descriptions;
    descriptions.insert({ 1, QModbusDataUnit::InputRegisters, 20 }, QStringLiteral("keep"));
    descriptions.insert({ 1, QModbusDataUnit::InputRegisters, 22 }, QStringLiteral("drop"));
    descriptions.insert({ 2, QModbusDataUnit::InputRegisters, 20 }, QStringLiteral("drop unit"));
    AddressTimestampMap timestamps;
    timestamps.insert({ 1, QModbusDataUnit::InputRegisters, 21 }, timestamp);
    timestamps.insert({ 1, QModbusDataUnit::HoldingRegisters, 21 }, timestamp.addSecs(1));

    const auto filteredDescriptions = filterProjectAddressDescriptions(descriptions, ranges);
    const auto filteredTimestamps = filterProjectAddressTimestamps(timestamps, ranges);

    QCOMPARE(filteredDescriptions.size(), 1);
    QCOMPARE(filteredDescriptions.value({ 1, QModbusDataUnit::InputRegisters, 20 }), QStringLiteral("keep"));
    QCOMPARE(filteredTimestamps.size(), 1);
    QCOMPARE(filteredTimestamps.value({ 1, QModbusDataUnit::InputRegisters, 21 }), timestamp);
}

///
/// \brief TestProjectAddressSpaceFilter::keepsDataMapMultiRegisterSpan
///
void TestProjectAddressSpaceFilter::keepsDataMapMultiRegisterSpan()
{
    const ProjectAddressSpaceRanges ranges = {
        { 1, QModbusDataUnit::HoldingRegisters, 30, 2 }
    };
    const ProjectAddressSpaceValues values = {
        { { 1, QModbusDataUnit::HoldingRegisters, 30 }, 300 },
        { { 1, QModbusDataUnit::HoldingRegisters, 31 }, 310 },
        { { 1, QModbusDataUnit::HoldingRegisters, 32 }, 320 }
    };

    const auto result = filterProjectAddressValues(values, ranges);

    QCOMPARE(result.size(), 2);
    QCOMPARE(result.at(0).Key.Address, quint16(30));
    QCOMPARE(result.at(1).Key.Address, quint16(31));
}

///
/// \brief TestProjectAddressSpaceFilter::filtersSimulationsByOverlap
///
void TestProjectAddressSpaceFilter::filtersSimulationsByOverlap()
{
    const ProjectAddressSpaceRanges ranges = {
        { 1, QModbusDataUnit::HoldingRegisters, 40, 2 },
        { 1, QModbusDataUnit::InputRegisters, 60, 1 }
    };
    ModbusSimulationParams activeSingle;
    activeSingle.Mode = SimulationMode::Increment;
    activeSingle.DataMode = DataType::UInt16;
    ModbusSimulationParams staleSingle = activeSingle;
    ModbusSimulationParams overlappingMulti = activeSingle;
    overlappingMulti.DataMode = DataType::Int32;
    ModbusSimulationMap2 simulations;
    simulations.insert({ 1, QModbusDataUnit::HoldingRegisters, 40 }, activeSingle);
    simulations.insert({ 1, QModbusDataUnit::HoldingRegisters, 42 }, staleSingle);
    simulations.insert({ 1, QModbusDataUnit::InputRegisters, 59 }, overlappingMulti);
    simulations.insert({ 2, QModbusDataUnit::HoldingRegisters, 40 }, activeSingle);

    const auto result = filterProjectAddressSimulations(simulations, ranges);

    QCOMPARE(result.size(), 2);
    QVERIFY(result.contains({ 1, QModbusDataUnit::HoldingRegisters, 40 }));
    QVERIFY(result.contains({ 1, QModbusDataUnit::InputRegisters, 59 }));
    QVERIFY(!result.contains({ 1, QModbusDataUnit::HoldingRegisters, 42 }));
    QVERIFY(!result.contains({ 2, QModbusDataUnit::HoldingRegisters, 40 }));
}

///
/// \brief TestProjectAddressSpaceFilter::buildsRangesForModifiedRuntimeData
///
void TestProjectAddressSpaceFilter::buildsRangesForModifiedRuntimeData()
{
    AddressDescriptionMap descriptions;
    descriptions.insert({ 1, QModbusDataUnit::Coils, 3 }, QStringLiteral("description"));
    AddressTimestampMap timestamps;
    timestamps.insert({ 2, QModbusDataUnit::HoldingRegisters, 7 }, QDateTime::currentDateTime());
    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::Int32;
    ModbusSimulationMap2 simulations;
    simulations.insert({ 3, QModbusDataUnit::InputRegisters, 10 }, params);

    const auto ranges = projectAddressSpaceModifiedRanges(descriptions, timestamps, simulations);

    QVERIFY(projectAddressSpaceOverlaps(ranges, { 1, QModbusDataUnit::Coils, 3, 1 }));
    QVERIFY(projectAddressSpaceOverlaps(ranges, { 2, QModbusDataUnit::HoldingRegisters, 7, 1 }));
    QVERIFY(projectAddressSpaceOverlaps(ranges, { 3, QModbusDataUnit::InputRegisters, 10, 2 }));
    QVERIFY(projectAddressSpaceContains(ranges, { 3, QModbusDataUnit::InputRegisters, 11 }));
    QVERIFY(!projectAddressSpaceContains(ranges, { 3, QModbusDataUnit::InputRegisters, 12 }));
}

///
/// \brief TestProjectAddressSpaceFilter::emptyProjectRangesDropRuntimeState
///
void TestProjectAddressSpaceFilter::emptyProjectRangesDropRuntimeState()
{
    const ProjectAddressSpaceRanges ranges;
    const ProjectAddressSpaceValues values = {
        { { 1, QModbusDataUnit::HoldingRegisters, 1 }, 10 }
    };
    AddressDescriptionMap descriptions;
    descriptions.insert({ 1, QModbusDataUnit::HoldingRegisters, 1 }, QStringLiteral("drop"));
    AddressTimestampMap timestamps;
    timestamps.insert({ 1, QModbusDataUnit::HoldingRegisters, 1 }, QDateTime::currentDateTime());
    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::UInt16;
    ModbusSimulationMap2 simulations;
    simulations.insert({ 1, QModbusDataUnit::HoldingRegisters, 1 }, params);

    QVERIFY(filterProjectAddressValues(values, ranges).isEmpty());
    QVERIFY(filterProjectAddressDescriptions(descriptions, ranges).isEmpty());
    QVERIFY(filterProjectAddressTimestamps(timestamps, ranges).isEmpty());
    QVERIFY(filterProjectAddressSimulations(simulations, ranges).isEmpty());
}

///
/// \brief TestProjectAddressSpaceFilter::zeroLengthAndMismatchedRangesDoNotMatch
///
void TestProjectAddressSpaceFilter::zeroLengthAndMismatchedRangesDoNotMatch()
{
    const ProjectAddressSpaceRanges ranges = {
        { 1, QModbusDataUnit::Coils, 10, 0 },
        { 2, QModbusDataUnit::Coils, 20, 2 },
        { 1, QModbusDataUnit::HoldingRegisters, 20, 2 }
    };

    QVERIFY(!projectAddressSpaceContains(ranges, { 1, QModbusDataUnit::Coils, 10 }));
    QVERIFY(!projectAddressSpaceContains(ranges, { 1, QModbusDataUnit::Coils, 20 }));
    QVERIFY(!projectAddressSpaceContains(ranges, { 2, QModbusDataUnit::HoldingRegisters, 20 }));
    QVERIFY(!projectAddressSpaceOverlaps(ranges, { 1, QModbusDataUnit::Coils, 10, 0 }));
    QVERIFY(!projectAddressSpaceOverlaps(ranges, { 1, QModbusDataUnit::Coils, 20, 1 }));
    QVERIFY(!projectAddressSpaceOverlaps(ranges, { 2, QModbusDataUnit::HoldingRegisters, 20, 1 }));
}

///
/// \brief TestProjectAddressSpaceFilter::rangeBoundaryOverlapCases
///
void TestProjectAddressSpaceFilter::rangeBoundaryOverlapCases()
{
    const ProjectAddressSpaceRanges ranges = {
        { 1, QModbusDataUnit::HoldingRegisters, 10, 5 },
        { 1, QModbusDataUnit::HoldingRegisters, 30, 5 }
    };

    QVERIFY(projectAddressSpaceContains(ranges, { 1, QModbusDataUnit::HoldingRegisters, 10 }));
    QVERIFY(projectAddressSpaceContains(ranges, { 1, QModbusDataUnit::HoldingRegisters, 14 }));
    QVERIFY(!projectAddressSpaceContains(ranges, { 1, QModbusDataUnit::HoldingRegisters, 15 }));

    QVERIFY(projectAddressSpaceOverlaps(ranges, { 1, QModbusDataUnit::HoldingRegisters, 5, 6 }));
    QVERIFY(projectAddressSpaceOverlaps(ranges, { 1, QModbusDataUnit::HoldingRegisters, 14, 1 }));
    QVERIFY(projectAddressSpaceOverlaps(ranges, { 1, QModbusDataUnit::HoldingRegisters, 12, 10 }));
    QVERIFY(!projectAddressSpaceOverlaps(ranges, { 1, QModbusDataUnit::HoldingRegisters, 5, 4 }));
    QVERIFY(!projectAddressSpaceOverlaps(ranges, { 1, QModbusDataUnit::HoldingRegisters, 15, 15 }));
}

///
/// \brief TestProjectAddressSpaceFilter::filtersMixedMapsWithOnlyOutsideItems
///
void TestProjectAddressSpaceFilter::filtersMixedMapsWithOnlyOutsideItems()
{
    const ProjectAddressSpaceRanges ranges = {
        { 7, QModbusDataUnit::Coils, 100, 2 }
    };

    ProjectAddressSpaceValues values = {
        { { 7, QModbusDataUnit::Coils, 99 }, 1 },
        { { 7, QModbusDataUnit::Coils, 102 }, 2 },
        { { 8, QModbusDataUnit::Coils, 100 }, 3 },
        { { 7, QModbusDataUnit::DiscreteInputs, 100 }, 4 }
    };
    QVERIFY(filterProjectAddressValues(values, ranges).isEmpty());

    AddressDescriptionMap descriptions;
    descriptions.insert({ 7, QModbusDataUnit::Coils, 99 }, QStringLiteral("before"));
    descriptions.insert({ 8, QModbusDataUnit::Coils, 100 }, QStringLiteral("device"));
    QVERIFY(filterProjectAddressDescriptions(descriptions, ranges).isEmpty());

    AddressTimestampMap timestamps;
    timestamps.insert({ 7, QModbusDataUnit::Coils, 102 }, QDateTime::currentDateTime());
    timestamps.insert({ 7, QModbusDataUnit::DiscreteInputs, 100 }, QDateTime::currentDateTime());
    QVERIFY(filterProjectAddressTimestamps(timestamps, ranges).isEmpty());

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::UInt16;
    ModbusSimulationMap2 simulations;
    simulations.insert({ 7, QModbusDataUnit::Coils, 99 }, params);
    simulations.insert({ 7, QModbusDataUnit::Coils, 102 }, params);
    simulations.insert({ 8, QModbusDataUnit::Coils, 100 }, params);
    simulations.insert({ 7, QModbusDataUnit::DiscreteInputs, 100 }, params);
    QVERIFY(filterProjectAddressSimulations(simulations, ranges).isEmpty());
}

///
/// \brief TestProjectAddressSpaceFilter::modifiedRangesIgnoreInactiveSimulations
///
void TestProjectAddressSpaceFilter::modifiedRangesIgnoreInactiveSimulations()
{
    ModbusSimulationParams disabled;
    disabled.Mode = SimulationMode::Disabled;
    disabled.DataMode = DataType::Int32;

    ModbusSimulationParams off;
    off.Mode = SimulationMode::Off;
    off.DataMode = DataType::Int64;

    ModbusSimulationParams active;
    active.Mode = SimulationMode::Toggle;
    active.DataMode = DataType::UInt16;

    ModbusSimulationMap2 simulations;
    simulations.insert({ 1, QModbusDataUnit::Coils, 1 }, disabled);
    simulations.insert({ 1, QModbusDataUnit::Coils, 2 }, off);
    simulations.insert({ 1, QModbusDataUnit::Coils, 3 }, active);

    const auto ranges = projectAddressSpaceModifiedRanges({}, {}, simulations);

    QCOMPARE(ranges.size(), 1);
    QCOMPARE(ranges.first().StartAddress, quint16(3));
    QCOMPARE(ranges.first().Length, quint16(1));
}

QTEST_APPLESS_MAIN(TestProjectAddressSpaceFilter)

#include "test_projectaddressspacefilter.moc"
