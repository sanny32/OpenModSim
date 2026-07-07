// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_simulationparams.cpp
/// \brief Unit tests for XML serialization of ModbusSimulationParams.
///

#include <limits>

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
    void defaultsSurviveWrongXmlElements();
    void invalidAttributesKeepDefaults();
    void missingAttributesKeepExistingValues();
    void mismatchedChildParamsAreSkipped();
    void matchingChildParamsForEveryMode();
    void writerSkipsUnsupportedChildParams();
    void invalidRangeFallsBackToDefaultRange();
    void rangeContainsBoundaries();
    void rangeIgnoresWrongReaderState();
    void childParamsIgnoreWrongReaderStateAndRoot();
    void childParamsKeepExistingValuesWhenChildRangeIsMissing();
    void numericAttributesAreAccepted();
    void negativeIntervalIsIgnored();
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

void TestSimulationParams::defaultsSurviveWrongXmlElements()
{
    ModbusSimulationParams params;
    params.Mode = SimulationMode::Toggle;
    params.Interval = 42;

    QXmlStreamReader reader(QByteArrayLiteral("<NotSimulationParams Mode=\"Random\" Interval=\"100\"/>"));
    reader.readNextStartElement();
    reader >> params;

    QCOMPARE(params.Mode, SimulationMode::Toggle);
    QCOMPARE(params.Interval, 42u);

    IncrementSimulationParams increment;
    increment.Step = 7.;
    QXmlStreamReader incrementReader(QByteArrayLiteral("<NotIncrementSimulationParams Step=\"9\"/>"));
    incrementReader.readNextStartElement();
    incrementReader >> increment;
    QCOMPARE(increment.Step, 7.);
}

void TestSimulationParams::invalidAttributesKeepDefaults()
{
    ModbusSimulationParams params;
    params.Interval = 123;

    QXmlStreamReader reader(QByteArrayLiteral(
        "<ModbusSimulationParams Mode=\"not-a-mode\" Interval=\"not-a-number\" DataType=\"not-a-type\" "
        "RegisterOrder=\"not-an-order\"/>"));
    reader.readNextStartElement();
    reader >> params;

    QCOMPARE(params.Mode, SimulationMode::Off);
    QCOMPARE(params.Interval, 123u);
    QCOMPARE(params.DataMode, DataType::Hex);
    QCOMPARE(params.RegOrder, RegisterOrder::MSRF);

    IncrementSimulationParams increment;
    increment.Step = 5.;
    QXmlStreamReader incrementReader(QByteArrayLiteral("<IncrementSimulationParams Step=\"not-a-number\"/>"));
    incrementReader.readNextStartElement();
    incrementReader >> increment;
    QCOMPARE(increment.Step, 5.);

    DecrementSimulationParams decrement;
    decrement.Step = 6.;
    QXmlStreamReader decrementReader(QByteArrayLiteral("<DecrementSimulationParams Step=\"not-a-number\"/>"));
    decrementReader.readNextStartElement();
    decrementReader >> decrement;
    QCOMPARE(decrement.Step, 6.);
}

void TestSimulationParams::missingAttributesKeepExistingValues()
{
    ModbusSimulationParams params;
    params.Mode = SimulationMode::Toggle;
    params.Interval = 321;
    params.DataMode = DataType::Float64;
    params.RegOrder = RegisterOrder::LSRF;

    QXmlStreamReader reader(QByteArrayLiteral("<ModbusSimulationParams Interval=\"654\"/>"));
    reader.readNextStartElement();
    reader >> params;

    QCOMPARE(params.Mode, SimulationMode::Toggle);
    QCOMPARE(params.Interval, 654u);
    QCOMPARE(params.DataMode, DataType::Float64);
    QCOMPARE(params.RegOrder, RegisterOrder::LSRF);
}

void TestSimulationParams::mismatchedChildParamsAreSkipped()
{
    QXmlStreamReader reader(QByteArrayLiteral(
        "<ModbusSimulationParams Mode=\"Random\">"
        "<IncrementSimulationParams Step=\"9\"><Range From=\"1\" To=\"2\"/></IncrementSimulationParams>"
        "<UnknownChild/>"
        "</ModbusSimulationParams>"));
    reader.readNextStartElement();

    ModbusSimulationParams params;
    reader >> params;

    QCOMPARE(params.Mode, SimulationMode::Random);
    QCOMPARE(params.IncrementParams.Step, 1.);
    QCOMPARE(params.RandomParams.Range.from(), 0.);
    QCOMPARE(params.RandomParams.Range.to(), 65535.);
}

