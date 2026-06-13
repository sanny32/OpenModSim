// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_simulationparams.cpp
/// \brief Unit tests for XML serialization of ModbusSimulationParams.
///

#include <QTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "modbussimulationparams.h"

namespace {

ModbusSimulationParams roundTrip(const ModbusSimulationParams& source)
{
    QByteArray buffer;
    {
        QXmlStreamWriter writer(&buffer);
        writer << source;
    }

    ModbusSimulationParams restored;
    QXmlStreamReader reader(buffer);
    reader.readNextStartElement();
    reader >> restored;
    return restored;
}

}

class TestSimulationParams : public QObject
{
    Q_OBJECT

private slots:
    void randomRoundTrip();
    void incrementRoundTripWithRegisterOrder();
    void decrementRoundTrip();
    void rangeRoundTrip();
};

void TestSimulationParams::randomRoundTrip()
{
    ModbusSimulationParams source;
    source.Mode = SimulationMode::Random;
    source.DataMode = DataType::UInt16;
    source.Interval = 2500;
    source.RandomParams.Range = QRange<double>(3., 77.);

    const auto restored = roundTrip(source);
    QCOMPARE(restored.Mode, SimulationMode::Random);
    QCOMPARE(restored.DataMode, DataType::UInt16);
    QCOMPARE(restored.Interval, 2500u);
    QCOMPARE(restored.RandomParams.Range.from(), 3.);
    QCOMPARE(restored.RandomParams.Range.to(), 77.);
}

void TestSimulationParams::incrementRoundTripWithRegisterOrder()
{
    ModbusSimulationParams source;
    source.Mode = SimulationMode::Increment;
    source.DataMode = DataType::Int32;
    source.RegOrder = RegisterOrder::LSRF;
    source.IncrementParams.Step = 2.5;
    source.IncrementParams.Range = QRange<double>(0., 500.);

    const auto restored = roundTrip(source);
    QCOMPARE(restored.Mode, SimulationMode::Increment);
    QCOMPARE(restored.DataMode, DataType::Int32);
    QCOMPARE(restored.RegOrder, RegisterOrder::LSRF);
    QCOMPARE(restored.IncrementParams.Step, 2.5);
    QCOMPARE(restored.IncrementParams.Range.to(), 500.);
}

void TestSimulationParams::decrementRoundTrip()
{
    ModbusSimulationParams source;
    source.Mode = SimulationMode::Decrement;
    source.DataMode = DataType::Float32;
    source.DecrementParams.Step = 4.;
    source.DecrementParams.Range = QRange<double>(-10., 10.);

    const auto restored = roundTrip(source);
    QCOMPARE(restored.Mode, SimulationMode::Decrement);
    QCOMPARE(restored.DecrementParams.Step, 4.);
    QCOMPARE(restored.DecrementParams.Range.from(), -10.);
}

void TestSimulationParams::rangeRoundTrip()
{
    QByteArray buffer;
    {
        QXmlStreamWriter writer(&buffer);
        writer << QRange<double>(1.5, 9.5);
    }

    QRange<double> restored;
    QXmlStreamReader reader(buffer);
    reader.readNextStartElement();
    reader >> restored;
    QCOMPARE(restored.from(), 1.5);
    QCOMPARE(restored.to(), 9.5);
}

QTEST_GUILESS_MAIN(TestSimulationParams)
#include "test_simulationparams.moc"
