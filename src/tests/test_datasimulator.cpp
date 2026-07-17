// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_datasimulator.cpp
/// \brief Unit tests for DataSimulator registration, lookup and emitted updates.
///

#include <initializer_list>

#include <QSignalSpy>
#include <QTest>

#include "datasimulator.h"

namespace {

///
/// \brief compareAddresses
/// \param actual
/// \param expected
///
void compareAddresses(const QVector<quint16>& actual, std::initializer_list<quint16> expected)
{
    QCOMPARE(actual.size(), int(expected.size()));

    int index = 0;
    for (const quint16 address : expected) {
        QCOMPARE(actual.at(index), address);
        ++index;
    }
}

}

class TestDataSimulator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void startRegistersSimulation();
    void multiRegisterReservesFollowingAddresses();
    void stopSimulationRemovesEntry();
    void stopMultiRegisterSimulationRemovesAllAddresses();
    void stopMissingSimulationDoesNotEmit();
    void stopSimulationsClearsEverything();
    void canStartSimulationBlocksOverlap();
    void canStartSimulationAllowsSameActiveAddress();
    void canStartSimulationAllowsSameActiveSingleRegisterAddress();
    void canStartSimulationAllowsDifferentDeviceAndRegisterType();
    void missingSimulationLookupReturnsDefaults();
    void startAndStopSignalsCarryExpectedPayload();
    void incrementEmitsInitialValue();
    void toggleAlternatesOnTimer();
    void incrementProgressesUpward();
    void decrementProgressesDownward();
    void randomStaysInRange();
    void randomCoversEveryRegisterDataType_data();
    void randomCoversEveryRegisterDataType();
    void incrementCoversEveryDataType_data();
    void incrementCoversEveryDataType();
    void decrementCoversEveryDataType_data();
    void decrementCoversEveryDataType();
    void incrementWrapsToRangeStart();
    void decrementWrapsToRangeEnd();
    void randomCoversBinaryRegisterTypes_data();
    void randomCoversBinaryRegisterTypes();
    void randomCoversInputRegisters();
    void simulationWithFutureNextRunIsSkipped();
    void zeroIntervalDoesNotEmitOnTimer();
    void offAndDisabledModesDoNotEmitOnTimer();
    void restartEmptySimulatorDoesNothing();
    void restartReemitsStarted();
    void pauseStopsEmissions();
    void decrementEmitsInitialValue();
    void nonSteppingModesDoNotEmitInitialValue();
    void disabledSecondaryAddressIsReportedInSimulationMap();
    void randomUnknownRegisterTypeDoesNotEmitValue();
    void incrementWrapsEveryDataTypeToRangeStart();
    void decrementWrapsEveryDataTypeToRangeEnd();
};

void TestDataSimulator::initTestCase()
{
    qRegisterMetaType<DataType>();
    qRegisterMetaType<RegisterOrder>();
    qRegisterMetaType<QModbusDataUnit::RegisterType>();
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");
}

void TestDataSimulator::startRegistersSimulation()
{
    DataSimulator simulator;
    QSignalSpy started(&simulator, &DataSimulator::simulationStarted);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::UInt16;
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 5, params);

    QCOMPARE(started.count(), 1);
    QVERIFY(simulator.hasSimulation(1, QModbusDataUnit::HoldingRegisters, 5));
    QCOMPARE(simulator.simulationParams(1, QModbusDataUnit::HoldingRegisters, 5).Mode, SimulationMode::Random);
}

void TestDataSimulator::multiRegisterReservesFollowingAddresses()
{
    DataSimulator simulator;

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::Int32;
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QCOMPARE(simulator.simulationMap().size(), 2);
    QCOMPARE(simulator.simulationParams(1, QModbusDataUnit::HoldingRegisters, 0).Mode, SimulationMode::Increment);
    QCOMPARE(simulator.simulationParams(1, QModbusDataUnit::HoldingRegisters, 1).Mode, SimulationMode::Disabled);
}