void TestSimulationParams::matchingChildParamsForEveryMode()
{
    QXmlStreamReader randomReader(QByteArrayLiteral(
        "<ModbusSimulationParams Mode=\"Random\"><RandomSimulationParams><Range From=\"11\" To=\"22\"/></RandomSimulationParams></ModbusSimulationParams>"));
    randomReader.readNextStartElement();
    ModbusSimulationParams randomParams;
    randomReader >> randomParams;
    QCOMPARE(randomParams.RandomParams.Range.from(), 11.);
    QCOMPARE(randomParams.RandomParams.Range.to(), 22.);

    QXmlStreamReader incrementReader(QByteArrayLiteral(
        "<ModbusSimulationParams Mode=\"Increment\"><IncrementSimulationParams Step=\"3\"><Range From=\"4\" To=\"5\"/></IncrementSimulationParams></ModbusSimulationParams>"));
    incrementReader.readNextStartElement();
    ModbusSimulationParams incrementParams;
    incrementReader >> incrementParams;
    QCOMPARE(incrementParams.IncrementParams.Step, 3.);
    QCOMPARE(incrementParams.IncrementParams.Range.from(), 4.);
    QCOMPARE(incrementParams.IncrementParams.Range.to(), 5.);

    QXmlStreamReader decrementReader(QByteArrayLiteral(
        "<ModbusSimulationParams Mode=\"Decrement\"><DecrementSimulationParams Step=\"6\"><Range From=\"7\" To=\"8\"/></DecrementSimulationParams></ModbusSimulationParams>"));
    decrementReader.readNextStartElement();
    ModbusSimulationParams decrementParams;
    decrementReader >> decrementParams;
    QCOMPARE(decrementParams.DecrementParams.Step, 6.);
    QCOMPARE(decrementParams.DecrementParams.Range.from(), 7.);
    QCOMPARE(decrementParams.DecrementParams.Range.to(), 8.);
}

void TestSimulationParams::writerSkipsUnsupportedChildParams()
{
    ModbusSimulationParams offParams;
    offParams.Mode = SimulationMode::Off;
    offParams.DataMode = DataType::UInt16;

    QByteArray offXml;
    {
        QXmlStreamWriter writer(&offXml);
        writer << offParams;
    }

    QVERIFY(!offXml.contains("RandomSimulationParams"));
    QVERIFY(!offXml.contains("IncrementSimulationParams"));
    QVERIFY(!offXml.contains("DecrementSimulationParams"));
    QVERIFY(!offXml.contains("RegisterOrder"));

    ModbusSimulationParams toggleParams;
    toggleParams.Mode = SimulationMode::Toggle;
    toggleParams.DataMode = DataType::Int32;

    QByteArray toggleXml;
    {
        QXmlStreamWriter writer(&toggleXml);
        writer << toggleParams;
    }

    QVERIFY(toggleXml.contains("RegisterOrder"));
    QVERIFY(!toggleXml.contains("RandomSimulationParams"));
}

void TestSimulationParams::invalidRangeFallsBackToDefaultRange()
{
    QRange<double> range(1., 2.);
    QXmlStreamReader reader(QByteArrayLiteral("<Range From=\"bad\" To=\"9\"/>"));
    reader.readNextStartElement();
    reader >> range;

    QCOMPARE(range.from(), std::numeric_limits<double>::min());
    QCOMPARE(range.to(), std::numeric_limits<double>::max());
}

void TestSimulationParams::rangeContainsBoundaries()
{
    const QRange<int> range(10, 20);

    QVERIFY(!range.contains(9));
    QVERIFY(range.contains(10));
    QVERIFY(range.contains(15));
    QVERIFY(range.contains(20));
    QVERIFY(!range.contains(21));
}

void TestSimulationParams::rangeIgnoresWrongReaderState()
{
    QRange<double> range(1., 2.);
    QXmlStreamReader textReader(QByteArrayLiteral("<Range From=\"3\" To=\"4\"/>"));
    textReader.readNext();
    textReader >> range;
    QCOMPARE(range.from(), 1.);
    QCOMPARE(range.to(), 2.);

    QXmlStreamReader wrongElementReader(QByteArrayLiteral("<NotRange From=\"3\" To=\"4\"/>"));
    wrongElementReader.readNextStartElement();
    wrongElementReader >> range;
    QCOMPARE(range.from(), 1.);
    QCOMPARE(range.to(), 2.);
}

