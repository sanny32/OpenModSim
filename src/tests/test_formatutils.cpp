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
    void int16AndBinaryFormatters();
    void multiRegisterFormatters();
    void formatterCoilAndDiscreteInputBranches();
    void formatterDeferredRegisterBranches();
    void formatterDefaultRegisterTypeBranches();
    void scalarAndArrayFormatterBranches();
    void addressFormatterCoversAllRegisterTypes();
    void ansiFormatter();
    void coilPassthrough();
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

void TestFormatUtils::int16AndBinaryFormatters()
{
    QVariant out;
    const QString neg = formatInt16Value(QModbusDataUnit::HoldingRegisters, qint16(-5), ByteOrder::Direct, out, false);
    QCOMPARE(out.toInt(), -5);
    QVERIFY(neg.trimmed() == QStringLiteral("-5"));

    const QString bin = formatBinaryValue(QModbusDataUnit::HoldingRegisters, 0x000F, ByteOrder::Direct, out, false);
    QCOMPARE(out.toUInt(), 0x000Fu);
    QVERIFY(bin.contains(QStringLiteral("1111")));
}

void TestFormatUtils::multiRegisterFormatters()
{
    QVariant out;

    quint16 fl = 0, fh = 0;
    breakFloat(3.5f, fl, fh, ByteOrder::Direct);
    formatFloatValue(QModbusDataUnit::HoldingRegisters, fl, fh, ByteOrder::Direct, false, out, false);
    QCOMPARE(out.toFloat(), 3.5f);

    quint16 il = 0, ih = 0;
    breakInt32(-123456, il, ih, ByteOrder::Direct);
    formatInt32Value(QModbusDataUnit::HoldingRegisters, il, ih, ByteOrder::Direct, false, out, false);
    QCOMPARE(out.toInt(), -123456);

    quint16 ul = 0, uh = 0;
    breakUInt32(0x80000001u, ul, uh, ByteOrder::Direct);
    formatUInt32Value(QModbusDataUnit::HoldingRegisters, ul, uh, ByteOrder::Direct, false, false, out, false);
    QCOMPARE(out.toUInt(), 0x80000001u);

    quint16 d[4] = {0, 0, 0, 0};
    breakDouble(2.5, d[0], d[1], d[2], d[3], ByteOrder::Direct);
    formatDoubleValue(QModbusDataUnit::HoldingRegisters, d[0], d[1], d[2], d[3], ByteOrder::Direct, false, out, false);
    QCOMPARE(out.toDouble(), 2.5);

    quint16 q[4] = {0, 0, 0, 0};
    breakInt64(Q_INT64_C(-987654321), q[0], q[1], q[2], q[3], ByteOrder::Direct);
    formatInt64Value(QModbusDataUnit::HoldingRegisters, q[0], q[1], q[2], q[3], ByteOrder::Direct, false, out, false);
    QCOMPARE(out.toLongLong(), Q_INT64_C(-987654321));

    quint16 u[4] = {0, 0, 0, 0};
    breakUInt64(Q_UINT64_C(0x8000000000000001), u[0], u[1], u[2], u[3], ByteOrder::Direct);
    formatUInt64Value(QModbusDataUnit::HoldingRegisters, u[0], u[1], u[2], u[3], ByteOrder::Direct, false, false, out, false);
    QCOMPARE(out.toULongLong(), Q_UINT64_C(0x8000000000000001));
}

void TestFormatUtils::formatterCoilAndDiscreteInputBranches()
{
    QVariant out;

    QCOMPARE(formatBinaryValue(QModbusDataUnit::DiscreteInputs, 1, ByteOrder::Direct, out), QStringLiteral("<1>"));
    QCOMPARE(out.toUInt(), 1u);

    QCOMPARE(formatUInt16Value(QModbusDataUnit::DiscreteInputs, 2, ByteOrder::Direct, true, out), QStringLiteral("<2>"));
    QCOMPARE(out.toUInt(), 2u);

    QCOMPARE(formatInt16Value(QModbusDataUnit::DiscreteInputs, -3, ByteOrder::Direct, out), QStringLiteral("<-3>"));
    QCOMPARE(out.toInt(), -3);

    QCOMPARE(formatHexValue(QModbusDataUnit::DiscreteInputs, 4, ByteOrder::Direct, out), QStringLiteral("<4>"));
    QCOMPARE(out.toUInt(), 4u);

    QCOMPARE(formatAnsiValue(QModbusDataUnit::DiscreteInputs, 5, ByteOrder::Direct, QStringLiteral("UTF-8"), out),
             QStringLiteral("<5>"));
    QCOMPARE(out.toUInt(), 5u);

    QCOMPARE(formatDoubleValue(QModbusDataUnit::DiscreteInputs, 6, 0, 0, 0, ByteOrder::Direct, false, out),
             QStringLiteral("<6>"));
    QCOMPARE(out.toUInt(), 6u);

    QCOMPARE(formatInt64Value(QModbusDataUnit::DiscreteInputs, 7, 0, 0, 0, ByteOrder::Direct, false, out),
             QStringLiteral("<7>"));
    QCOMPARE(out.toUInt(), 7u);

    QCOMPARE(formatUInt64Value(QModbusDataUnit::DiscreteInputs, 8, 0, 0, 0, ByteOrder::Direct, true, false, out),
             QStringLiteral("<8>"));
    QCOMPARE(out.toUInt(), 8u);
}