void TestDataSimulator::stopSimulationRemovesEntry()
{
    DataSimulator simulator;
    QSignalSpy stopped(&simulator, &DataSimulator::simulationStopped);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::Int32;
    simulator.startSimulation(2, QModbusDataUnit::InputRegisters, 10, params);
    simulator.stopSimulation(2, QModbusDataUnit::InputRegisters, 10);

    QCOMPARE(stopped.count(), 1);
    QVERIFY(!simulator.hasSimulation(2, QModbusDataUnit::InputRegisters, 10));
    QVERIFY(simulator.simulationMap().isEmpty());
}

void TestDataSimulator::stopMultiRegisterSimulationRemovesAllAddresses()
{
    DataSimulator simulator;
    QSignalSpy stopped(&simulator, &DataSimulator::simulationStopped);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::Float64;
    simulator.startSimulation(3, QModbusDataUnit::HoldingRegisters, 20, params);

    QCOMPARE(simulator.simulationMap().size(), 4);
    simulator.stopSimulation(3, QModbusDataUnit::HoldingRegisters, 20);

    QCOMPARE(stopped.count(), 1);
    const auto addresses = stopped.takeFirst().at(4).value<QVector<quint16>>();
    compareAddresses(addresses, {20, 21, 22, 23});
    QVERIFY(simulator.simulationMap().isEmpty());
}

void TestDataSimulator::stopMissingSimulationDoesNotEmit()
{
    DataSimulator simulator;
    QSignalSpy stopped(&simulator, &DataSimulator::simulationStopped);

    simulator.stopSimulation(2, QModbusDataUnit::InputRegisters, 10);

    QCOMPARE(stopped.count(), 0);
}

void TestDataSimulator::stopSimulationsClearsEverything()
{
    DataSimulator simulator;

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::UInt16;
    simulator.startSimulation(1, QModbusDataUnit::Coils, 0, params);
    simulator.startSimulation(1, QModbusDataUnit::Coils, 1, params);

    simulator.stopSimulations();
    QVERIFY(simulator.simulationMap().isEmpty());
}

void TestDataSimulator::canStartSimulationBlocksOverlap()
{
    DataSimulator simulator;

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::UInt16;
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 11, params);

    QVERIFY(!simulator.canStartSimulation(DataType::Int32, 1, QModbusDataUnit::HoldingRegisters, 10));
    QVERIFY(simulator.canStartSimulation(DataType::UInt16, 1, QModbusDataUnit::HoldingRegisters, 20));
}

void TestDataSimulator::canStartSimulationAllowsSameActiveAddress()
{
    DataSimulator simulator;

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::Int32;
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 10, params);

    QVERIFY(simulator.canStartSimulation(DataType::Int32, 1, QModbusDataUnit::HoldingRegisters, 10));
    QVERIFY(simulator.canStartSimulation(DataType::UInt16, 1, QModbusDataUnit::HoldingRegisters, 11));
}

void TestDataSimulator::canStartSimulationAllowsSameActiveSingleRegisterAddress()
{
    DataSimulator simulator;

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::UInt16;
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 7, params);

    QVERIFY(simulator.canStartSimulation(DataType::Float64, 1, QModbusDataUnit::HoldingRegisters, 7));
}

void TestDataSimulator::canStartSimulationAllowsDifferentDeviceAndRegisterType()
{
    DataSimulator simulator;

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::Int32;
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 10, params);

    QVERIFY(simulator.canStartSimulation(DataType::Int32, 2, QModbusDataUnit::HoldingRegisters, 10));
    QVERIFY(simulator.canStartSimulation(DataType::Int32, 1, QModbusDataUnit::InputRegisters, 10));
}

void TestDataSimulator::missingSimulationLookupReturnsDefaults()
{
    DataSimulator simulator;

    QVERIFY(!simulator.hasSimulation(1, QModbusDataUnit::HoldingRegisters, 99));
    const ModbusSimulationParams params = simulator.simulationParams(1, QModbusDataUnit::HoldingRegisters, 99);
    QCOMPARE(params.Mode, SimulationMode::Off);
    QCOMPARE(params.DataMode, DataType::Hex);
    QCOMPARE(params.RegOrder, RegisterOrder::MSRF);
}