void TestSimulationParams::childParamsIgnoreWrongReaderStateAndRoot()
{
    RandomSimulationParams random;
    random.Range = QRange<double>(1., 2.);
    QXmlStreamReader randomTextReader(QByteArrayLiteral("<RandomSimulationParams><Range From=\"3\" To=\"4\"/></RandomSimulationParams>"));
    randomTextReader.readNext();
    randomTextReader >> random;
    QCOMPARE(random.Range.from(), 1.);
    QCOMPARE(random.Range.to(), 2.);

    IncrementSimulationParams increment;
    increment.Step = 5.;
    QXmlStreamReader incrementWrongRoot(QByteArrayLiteral("<NotIncrementSimulationParams Step=\"7\"><Range From=\"8\" To=\"9\"/></NotIncrementSimulationParams>"));
    incrementWrongRoot.readNextStartElement();
    incrementWrongRoot >> increment;
    QCOMPARE(increment.Step, 5.);

    DecrementSimulationParams decrement;
    decrement.Step = 6.;
    QXmlStreamReader decrementTextReader(QByteArrayLiteral("<DecrementSimulationParams Step=\"7\"><Range From=\"8\" To=\"9\"/></DecrementSimulationParams>"));
    decrementTextReader.readNext();
    decrementTextReader >> decrement;
    QCOMPARE(decrement.Step, 6.);
}

void TestSimulationParams::childParamsKeepExistingValuesWhenChildRangeIsMissing()
{
    RandomSimulationParams random;
    random.Range = QRange<double>(1., 2.);
    QXmlStreamReader randomReader(QByteArrayLiteral("<RandomSimulationParams/>"));
    randomReader.readNextStartElement();
    randomReader >> random;
    QCOMPARE(random.Range.from(), 1.);
    QCOMPARE(random.Range.to(), 2.);

    IncrementSimulationParams increment;
    increment.Step = 3.;
    increment.Range = QRange<double>(4., 5.);
    QXmlStreamReader incrementReader(QByteArrayLiteral("<IncrementSimulationParams Step=\"7\"/>"));
    incrementReader.readNextStartElement();
    incrementReader >> increment;
    QCOMPARE(increment.Step, 7.);
    QCOMPARE(increment.Range.from(), 4.);
    QCOMPARE(increment.Range.to(), 5.);

    DecrementSimulationParams decrement;
    decrement.Step = 8.;
    decrement.Range = QRange<double>(9., 10.);
    QXmlStreamReader decrementReader(QByteArrayLiteral("<DecrementSimulationParams Step=\"11\"/>"));
    decrementReader.readNextStartElement();
    decrementReader >> decrement;
    QCOMPARE(decrement.Step, 11.);
    QCOMPARE(decrement.Range.from(), 9.);
    QCOMPARE(decrement.Range.to(), 10.);
}

void TestSimulationParams::numericAttributesAreAccepted()
{
    QXmlStreamReader reader(QByteArrayLiteral(
        "<ModbusSimulationParams Mode=\"3\" Interval=\"25\" DataType=\"8\" RegisterOrder=\"1\">"
        "<IncrementSimulationParams Step=\"4\"><Range From=\"5\" To=\"6\"/></IncrementSimulationParams>"
        "</ModbusSimulationParams>"));
    reader.readNextStartElement();

    ModbusSimulationParams params;
    reader >> params;

    QCOMPARE(params.Mode, SimulationMode::Increment);
    QCOMPARE(params.Interval, 25u);
    QCOMPARE(params.DataMode, DataType::Int64);
    QCOMPARE(params.RegOrder, RegisterOrder::LSRF);
    QCOMPARE(params.IncrementParams.Step, 4.);
    QCOMPARE(params.IncrementParams.Range.from(), 5.);
}

void TestSimulationParams::negativeIntervalIsIgnored()
{
    QXmlStreamReader reader(QByteArrayLiteral("<ModbusSimulationParams Interval=\"-1\"/>"));
    reader.readNextStartElement();

    ModbusSimulationParams params;
    params.Interval = 44;
    reader >> params;

    QCOMPARE(params.Interval, 44u);
}

QTEST_GUILESS_MAIN(TestSimulationParams)
#include "test_simulationparams.moc"
