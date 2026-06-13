// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_datasimulator.cpp
/// \brief Unit tests for DataSimulator registration, lookup and emitted updates.
///

#include <QSignalSpy>
#include <QTest>

#include "datasimulator.h"

class TestDataSimulator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void startRegistersSimulation();
    void multiRegisterReservesFollowingAddresses();
    void stopSimulationRemovesEntry();
    void stopSimulationsClearsEverything();
    void canStartSimulationBlocksOverlap();
    void incrementEmitsInitialValue();
    void toggleAlternatesOnTimer();
    void incrementProgressesUpward();
    void decrementProgressesDownward();
    void randomStaysInRange();
    void restartReemitsStarted();
    void pauseStopsEmissions();
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

QTEST_GUILESS_MAIN(TestDataSimulator)
#include "test_datasimulator.moc"
