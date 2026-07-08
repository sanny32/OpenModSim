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
    void inputRegisterFormatterBranches();
    void unsignedFormatterLeadingZeroBranches();
    void ansiFormatter();
    void coilPassthrough();
    void emptyArrays();
    void bracketedRegisterFormatters();
    void holdingRegisterDeferredBranches();
    void remainingCoilAndDiscreteMultiRegisterBranches();
    void scalarRegisterFormatterMatrix();
    void multiRegisterFormatterMatrix();
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

void TestFormatUtils::inputRegisterFormatterBranches()
{
    QVariant out;

    QCOMPARE(formatBinaryValue(QModbusDataUnit::InputRegisters, 0x00F0, ByteOrder::Swapped, out, false),
             QStringLiteral("1111 0000 0000 0000"));
    QCOMPARE(out.toUInt(), 0xF000u);

    QCOMPARE(formatUInt16Value(QModbusDataUnit::InputRegisters, 0x0100, ByteOrder::Swapped, false, out, false),
             QStringLiteral("    1"));
    QCOMPARE(out.toUInt(), 0x0001u);

    const QString signedText = formatInt16Value(QModbusDataUnit::InputRegisters, qint16(0x00FF), ByteOrder::Swapped,
                                                out, false);
    QVERIFY(signedText.trimmed() == QStringLiteral("-256"));
    QCOMPARE(out.toInt(), -256);

    QCOMPARE(formatHexValue(QModbusDataUnit::InputRegisters, 0x00AB, ByteOrder::Swapped, out, false),
             QStringLiteral("0xAB00"));
    QCOMPARE(out.toUInt(), 0xAB00u);

    const QString ansi = formatAnsiValue(QModbusDataUnit::InputRegisters, 0x4142, ByteOrder::Swapped,
                                         QStringLiteral("UTF-8"), out, false);
    QVERIFY(!ansi.isEmpty());
    QCOMPARE(out.toUInt(), 0x4241u);
}