void TestDataSimulator::startAndStopSignalsCarryExpectedPayload()
{
    DataSimulator simulator;
    QSignalSpy started(&simulator, &DataSimulator::simulationStarted);
    QSignalSpy stopped(&simulator, &DataSimulator::simulationStopped);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::UInt64;
    params.RegOrder = RegisterOrder::LSRF;
    simulator.startSimulation(7, QModbusDataUnit::InputRegisters, 30, params);

    QCOMPARE(started.count(), 1);
    QCOMPARE(started.first().at(0).value<DataType>(), DataType::UInt64);
    QCOMPARE(started.first().at(1).value<RegisterOrder>(), RegisterOrder::LSRF);
    QCOMPARE(started.first().at(2).toUInt(), 7u);
    QCOMPARE(started.first().at(3).value<QModbusDataUnit::RegisterType>(), QModbusDataUnit::InputRegisters);
    compareAddresses(started.first().at(4).value<QVector<quint16>>(), {30, 31, 32, 33});

    simulator.stopSimulation(7, QModbusDataUnit::InputRegisters, 30);

    QCOMPARE(stopped.count(), 1);
    QCOMPARE(stopped.first().at(0).value<DataType>(), DataType::UInt64);
    QCOMPARE(stopped.first().at(1).value<RegisterOrder>(), RegisterOrder::LSRF);
    QCOMPARE(stopped.first().at(2).toUInt(), 7u);
    QCOMPARE(stopped.first().at(3).value<QModbusDataUnit::RegisterType>(), QModbusDataUnit::InputRegisters);
    compareAddresses(stopped.first().at(4).value<QVector<quint16>>(), {30, 31, 32, 33});
}

void TestDataSimulator::incrementEmitsInitialValue()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::UInt16;
    params.Interval = 60000;
    params.IncrementParams.Range = QRange<double>(5., 100.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QCOMPARE(simulated.count(), 1);
    QCOMPARE(simulated.takeFirst().at(5).toDouble(), 5.);
}

void TestDataSimulator::toggleAlternatesOnTimer()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Toggle;
    params.DataMode = DataType::Binary;
    params.Interval = 20;
    simulator.startSimulation(1, QModbusDataUnit::Coils, 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 2, 2000);
    QCOMPARE(simulated.at(0).at(5).toBool(), true);
    QCOMPARE(simulated.at(1).at(5).toBool(), false);
}

void TestDataSimulator::incrementProgressesUpward()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::UInt16;
    params.Interval = 20;
    params.IncrementParams.Step = 1.;
    params.IncrementParams.Range = QRange<double>(0., 1000.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 3, 2000);
    QVERIFY(simulated.at(2).at(5).toUInt() > simulated.at(0).at(5).toUInt());
}

void TestDataSimulator::decrementProgressesDownward()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Decrement;
    params.DataMode = DataType::Float32;
    params.Interval = 20;
    params.DecrementParams.Step = 1.;
    params.DecrementParams.Range = QRange<double>(0., 1000.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 3, 2000);
    QVERIFY(simulated.at(2).at(5).toDouble() < simulated.at(0).at(5).toDouble());
}

void TestDataSimulator::randomStaysInRange()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::UInt16;
    params.Interval = 20;
    params.RandomParams.Range = QRange<double>(10., 20.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 1, 2000);
    const uint value = simulated.at(0).at(5).toUInt();
    QVERIFY(value >= 10 && value <= 20);
}

void TestDataSimulator::randomCoversEveryRegisterDataType_data()
{
    QTest::addColumn<int>("type");

    for(const auto type : {DataType::Binary, DataType::UInt16, DataType::Int16, DataType::Hex, DataType::Ansi,
                           DataType::Int32, DataType::UInt32, DataType::Float32, DataType::Float64, DataType::Int64,
                           DataType::UInt64})
        QTest::addRow("%s", qPrintable(enumToString(type))) << int(type);
}

