// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_enums.cpp
/// \brief Unit tests for the enum<->string helpers and register helpers in enums.h.
///

#include <QSettings>
#include <QTemporaryDir>
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
    void allEnumNumericFallbacks();
    void allEnumDefaultFallbacks();
    void settingsOperatorsRoundTrip();
    void registersCountPerType();
    void multiRegisterClassification();
    void boolConversions();
    void registerTypeNames();
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
    QCOMPARE(enumToString(static_cast<AddressBase>(12)), QStringLiteral("12"));
    QCOMPARE(enumToString(static_cast<AddressSpace>(13)), QStringLiteral("13"));
    QCOMPARE(enumToString(static_cast<RegisterOrder>(14)), QStringLiteral("14"));
    QCOMPARE(enumToString(static_cast<ByteOrder>(15)), QStringLiteral("15"));
    QCOMPARE(enumToString(static_cast<ConnectionType>(16)), QStringLiteral("16"));
    QCOMPARE(enumToString(static_cast<TransmissionMode>(17)), QStringLiteral("17"));
    QCOMPARE(enumToString(static_cast<SimulationMode>(77)), QStringLiteral("77"));
    QCOMPARE(enumToString(static_cast<RunMode>(18)), QStringLiteral("18"));
    QCOMPARE(enumToString(static_cast<LogViewState>(19)), QStringLiteral("19"));
    QCOMPARE(enumFromString<DataType>(QStringLiteral("123")), static_cast<DataType>(123));
}

void TestEnums::allEnumNumericFallbacks()
{
    QCOMPARE(enumFromString<AddressBase>(QStringLiteral("12")), static_cast<AddressBase>(12));
    QCOMPARE(enumFromString<AddressSpace>(QStringLiteral("13")), static_cast<AddressSpace>(13));
    QCOMPARE(enumFromString<DataType>(QStringLiteral("14")), static_cast<DataType>(14));
    QCOMPARE(enumFromString<RegisterOrder>(QStringLiteral("15")), static_cast<RegisterOrder>(15));
    QCOMPARE(enumFromString<ByteOrder>(QStringLiteral("16")), static_cast<ByteOrder>(16));
    QCOMPARE(enumFromString<ConnectionType>(QStringLiteral("17")), static_cast<ConnectionType>(17));
    QCOMPARE(enumFromString<TransmissionMode>(QStringLiteral("18")), static_cast<TransmissionMode>(18));
    QCOMPARE(enumFromString<SimulationMode>(QStringLiteral("19")), static_cast<SimulationMode>(19));
    QCOMPARE(enumFromString<RunMode>(QStringLiteral("20")), static_cast<RunMode>(20));
    QCOMPARE(enumFromString<LogViewState>(QStringLiteral("21")), static_cast<LogViewState>(21));
}

void TestEnums::allEnumDefaultFallbacks()
{
    QCOMPARE(enumFromString<AddressBase>(QStringLiteral("bad"), AddressBase::Base1), AddressBase::Base1);
    QCOMPARE(enumFromString<AddressSpace>(QStringLiteral("bad"), AddressSpace::Addr5Digits), AddressSpace::Addr5Digits);
    QCOMPARE(enumFromString<DataType>(QStringLiteral("bad"), DataType::Ansi), DataType::Ansi);
    QCOMPARE(enumFromString<RegisterOrder>(QStringLiteral("bad"), RegisterOrder::LSRF), RegisterOrder::LSRF);
    QCOMPARE(enumFromString<ByteOrder>(QStringLiteral("bad"), ByteOrder::Swapped), ByteOrder::Swapped);
    QCOMPARE(enumFromString<ConnectionType>(QStringLiteral("bad"), ConnectionType::RtuTcp), ConnectionType::RtuTcp);
    QCOMPARE(enumFromString<TransmissionMode>(QStringLiteral("bad"), TransmissionMode::RTU), TransmissionMode::RTU);
    QCOMPARE(enumFromString<SimulationMode>(QStringLiteral("bad"), SimulationMode::Toggle), SimulationMode::Toggle);
    QCOMPARE(enumFromString<RunMode>(QStringLiteral("bad"), RunMode::Periodically), RunMode::Periodically);
    QCOMPARE(enumFromString<LogViewState>(QStringLiteral("bad"), LogViewState::Paused), LogViewState::Paused);
}

void TestEnums::settingsOperatorsRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QSettings settings(dir.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat);
    settings << AddressBase::Base1;
    settings << AddressSpace::Addr5Digits;
    settings << DataType::Float64;
    settings << RegisterOrder::LSRF;
    settings << ByteOrder::Swapped;
    settings.sync();

    AddressBase base = AddressBase::Base0;
    AddressSpace space = AddressSpace::Addr6Digits;
    DataType type = DataType::Binary;
    RegisterOrder registerOrder = RegisterOrder::MSRF;
    ByteOrder byteOrder = ByteOrder::Direct;

    settings >> base;
    settings >> space;
    settings >> type;
    settings >> registerOrder;
    settings >> byteOrder;

    QCOMPARE(base, AddressBase::Base1);
    QCOMPARE(space, AddressSpace::Addr5Digits);
    QCOMPARE(type, DataType::Float64);
    QCOMPARE(registerOrder, RegisterOrder::LSRF);
    QCOMPARE(byteOrder, ByteOrder::Swapped);
}

void TestEnums::registersCountPerType()
{
    QCOMPARE(registersCount(DataType::Binary), 1);
    QCOMPARE(registersCount(DataType::UInt16), 1);
    QCOMPARE(registersCount(DataType::Int16), 1);
    QCOMPARE(registersCount(DataType::Hex), 1);
    QCOMPARE(registersCount(DataType::Ansi), 1);
    QCOMPARE(registersCount(DataType::Float32), 2);
    QCOMPARE(registersCount(DataType::Int32), 2);
    QCOMPARE(registersCount(DataType::UInt32), 2);
    QCOMPARE(registersCount(DataType::Float64), 4);
    QCOMPARE(registersCount(DataType::Int64), 4);
    QCOMPARE(registersCount(DataType::UInt64), 4);
}

void TestEnums::multiRegisterClassification()
{
    QVERIFY(!isMultiRegisterType(DataType::Binary));
    QVERIFY(!isMultiRegisterType(DataType::Int16));
    QVERIFY(!isMultiRegisterType(DataType::Hex));
    QVERIFY(isMultiRegisterType(DataType::Float32));
    QVERIFY(isMultiRegisterType(DataType::Int32));
    QVERIFY(isMultiRegisterType(DataType::UInt32));
    QVERIFY(isMultiRegisterType(DataType::Float64));
    QVERIFY(isMultiRegisterType(DataType::Int64));
    QVERIFY(isMultiRegisterType(DataType::UInt64));
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

void TestEnums::registerTypeNames()
{
    QVERIFY(!registerTypeName(QModbusDataUnit::Coils).isEmpty());
    QVERIFY(!registerTypeName(QModbusDataUnit::DiscreteInputs).isEmpty());
    QVERIFY(!registerTypeName(QModbusDataUnit::InputRegisters).isEmpty());
    QVERIFY(!registerTypeName(QModbusDataUnit::HoldingRegisters).isEmpty());
    QCOMPARE(registerTypeName(static_cast<QModbusDataUnit::RegisterType>(99)), QStringLiteral("99"));
}

QTEST_GUILESS_MAIN(TestEnums)
#include "test_enums.moc"
