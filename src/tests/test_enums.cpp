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