void TestDataSimulator::randomCoversEveryRegisterDataType()
{
    QFETCH(int, type);

    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = static_cast<DataType>(type);
    params.Interval = 20;
    params.RandomParams.Range = QRange<double>(1., 3.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 1, 2000);
    const QVariant value = simulated.last().at(5);
    QVERIFY(value.isValid());
    QVERIFY(value.toDouble() >= 1.);
    QVERIFY(value.toDouble() < 4.);
}

void TestDataSimulator::incrementCoversEveryDataType_data()
{
    QTest::addColumn<int>("type");

    for(const auto type : {DataType::Binary, DataType::UInt16, DataType::Int16, DataType::Hex, DataType::Ansi,
                           DataType::Int32, DataType::UInt32, DataType::Float32, DataType::Float64, DataType::Int64,
                           DataType::UInt64})
        QTest::addRow("%s", qPrintable(enumToString(type))) << int(type);
}

void TestDataSimulator::incrementCoversEveryDataType()
{
    QFETCH(int, type);

    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = static_cast<DataType>(type);
    params.Interval = 20;
    params.IncrementParams.Step = 1.;
    params.IncrementParams.Range = QRange<double>(0., 3.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 2, 2000);
    QCOMPARE(simulated.first().at(5).toDouble(), 0.);
    QCOMPARE(simulated.last().at(5).toDouble(), 1.);
}

void TestDataSimulator::decrementCoversEveryDataType_data()
{
    QTest::addColumn<int>("type");

    for(const auto type : {DataType::Binary, DataType::UInt16, DataType::Int16, DataType::Hex, DataType::Ansi,
                           DataType::Int32, DataType::UInt32, DataType::Float32, DataType::Float64, DataType::Int64,
                           DataType::UInt64})
        QTest::addRow("%s", qPrintable(enumToString(type))) << int(type);
}

void TestDataSimulator::decrementCoversEveryDataType()
{
    QFETCH(int, type);

    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Decrement;
    params.DataMode = static_cast<DataType>(type);
    params.Interval = 20;
    params.DecrementParams.Step = 1.;
    params.DecrementParams.Range = QRange<double>(0., 3.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 2, 2000);
    QCOMPARE(simulated.first().at(5).toDouble(), 3.);
    QCOMPARE(simulated.last().at(5).toDouble(), 2.);
}

void TestDataSimulator::incrementWrapsToRangeStart()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::UInt16;
    params.Interval = 20;
    params.IncrementParams.Step = 1.;
    params.IncrementParams.Range = QRange<double>(0., 1.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 3, 2000);
    QCOMPARE(simulated.at(0).at(5).toUInt(), 0u);
    QCOMPARE(simulated.at(1).at(5).toUInt(), 1u);
    QCOMPARE(simulated.at(2).at(5).toUInt(), 0u);
}

void TestDataSimulator::decrementWrapsToRangeEnd()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Decrement;
    params.DataMode = DataType::UInt16;
    params.Interval = 20;
    params.DecrementParams.Step = 1.;
    params.DecrementParams.Range = QRange<double>(0., 1.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 3, 2000);
    QCOMPARE(simulated.at(0).at(5).toUInt(), 1u);
    QCOMPARE(simulated.at(1).at(5).toUInt(), 0u);
    QCOMPARE(simulated.at(2).at(5).toUInt(), 1u);
}

void TestDataSimulator::randomCoversBinaryRegisterTypes_data()
{
    QTest::addColumn<int>("registerType");

    QTest::newRow("coils") << int(QModbusDataUnit::Coils);
    QTest::newRow("discrete-inputs") << int(QModbusDataUnit::DiscreteInputs);
}

void TestDataSimulator::randomCoversBinaryRegisterTypes()
{
    QFETCH(int, registerType);

    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::Binary;
    params.Interval = 20;
    simulator.startSimulation(1, static_cast<QModbusDataUnit::RegisterType>(registerType), 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 1, 2000);
    QVERIFY(simulated.last().at(5).toUInt() <= 1u);
}