void TestFormatUtils::formatterDeferredRegisterBranches()
{
    QVariant out = 42;

    QVERIFY(formatFloatValue(QModbusDataUnit::InputRegisters, 1, 2, ByteOrder::Direct, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);

    QVERIFY(formatInt32Value(QModbusDataUnit::InputRegisters, 1, 2, ByteOrder::Direct, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);

    QVERIFY(formatUInt32Value(QModbusDataUnit::InputRegisters, 1, 2, ByteOrder::Direct, true, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);

    QVERIFY(formatDoubleValue(QModbusDataUnit::InputRegisters, 1, 2, 3, 4, ByteOrder::Direct, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);

    QVERIFY(formatInt64Value(QModbusDataUnit::InputRegisters, 1, 2, 3, 4, ByteOrder::Direct, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);

    QVERIFY(formatUInt64Value(QModbusDataUnit::InputRegisters, 1, 2, 3, 4, ByteOrder::Direct, true, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);
}

void TestFormatUtils::formatterDefaultRegisterTypeBranches()
{
    const auto invalidType = static_cast<QModbusDataUnit::RegisterType>(999);
    QVariant out = 123;

    QVERIFY(formatBinaryValue(invalidType, 1, ByteOrder::Direct, out).isEmpty());
    QCOMPARE(out.toUInt(), 1u);

    QVERIFY(formatUInt16Value(invalidType, 2, ByteOrder::Direct, true, out).isEmpty());
    QCOMPARE(out.toUInt(), 2u);

    QVERIFY(formatInt16Value(invalidType, -3, ByteOrder::Direct, out).isEmpty());
    QCOMPARE(out.toInt(), -3);

    QVERIFY(formatHexValue(invalidType, 4, ByteOrder::Direct, out).isEmpty());
    QCOMPARE(out.toUInt(), 4u);

    QVERIFY(formatAnsiValue(invalidType, 5, ByteOrder::Direct, QStringLiteral("UTF-8"), out).isEmpty());
    QCOMPARE(out.toUInt(), 5u);

    out = 123;
    QVERIFY(formatFloatValue(invalidType, 6, 0, ByteOrder::Direct, false, out).isEmpty());
    QCOMPARE(out.toInt(), 123);

    QVERIFY(formatInt32Value(invalidType, 7, 0, ByteOrder::Direct, false, out).isEmpty());
    QCOMPARE(out.toInt(), 123);

    QVERIFY(formatUInt32Value(invalidType, 8, 0, ByteOrder::Direct, true, false, out).isEmpty());
    QCOMPARE(out.toInt(), 123);

    QVERIFY(formatDoubleValue(invalidType, 9, 0, 0, 0, ByteOrder::Direct, false, out).isEmpty());
    QCOMPARE(out.toInt(), 123);

    QVERIFY(formatInt64Value(invalidType, 10, 0, 0, 0, ByteOrder::Direct, false, out).isEmpty());
    QCOMPARE(out.toInt(), 123);

    QVERIFY(formatUInt64Value(invalidType, 11, 0, 0, 0, ByteOrder::Direct, true, false, out).isEmpty());
    QCOMPARE(out.toInt(), 123);
}

void TestFormatUtils::scalarAndArrayFormatterBranches()
{
    const QByteArray bytes = QByteArray::fromHex("01AB");

    QCOMPARE(formatUInt8Value(DataType::Int16, true, 5), QStringLiteral("005"));
    QCOMPARE(formatUInt8Value(DataType::Ansi, false, 0x0C), QStringLiteral("0x0C"));

    QCOMPARE(formatUInt8Array(DataType::Int16, false, bytes), QStringLiteral("  1 171"));
    QCOMPARE(formatUInt8Array(DataType::Ansi, false, bytes), QStringLiteral("01 AB"));

    QCOMPARE(formatUInt16Array(DataType::Int16, false, bytes, ByteOrder::Direct), QStringLiteral("  427"));
    QCOMPARE(formatUInt16Array(DataType::Hex, true, bytes, ByteOrder::Swapped), QStringLiteral("0xAB01"));

    QCOMPARE(formatUInt16Value(DataType::Int16, false, 12), QStringLiteral("   12"));
    QCOMPARE(formatUInt16Value(DataType::Binary, false, 0x00EF), QStringLiteral("0x00EF"));
}

void TestFormatUtils::addressFormatterCoversAllRegisterTypes()
{
    QCOMPARE(formatAddress(QModbusDataUnit::DiscreteInputs, 5, AddressSpace::Addr5Digits, false), QStringLiteral("10005"));
    QCOMPARE(formatAddress(QModbusDataUnit::InputRegisters, 5, AddressSpace::Addr5Digits, false), QStringLiteral("30005"));
    QCOMPARE(formatAddress(static_cast<QModbusDataUnit::RegisterType>(999), 5, AddressSpace::Addr5Digits, false),
             QStringLiteral(" 0005"));
    QCOMPARE(formatAddress(QModbusDataUnit::Coils, 5, AddressSpace::Addr5Digits, true, AddressBase::Base1),
             QStringLiteral("0x0005"));
}

void TestFormatUtils::ansiFormatter()
{
    QVariant out;
    const QString text = formatAnsiValue(QModbusDataUnit::HoldingRegisters, 0x4142, ByteOrder::Direct,
                                         QStringLiteral("UTF-8"), out, false);
    QVERIFY(!text.isEmpty());
    QCOMPARE(out.toUInt(), 0x4142u);
}

void TestFormatUtils::coilPassthrough()
{
    QVariant out;
    QCOMPARE(formatFloatValue(QModbusDataUnit::Coils, 1, 0, ByteOrder::Direct, false, out, false), QStringLiteral("1"));
    QCOMPARE(out.toUInt(), 1u);
    QCOMPARE(formatInt32Value(QModbusDataUnit::Coils, 0, 0, ByteOrder::Direct, false, out, false), QStringLiteral("0"));
}

QTEST_GUILESS_MAIN(TestFormatUtils)
#include "test_formatutils.moc"