void TestFormatUtils::unsignedFormatterLeadingZeroBranches()
{
    QVariant out;

    QCOMPARE(formatUInt32Value(QModbusDataUnit::HoldingRegisters, 7, 0, ByteOrder::Direct, true, false, out, false),
             QStringLiteral("0000000007"));
    QCOMPARE(out.toUInt(), 7u);

    QCOMPARE(formatUInt32Value(QModbusDataUnit::DiscreteInputs, 8, 0, ByteOrder::Direct, false, false, out, false),
             QStringLiteral("8"));
    QCOMPARE(out.toUInt(), 8u);

    QCOMPARE(formatUInt64Value(QModbusDataUnit::HoldingRegisters, 9, 0, 0, 0, ByteOrder::Direct, true, false, out, false),
             QStringLiteral("00000000000000000009"));
    QCOMPARE(out.toULongLong(), Q_UINT64_C(9));

    QCOMPARE(formatUInt64Value(QModbusDataUnit::Coils, 10, 0, 0, 0, ByteOrder::Direct, false, false, out, false),
             QStringLiteral("10"));
    QCOMPARE(out.toUInt(), 10u);
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

void TestFormatUtils::emptyArrays()
{
    QVERIFY(formatUInt8Array(DataType::UInt16, true, QByteArray()).isEmpty());
    QVERIFY(formatUInt16Array(DataType::Hex, false, QByteArray(), ByteOrder::Direct).isEmpty());
}

void TestFormatUtils::bracketedRegisterFormatters()
{
    QVariant out;

    QCOMPARE(formatBinaryValue(QModbusDataUnit::HoldingRegisters, 0x000F, ByteOrder::Direct, out),
             QStringLiteral("<0000 0000 0000 1111>"));
    QCOMPARE(out.toUInt(), 0x000Fu);

    QCOMPARE(formatUInt16Value(QModbusDataUnit::HoldingRegisters, 12, ByteOrder::Direct, false, out),
             QStringLiteral("<   12>"));
    QCOMPARE(out.toUInt(), 12u);

    const QString intText = formatInt16Value(QModbusDataUnit::HoldingRegisters, qint16(-12), ByteOrder::Direct, out);
    QVERIFY(intText.startsWith('<'));
    QVERIFY(intText.endsWith('>'));
    QCOMPARE(out.toInt(), -12);

    QCOMPARE(formatHexValue(QModbusDataUnit::HoldingRegisters, 0x00EF, ByteOrder::Direct, out),
             QStringLiteral("<0x00EF>"));
    QCOMPARE(out.toUInt(), 0x00EFu);

    const QString ansi = formatAnsiValue(QModbusDataUnit::HoldingRegisters, 0x4142, ByteOrder::Direct,
                                         QStringLiteral("UTF-8"), out);
    QVERIFY(ansi.startsWith('<'));
    QVERIFY(ansi.endsWith('>'));
    QCOMPARE(out.toUInt(), 0x4142u);
}

void TestFormatUtils::holdingRegisterDeferredBranches()
{
    QVariant out = 42;

    QVERIFY(formatFloatValue(QModbusDataUnit::HoldingRegisters, 1, 2, ByteOrder::Direct, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);

    QVERIFY(formatInt32Value(QModbusDataUnit::HoldingRegisters, 1, 2, ByteOrder::Direct, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);

    QVERIFY(formatUInt32Value(QModbusDataUnit::HoldingRegisters, 1, 2, ByteOrder::Direct, false, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);

    QVERIFY(formatDoubleValue(QModbusDataUnit::HoldingRegisters, 1, 2, 3, 4, ByteOrder::Direct, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);

    QVERIFY(formatInt64Value(QModbusDataUnit::HoldingRegisters, 1, 2, 3, 4, ByteOrder::Direct, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);

    QVERIFY(formatUInt64Value(QModbusDataUnit::HoldingRegisters, 1, 2, 3, 4, ByteOrder::Direct, false, true, out).isEmpty());
    QCOMPARE(out.toInt(), 42);
}

void TestFormatUtils::remainingCoilAndDiscreteMultiRegisterBranches()
{
    QVariant out;

    QCOMPARE(formatUInt32Value(QModbusDataUnit::Coils, 13, 0, ByteOrder::Direct, true, false, out),
             QStringLiteral("<13>"));
    QCOMPARE(out.toUInt(), 13u);

    QCOMPARE(formatDoubleValue(QModbusDataUnit::Coils, 14, 0, 0, 0, ByteOrder::Direct, false, out, false),
             QStringLiteral("14"));
    QCOMPARE(out.toUInt(), 14u);

    QCOMPARE(formatInt32Value(QModbusDataUnit::DiscreteInputs, 15, 0, ByteOrder::Direct, false, out, false),
             QStringLiteral("15"));
    QCOMPARE(out.toUInt(), 15u);

    QCOMPARE(formatUInt32Value(QModbusDataUnit::DiscreteInputs, 16, 0, ByteOrder::Direct, false, false, out),
             QStringLiteral("<16>"));
    QCOMPARE(out.toUInt(), 16u);

    QCOMPARE(formatInt64Value(QModbusDataUnit::Coils, 17, 0, 0, 0, ByteOrder::Direct, false, out, false),
             QStringLiteral("17"));
    QCOMPARE(out.toUInt(), 17u);

    QCOMPARE(formatUInt64Value(QModbusDataUnit::DiscreteInputs, 18, 0, 0, 0, ByteOrder::Direct, false, false, out, false),
             QStringLiteral("18"));
    QCOMPARE(out.toUInt(), 18u);
}

void TestFormatUtils::scalarRegisterFormatterMatrix()
{
    const QVector<QModbusDataUnit::RegisterType> registerTypes = {
        QModbusDataUnit::Coils,
        QModbusDataUnit::DiscreteInputs,
        QModbusDataUnit::HoldingRegisters,
        QModbusDataUnit::InputRegisters,
        static_cast<QModbusDataUnit::RegisterType>(999)
    };
    const QVector<ByteOrder> byteOrders = {
        ByteOrder::Direct,
        ByteOrder::Swapped,
        static_cast<ByteOrder>(99)
    };

    for (const auto registerType : registerTypes) {
        for (const auto byteOrder : byteOrders) {
            for (const bool brackets : {true, false}) {
                QVariant out;

                const QString binary = formatBinaryValue(registerType, 0x00F0, byteOrder, out, brackets);
                if (registerType == QModbusDataUnit::Coils || registerType == QModbusDataUnit::DiscreteInputs) {
                    QCOMPARE(binary, brackets ? QStringLiteral("<240>") : QStringLiteral("240"));
                    QCOMPARE(out.toUInt(), 0x00F0u);
                } else if (registerType == QModbusDataUnit::HoldingRegisters
                           || registerType == QModbusDataUnit::InputRegisters) {
                    QVERIFY(!binary.isEmpty());
                    QVERIFY(binary.contains(QStringLiteral("1111")));
                    QVERIFY(out.isValid());
                } else {
                    QVERIFY(binary.isEmpty());
                    QCOMPARE(out.toUInt(), 0x00F0u);
                }

                const QString hex = formatHexValue(registerType, 0x00AB, byteOrder, out, brackets);
                if (registerType == QModbusDataUnit::Coils || registerType == QModbusDataUnit::DiscreteInputs) {
                    QCOMPARE(hex, brackets ? QStringLiteral("<171>") : QStringLiteral("171"));
                } else if (registerType == QModbusDataUnit::HoldingRegisters
                           || registerType == QModbusDataUnit::InputRegisters) {
                    QVERIFY(hex.contains(QStringLiteral("0x")));
                    QVERIFY(hex.startsWith(brackets ? QStringLiteral("<") : QString()));
                } else {
                    QVERIFY(hex.isEmpty());
                }

                const QString uintText = formatUInt16Value(registerType, 12, byteOrder, false, out, brackets);
                if (registerType == QModbusDataUnit::Coils || registerType == QModbusDataUnit::DiscreteInputs) {
                    QCOMPARE(uintText, brackets ? QStringLiteral("<12>") : QStringLiteral("12"));
                } else if (registerType == QModbusDataUnit::HoldingRegisters
                           || registerType == QModbusDataUnit::InputRegisters) {
                    QVERIFY(uintText.contains(QStringLiteral("12")) || uintText.contains(QStringLiteral("3072")));
                } else {
                    QVERIFY(uintText.isEmpty());
                }

                const QString ansi = formatAnsiValue(registerType, 0x4142, byteOrder, QStringLiteral("UTF-8"), out, brackets);
                if (registerType == QModbusDataUnit::Coils || registerType == QModbusDataUnit::DiscreteInputs) {
                    QCOMPARE(ansi, brackets ? QStringLiteral("<16706>") : QStringLiteral("16706"));
                } else if (registerType == QModbusDataUnit::HoldingRegisters
                           || registerType == QModbusDataUnit::InputRegisters) {
                    QVERIFY(!ansi.isEmpty());
                } else {
                    QVERIFY(ansi.isEmpty());
                }
            }
        }
    }
}

void TestFormatUtils::multiRegisterFormatterMatrix()
{
    const QVector<QModbusDataUnit::RegisterType> registerTypes = {
        QModbusDataUnit::Coils,
        QModbusDataUnit::DiscreteInputs,
        QModbusDataUnit::HoldingRegisters,
        QModbusDataUnit::InputRegisters,
        static_cast<QModbusDataUnit::RegisterType>(999)
    };
    const QVector<ByteOrder> byteOrders = {
        ByteOrder::Direct,
        ByteOrder::Swapped,
        static_cast<ByteOrder>(99)
    };

    for (const auto registerType : registerTypes) {
        for (const auto byteOrder : byteOrders) {
            for (const bool brackets : {true, false}) {
                QVariant out = 123;

                const QString int32Text = formatInt32Value(registerType, 0x0001, 0x0002, byteOrder, false, out, brackets);
                if (registerType == QModbusDataUnit::Coils || registerType == QModbusDataUnit::DiscreteInputs) {
                    QCOMPARE(int32Text, brackets ? QStringLiteral("<1>") : QStringLiteral("1"));
                    QCOMPARE(out.toUInt(), 1u);
                } else if (registerType == QModbusDataUnit::HoldingRegisters
                           || registerType == QModbusDataUnit::InputRegisters) {
                    QVERIFY(!int32Text.isEmpty());
                    QVERIFY(out.isValid());
                } else {
                    QVERIFY(int32Text.isEmpty());
                    QCOMPARE(out.toInt(), 123);
                }

                out = 123;
                const QString uint64Text = formatUInt64Value(registerType, 1, 2, 3, 4, byteOrder, true, false, out, brackets);
                if (registerType == QModbusDataUnit::Coils || registerType == QModbusDataUnit::DiscreteInputs) {
                    QCOMPARE(uint64Text, brackets ? QStringLiteral("<1>") : QStringLiteral("1"));
                    QCOMPARE(out.toUInt(), 1u);
                } else if (registerType == QModbusDataUnit::HoldingRegisters
                           || registerType == QModbusDataUnit::InputRegisters) {
                    QVERIFY(!uint64Text.isEmpty());
                    QVERIFY(out.isValid());
                } else {
                    QVERIFY(uint64Text.isEmpty());
                    QCOMPARE(out.toInt(), 123);
                }
            }
        }
    }
}

QTEST_GUILESS_MAIN(TestFormatUtils)
#include "test_formatutils.moc"
