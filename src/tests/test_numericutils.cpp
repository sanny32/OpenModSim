// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_numericutils.cpp
/// \brief Unit tests for the register/byte packing helpers in numericutils.h.
///

#include <QTest>

#include "numericutils.h"

class TestNumericUtils : public QObject
{
    Q_OBJECT

private slots:
    void uint16ByteLayout();
    void uint16RoundTrip_data();
    void uint16RoundTrip();
    void int32RoundTrip();
    void uint32WrapsInt32();
    void floatRoundTrip();
    void int64RoundTrip();
    void doubleRoundTrip();
    void makeValueScalars();
    void makeValueMultiRegister();
    void makeValueRejectsShortInput();
    void makeValueRegisterOrder();
};

void TestNumericUtils::uint16ByteLayout()
{
    QCOMPARE(makeUInt16(0x34, 0x12, ByteOrder::Direct), quint16(0x1234));
    QCOMPARE(makeUInt16(0x34, 0x12, ByteOrder::Swapped), quint16(0x3412));
}

void TestNumericUtils::uint16RoundTrip_data()
{
    QTest::addColumn<int>("order");
    QTest::newRow("direct") << int(ByteOrder::Direct);
    QTest::newRow("swapped") << int(ByteOrder::Swapped);
}

void TestNumericUtils::uint16RoundTrip()
{
    QFETCH(int, order);
    const ByteOrder bo = static_cast<ByteOrder>(order);

    quint8 lo = 0, hi = 0;
    breakUInt16(0xBEEF, lo, hi, bo);
    QCOMPARE(makeUInt16(lo, hi, bo), quint16(0xBEEF));
}

void TestNumericUtils::int32RoundTrip()
{
    for (const ByteOrder bo : {ByteOrder::Direct, ByteOrder::Swapped})
    {
        quint16 lo = 0, hi = 0;
        breakInt32(-123456789, lo, hi, bo);
        QCOMPARE(makeInt32(lo, hi, bo), qint32(-123456789));
    }
}

void TestNumericUtils::uint32WrapsInt32()
{
    quint16 lo = 0, hi = 0;
    breakUInt32(0xFFFFFFFFu, lo, hi, ByteOrder::Direct);
    QCOMPARE(makeUInt32(lo, hi, ByteOrder::Direct), quint32(0xFFFFFFFFu));
}

void TestNumericUtils::floatRoundTrip()
{
    for (const ByteOrder bo : {ByteOrder::Direct, ByteOrder::Swapped})
    {
        quint16 lo = 0, hi = 0;
        breakFloat(3.14159f, lo, hi, bo);
        QCOMPARE(makeFloat(lo, hi, bo), 3.14159f);
    }
}

void TestNumericUtils::int64RoundTrip()
{
    for (const ByteOrder bo : {ByteOrder::Direct, ByteOrder::Swapped})
    {
        quint16 r[4] = {0, 0, 0, 0};
        breakInt64(Q_INT64_C(-1234567890123), r[0], r[1], r[2], r[3], bo);
        QCOMPARE(makeInt64(r[0], r[1], r[2], r[3], bo), Q_INT64_C(-1234567890123));
    }
}

void TestNumericUtils::doubleRoundTrip()
{
    for (const ByteOrder bo : {ByteOrder::Direct, ByteOrder::Swapped})
    {
        quint16 r[4] = {0, 0, 0, 0};
        breakDouble(2.718281828459045, r[0], r[1], r[2], r[3], bo);
        QCOMPARE(makeDouble(r[0], r[1], r[2], r[3], bo), 2.718281828459045);
    }
}

void TestNumericUtils::makeValueScalars()
{
    const QVector<quint16> regs{0x1234};
    QCOMPARE(makeValue(regs, DataType::UInt16, RegisterOrder::MSRF, ByteOrder::Direct).toUInt(), 0x1234u);
    QCOMPARE(makeValue(regs, DataType::Int16, RegisterOrder::MSRF, ByteOrder::Direct).toInt(), 0x1234);

    const QVector<quint16> negative{0xFFFF};
    QCOMPARE(makeValue(negative, DataType::Int16, RegisterOrder::MSRF, ByteOrder::Direct).toInt(), -1);
}

void TestNumericUtils::makeValueMultiRegister()
{
    quint16 lo = 0, hi = 0;
    breakInt32(0x11223344, lo, hi, ByteOrder::Direct);

    const QVector<quint16> lsrf{lo, hi};
    QCOMPARE(makeValue(lsrf, DataType::Int32, RegisterOrder::LSRF, ByteOrder::Direct).toInt(), 0x11223344);

    const QVector<quint16> msrf{hi, lo};
    QCOMPARE(makeValue(msrf, DataType::Int32, RegisterOrder::MSRF, ByteOrder::Direct).toInt(), 0x11223344);
}

void TestNumericUtils::makeValueRejectsShortInput()
{
    const QVector<quint16> tooShort{0x0001};
    QVERIFY(!makeValue(tooShort, DataType::Int32, RegisterOrder::MSRF, ByteOrder::Direct).isValid());
    QVERIFY(!makeValue({}, DataType::UInt16, RegisterOrder::MSRF, ByteOrder::Direct).isValid());
}

void TestNumericUtils::makeValueRegisterOrder()
{
    quint16 r[4] = {0, 0, 0, 0};
    breakInt64(Q_INT64_C(0x0102030405060708), r[0], r[1], r[2], r[3], ByteOrder::Direct);

    const QVector<quint16> lsrf{r[0], r[1], r[2], r[3]};
    const QVector<quint16> msrf{r[3], r[2], r[1], r[0]};
    QCOMPARE(makeValue(lsrf, DataType::Int64, RegisterOrder::LSRF, ByteOrder::Direct),
             makeValue(msrf, DataType::Int64, RegisterOrder::MSRF, ByteOrder::Direct));
}

QTEST_GUILESS_MAIN(TestNumericUtils)
#include "test_numericutils.moc"
