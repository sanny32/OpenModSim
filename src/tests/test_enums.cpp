// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_enums.cpp
/// \brief Unit tests for the enum<->string helpers and register helpers in enums.h.
///

#include <QTest>

#include "enums.h"

class TestEnums : public QObject
{
    Q_OBJECT

private slots:
    void toStringKnownValues();
    void fromStringIsCaseInsensitive();
    void fromStringNumericFallback();
    void fromStringUnknownReturnsDefault();
    void roundTrip();
    void allEnumMappingsRoundTrip();
    void unknownEnumValuesUseNumericText();
    void registersCountPerType();
    void multiRegisterClassification();
    void boolConversions();
};

void TestEnums::toStringKnownValues()
{
    QCOMPARE(enumToString(DataType::Float32), QStringLiteral("Float32"));
    QCOMPARE(enumToString(ByteOrder::Swapped), QStringLiteral("Swapped"));
    QCOMPARE(enumToString(RegisterOrder::LSRF), QStringLiteral("LSRF"));
    QCOMPARE(enumToString(AddressBase::Base1), QStringLiteral("Base1"));
}

void TestEnums::fromStringIsCaseInsensitive()
{
    QCOMPARE(enumFromString<DataType>(QStringLiteral("float32")), DataType::Float32);
    QCOMPARE(enumFromString<DataType>(QStringLiteral("UINT16")), DataType::UInt16);
}

void TestEnums::fromStringNumericFallback()
{
    QCOMPARE(enumFromString<DataType>(QStringLiteral("0")), DataType::Binary);
    QCOMPARE(enumFromString<RegisterOrder>(QStringLiteral("1")), RegisterOrder::LSRF);
}

void TestEnums::fromStringUnknownReturnsDefault()
{
    QCOMPARE(enumFromString<DataType>(QStringLiteral("nonsense"), DataType::Hex), DataType::Hex);
    QCOMPARE(enumFromString<ByteOrder>(QStringLiteral("")), ByteOrder::Direct);
}

void TestEnums::roundTrip()
{
    for (const DataType t : {DataType::Binary, DataType::UInt16, DataType::Float64, DataType::Ansi})
        QCOMPARE(enumFromString<DataType>(enumToString(t)), t);
}

void TestEnums::allEnumMappingsRoundTrip()
{
    for(const auto value : {AddressBase::Base0, AddressBase::Base1})
        QCOMPARE(enumFromString<AddressBase>(enumToString(value)), value);

    for(const auto value : {AddressSpace::Addr6Digits, AddressSpace::Addr5Digits})
        QCOMPARE(enumFromString<AddressSpace>(enumToString(value)), value);

    for(const auto value : {DataType::Binary, DataType::UInt16, DataType::Int16, DataType::Hex, DataType::Float32,
                            DataType::Float64, DataType::Int32, DataType::UInt32, DataType::Int64, DataType::UInt64,
                            DataType::Ansi})
        QCOMPARE(enumFromString<DataType>(enumToString(value)), value);

    for(const auto value : {RegisterOrder::MSRF, RegisterOrder::LSRF})
        QCOMPARE(enumFromString<RegisterOrder>(enumToString(value)), value);

    for(const auto value : {ByteOrder::Direct, ByteOrder::Swapped})
        QCOMPARE(enumFromString<ByteOrder>(enumToString(value)), value);

    for(const auto value : {ConnectionType::Tcp, ConnectionType::Serial, ConnectionType::RtuTcp})
        QCOMPARE(enumFromString<ConnectionType>(enumToString(value)), value);

    for(const auto value : {TransmissionMode::ASCII, TransmissionMode::RTU})
        QCOMPARE(enumFromString<TransmissionMode>(enumToString(value)), value);

    for(const auto value : {SimulationMode::Disabled, SimulationMode::Off, SimulationMode::Random,
                            SimulationMode::Increment, SimulationMode::Decrement, SimulationMode::Toggle})
        QCOMPARE(enumFromString<SimulationMode>(enumToString(value)), value);

    for(const auto value : {RunMode::Once, RunMode::Periodically})
        QCOMPARE(enumFromString<RunMode>(enumToString(value)), value);

    for(const auto value : {LogViewState::Unknown, LogViewState::Running, LogViewState::Paused})
        QCOMPARE(enumFromString<LogViewState>(enumToString(value)), value);
}

void TestEnums::unknownEnumValuesUseNumericText()
{
    QCOMPARE(enumToString(static_cast<DataType>(123)), QStringLiteral("123"));
    QCOMPARE(enumToString(static_cast<SimulationMode>(77)), QStringLiteral("77"));
    QCOMPARE(enumFromString<DataType>(QStringLiteral("123")), static_cast<DataType>(123));
}

void TestEnums::registersCountPerType()
{
    QCOMPARE(registersCount(DataType::UInt16), 1);
    QCOMPARE(registersCount(DataType::Int16), 1);
    QCOMPARE(registersCount(DataType::Float32), 2);
    QCOMPARE(registersCount(DataType::UInt32), 2);
    QCOMPARE(registersCount(DataType::Float64), 4);
    QCOMPARE(registersCount(DataType::Int64), 4);
}

void TestEnums::multiRegisterClassification()
{
    QVERIFY(isMultiRegisterType(DataType::Int32));
    QVERIFY(isMultiRegisterType(DataType::Float64));
    QVERIFY(!isMultiRegisterType(DataType::UInt16));
    QVERIFY(!isMultiRegisterType(DataType::Ansi));
}

void TestEnums::boolConversions()
{
    QCOMPARE(boolToString(true), QStringLiteral("true"));
    QCOMPARE(boolToString(false), QStringLiteral("false"));
    QVERIFY(stringToBool(QStringLiteral("TRUE")));
    QVERIFY(stringToBool(QStringLiteral("1")));
    QVERIFY(stringToBool(QStringLiteral("yes")));
    QVERIFY(stringToBool(QStringLiteral("On")));
    QVERIFY(!stringToBool(QStringLiteral("false")));
    QVERIFY(!stringToBool(QStringLiteral("anything")));
}

QTEST_GUILESS_MAIN(TestEnums)
#include "test_enums.moc"
