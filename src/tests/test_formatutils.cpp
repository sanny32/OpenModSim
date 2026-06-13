// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_formatutils.cpp
/// \brief Unit tests for the value formatting helpers in formatutils.h.
///

#include <QTest>

#include "formatutils.h"

class TestFormatUtils : public QObject
{
    Q_OBJECT

private slots:
    void uint8ValueDecimalAndHex();
    void uint8ArrayJoinsValues();
    void uint16ArrayDecimalAndHex();
    void wrapValueBrackets();
    void uint16RegisterValueRespectsType();
    void addressFormatting();
};

void TestFormatUtils::uint8ValueDecimalAndHex()
{
    QCOMPARE(formatUInt8Value(DataType::UInt16, true, 5), QStringLiteral("005"));
    QCOMPARE(formatUInt8Value(DataType::UInt16, false, 5), QStringLiteral("  5"));
    QCOMPARE(formatUInt8Value(DataType::Hex, true, 0xAB), QStringLiteral("0xAB"));
}

void TestFormatUtils::uint8ArrayJoinsValues()
{
    const QByteArray data = QByteArray::fromHex("01AB");
    QCOMPARE(formatUInt8Array(DataType::Hex, true, data), QStringLiteral("01 AB"));
    QCOMPARE(formatUInt8Array(DataType::UInt16, true, data), QStringLiteral("001 171"));
}

void TestFormatUtils::uint16ArrayDecimalAndHex()
{
    const QByteArray data = QByteArray::fromHex("12340001");
    QCOMPARE(formatUInt16Array(DataType::Hex, true, data, ByteOrder::Direct), QStringLiteral("0x1234 0x0001"));
    QCOMPARE(formatUInt16Array(DataType::UInt16, true, data, ByteOrder::Direct), QStringLiteral("04660 00001"));
}

void TestFormatUtils::wrapValueBrackets()
{
    QCOMPARE(wrapValue(QStringLiteral("42"), true), QStringLiteral("<42>"));
    QCOMPARE(wrapValue(QStringLiteral("42"), false), QStringLiteral("42"));
}

void TestFormatUtils::uint16RegisterValueRespectsType()
{
    QVariant out;
    const QString coil = formatUInt16Value(QModbusDataUnit::Coils, 1, ByteOrder::Direct, true, out, false);
    QCOMPARE(coil, QStringLiteral("1"));
    QCOMPARE(out.toUInt(), 1u);

    const QString reg = formatUInt16Value(QModbusDataUnit::HoldingRegisters, 42, ByteOrder::Direct, true, out, false);
    QCOMPARE(reg, QStringLiteral("00042"));
}

void TestFormatUtils::addressFormatting()
{
    QCOMPARE(formatAddress(QModbusDataUnit::Coils, 5, AddressSpace::Addr6Digits, false), QStringLiteral("000005"));
    QCOMPARE(formatAddress(QModbusDataUnit::HoldingRegisters, 1, AddressSpace::Addr5Digits, false), QStringLiteral("40001"));
    QCOMPARE(formatAddress(QModbusDataUnit::InputRegisters, 0x1F, AddressSpace::Addr6Digits, true), QStringLiteral("0x001F"));
}

QTEST_GUILESS_MAIN(TestFormatUtils)
#include "test_formatutils.moc"