void TestDataSimulator::randomCoversInputRegisters()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::UInt16;
    params.Interval = 20;
    params.RandomParams.Range = QRange<double>(4., 6.);
    simulator.startSimulation(1, QModbusDataUnit::InputRegisters, 0, params);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 1, 2000);
    const uint value = simulated.last().at(5).toUInt();
    QVERIFY(value >= 4u);
    QVERIFY(value <= 6u);
}

void TestDataSimulator::simulationWithFutureNextRunIsSkipped()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams slowParams;
    slowParams.Mode = SimulationMode::Random;
    slowParams.DataMode = DataType::UInt16;
    slowParams.Interval = 1000;
    slowParams.RandomParams.Range = QRange<double>(10., 20.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, slowParams);

    ModbusSimulationParams fastParams;
    fastParams.Mode = SimulationMode::Toggle;
    fastParams.DataMode = DataType::Binary;
    fastParams.Interval = 20;
    simulator.startSimulation(1, QModbusDataUnit::Coils, 10, fastParams);

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 3, 2000);
    QTest::qWait(80);

    int slowEmissions = 0;
    int fastEmissions = 0;
    for (const auto& args : simulated) {
        if (args.at(3).value<QModbusDataUnit::RegisterType>() == QModbusDataUnit::HoldingRegisters
            && args.at(4).toUInt() == 0u) {
            ++slowEmissions;
        }

        if (args.at(3).value<QModbusDataUnit::RegisterType>() == QModbusDataUnit::Coils
            && args.at(4).toUInt() == 10u) {
            ++fastEmissions;
        }
    }

    QCOMPARE(slowEmissions, 1);
    QVERIFY(fastEmissions > 1);
}

void TestDataSimulator::zeroIntervalDoesNotEmitOnTimer()
{
    DataSimulator simulator;

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::UInt16;
    params.Interval = 0;
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);
    QTest::qWait(80);
    QCOMPARE(simulated.count(), 0);
}

void TestDataSimulator::offAndDisabledModesDoNotEmitOnTimer()
{
    DataSimulator simulator;

    ModbusSimulationParams offParams;
    offParams.Mode = SimulationMode::Off;
    offParams.DataMode = DataType::UInt16;
    offParams.Interval = 20;
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, offParams);

    ModbusSimulationParams multiParams;
    multiParams.Mode = SimulationMode::Increment;
    multiParams.DataMode = DataType::Int32;
    multiParams.Interval = 20;
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 10, multiParams);

    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);
    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 1, 2000);

    for (const auto& args : simulated) {
        QVERIFY(args.at(4).toUInt() != 0u);
        QVERIFY(args.at(4).toUInt() != 11u);
    }
}

void TestDataSimulator::restartEmptySimulatorDoesNothing()
{
    DataSimulator simulator;
    QSignalSpy started(&simulator, &DataSimulator::simulationStarted);

    simulator.restartSimulations();

    QCOMPARE(started.count(), 0);
    QVERIFY(simulator.simulationMap().isEmpty());
}

void TestDataSimulator::restartReemitsStarted()
{
    DataSimulator simulator;

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::UInt16;
    params.Interval = 60000;
    simulator.startSimulation(1, QModbusDataUnit::Coils, 0, params);

    QSignalSpy started(&simulator, &DataSimulator::simulationStarted);
    simulator.restartSimulations();

    QCOMPARE(started.count(), 1);
    QVERIFY(simulator.hasSimulation(1, QModbusDataUnit::Coils, 0));
}

void TestDataSimulator::pauseStopsEmissions()
{
    DataSimulator simulator;

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Toggle;
    params.DataMode = DataType::Binary;
    params.Interval = 20;
    simulator.startSimulation(1, QModbusDataUnit::Coils, 0, params);

    simulator.pauseSimulations();
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);
    QTest::qWait(120);
    QCOMPARE(simulated.count(), 0);

    simulator.resumeSimulations();
    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 1, 2000);
}

void TestDataSimulator::decrementEmitsInitialValue()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Decrement;
    params.DataMode = DataType::UInt16;
    params.Interval = 60000;
    params.DecrementParams.Range = QRange<double>(5., 100.);
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

    QCOMPARE(simulated.count(), 1);
    QCOMPARE(simulated.takeFirst().at(5).toDouble(), 100.);
}

void TestDataSimulator::nonSteppingModesDoNotEmitInitialValue()
{
    for (const auto mode : { SimulationMode::Random, SimulationMode::Toggle, SimulationMode::Off }) {
        DataSimulator simulator;
        QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

        ModbusSimulationParams params;
        params.Mode = mode;
        params.DataMode = DataType::UInt16;
        params.Interval = 60000;
        simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 0, params);

        QCOMPARE(simulated.count(), 0);
    }
}

void TestDataSimulator::disabledSecondaryAddressIsReportedInSimulationMap()
{
    DataSimulator simulator;

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Increment;
    params.DataMode = DataType::Int32;
    params.Interval = 60000;
    simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, 10, params);

    QVERIFY(simulator.hasSimulation(1, QModbusDataUnit::HoldingRegisters, 10));
    QVERIFY(simulator.hasSimulation(1, QModbusDataUnit::HoldingRegisters, 11));
    QCOMPARE(simulator.simulationParams(1, QModbusDataUnit::HoldingRegisters, 11).Mode, SimulationMode::Disabled);
    QCOMPARE(simulator.simulationMap().value({1, QModbusDataUnit::HoldingRegisters, 11}).Mode, SimulationMode::Disabled);
}

void TestDataSimulator::randomUnknownRegisterTypeDoesNotEmitValue()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    params.DataMode = DataType::UInt16;
    params.Interval = 20;
    simulator.startSimulation(1, static_cast<QModbusDataUnit::RegisterType>(999), 0, params);

    QTest::qWait(80);
    QCOMPARE(simulated.count(), 0);
}

void TestDataSimulator::incrementWrapsEveryDataTypeToRangeStart()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    quint16 address = 0;
    for (const auto type : {DataType::Binary, DataType::UInt16, DataType::Int16, DataType::Hex, DataType::Ansi,
                            DataType::Int32, DataType::UInt32, DataType::Float32, DataType::Float64, DataType::Int64,
                            DataType::UInt64}) {
        ModbusSimulationParams params;
        params.Mode = SimulationMode::Increment;
        params.DataMode = type;
        params.Interval = 20;
        params.IncrementParams.Step = 1.;
        params.IncrementParams.Range = QRange<double>(0., 0.);
        simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, address, params);
        address = static_cast<quint16>(address + registersCount(type) + 1);
    }

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 22, 2000);
}

void TestDataSimulator::decrementWrapsEveryDataTypeToRangeEnd()
{
    DataSimulator simulator;
    QSignalSpy simulated(&simulator, &DataSimulator::dataSimulated);

    quint16 address = 0;
    for (const auto type : {DataType::Binary, DataType::UInt16, DataType::Int16, DataType::Hex, DataType::Ansi,
                            DataType::Int32, DataType::UInt32, DataType::Float32, DataType::Float64, DataType::Int64,
                            DataType::UInt64}) {
        ModbusSimulationParams params;
        params.Mode = SimulationMode::Decrement;
        params.DataMode = type;
        params.Interval = 20;
        params.DecrementParams.Step = 1.;
        params.DecrementParams.Range = QRange<double>(0., 0.);
        simulator.startSimulation(1, QModbusDataUnit::HoldingRegisters, address, params);
        address = static_cast<quint16>(address + registersCount(type) + 1);
    }

    QTRY_VERIFY_WITH_TIMEOUT(simulated.count() >= 22, 2000);
}

QTEST_GUILESS_MAIN(TestDataSimulator)
#include "test_datasimulator.moc"
