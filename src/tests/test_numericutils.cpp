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
    void multiByteLayoutUsesByteOrder();
    void uint16RoundTrip_data();
    void uint16RoundTrip();
    void int32RoundTrip();
    void uint32WrapsInt32();
    void uint64AboveSignedMaxIsUnsigned();
    void floatRoundTrip();
    void int64RoundTrip();
    void doubleRoundTrip();
    void unknownByteOrderRoundTrip();
    void makeValueScalars();
    void makeValueMultiRegister();
    void makeValueRejectsShortInput();
    void makeValueRejectsUnknownType();
    void makeValueRegisterOrder();
    void makeValueCoversAllMultiRegisterTypes_data();
    void makeValueCoversAllMultiRegisterTypes();
};

void TestNumericUtils::uint16ByteLayout()
{
    QCOMPARE(makeUInt16(0x34, 0x12, ByteOrder::Direct), quint16(0x1234));
    QCOMPARE(makeUInt16(0x34, 0x12, ByteOrder::Swapped), quint16(0x3412));
}

void TestNumericUtils::multiByteLayoutUsesByteOrder()
{
    quint16 lo = 0, hi = 0;
    breakInt32(0x11223344, lo, hi, ByteOrder::Direct);
    QCOMPARE(lo, quint16(0x3344));
    QCOMPARE(hi, quint16(0x1122));

    breakInt32(0x11223344, lo, hi, ByteOrder::Swapped);
    QCOMPARE(lo, quint16(0x4433));
    QCOMPARE(hi, quint16(0x2211));

    QCOMPARE(makeInt32(0x3344, 0x1122, ByteOrder::Direct), qint32(0x11223344));
    QCOMPARE(makeInt32(0x4433, 0x2211, ByteOrder::Swapped), qint32(0x11223344));

    quint16 regs[4] = {0, 0, 0, 0};
    breakUInt64(Q_UINT64_C(0x0102030405060708), regs[0], regs[1], regs[2], regs[3], ByteOrder::Swapped);
    QCOMPARE(regs[0], quint16(0x0807));
    QCOMPARE(regs[1], quint16(0x0605));
    QCOMPARE(regs[2], quint16(0x0403));
    QCOMPARE(regs[3], quint16(0x0201));
    QCOMPARE(makeUInt64(regs[0], regs[1], regs[2], regs[3], ByteOrder::Swapped),
             Q_UINT64_C(0x0102030405060708));
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

void TestNumericUtils::uint64AboveSignedMaxIsUnsigned()
{
    quint16 r[4] = {0, 0, 0, 0};
    breakUInt64(Q_UINT64_C(0xFEDCBA9876543210), r[0], r[1], r[2], r[3], ByteOrder::Direct);
    QCOMPARE(makeUInt64(r[0], r[1], r[2], r[3], ByteOrder::Direct), Q_UINT64_C(0xFEDCBA9876543210));

    const QVector<quint16> regs{r[0], r[1], r[2], r[3]};
    const auto v = makeValue(regs, DataType::UInt64, RegisterOrder::LSRF, ByteOrder::Direct);
    QCOMPARE(v.userType(), qMetaTypeId<quint64>());
    QCOMPARE(v.toULongLong(), Q_UINT64_C(0xFEDCBA9876543210));
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

void TestNumericUtils::unknownByteOrderRoundTrip()
{
    const auto unknownOrder = static_cast<ByteOrder>(99);

    QCOMPARE(toByteOrderValue<quint16>(0x1234, unknownOrder), quint16(0x1234));
    QCOMPARE(toByteOrderValue<quint32>(0x12345678u, unknownOrder), quint32(0x12345678u));
    QCOMPARE(toByteOrderValue<quint64>(Q_UINT64_C(0x123456789ABCDEF0), unknownOrder),
             Q_UINT64_C(0x123456789ABCDEF0));
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

void TestNumericUtils::makeValueRejectsUnknownType()
{
    QVERIFY(!makeValue({0x1234}, static_cast<DataType>(99), RegisterOrder::MSRF, ByteOrder::Direct).isValid());
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

void TestNumericUtils::makeValueCoversAllMultiRegisterTypes_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("regOrder");
    QTest::addColumn<int>("byteOrder");

    for(const auto type : {DataType::Float32, DataType::Int32, DataType::UInt32, DataType::Float64,
                           DataType::Int64, DataType::UInt64})
        for(const auto regOrder : {RegisterOrder::MSRF, RegisterOrder::LSRF})
            for(const auto byteOrder : {ByteOrder::Direct, ByteOrder::Swapped})
                QTest::addRow("%s-%s-%s", qPrintable(enumToString(type)), qPrintable(enumToString(regOrder)),
                              qPrintable(enumToString(byteOrder)))
                    << int(type) << int(regOrder) << int(byteOrder);
}

void TestNumericUtils::makeValueCoversAllMultiRegisterTypes()
{
    QFETCH(int, type);
    QFETCH(int, regOrder);
    QFETCH(int, byteOrder);

    const auto dataType = static_cast<DataType>(type);
    const auto order = static_cast<RegisterOrder>(regOrder);
    const auto bo = static_cast<ByteOrder>(byteOrder);

    QVector<quint16> regs;
    switch(dataType)
    {
        case DataType::Float32:
        {
            quint16 lo = 0, hi = 0;
            breakFloat(12.5f, lo, hi, bo);
            regs = order == RegisterOrder::LSRF ? QVector<quint16>{lo, hi} : QVector<quint16>{hi, lo};
            QCOMPARE(makeValue(regs, dataType, order, bo).toFloat(), 12.5f);
            break;
        }

        case DataType::Int32:
        {
            quint16 lo = 0, hi = 0;
            breakInt32(-123456, lo, hi, bo);
            regs = order == RegisterOrder::LSRF ? QVector<quint16>{lo, hi} : QVector<quint16>{hi, lo};
            QCOMPARE(makeValue(regs, dataType, order, bo).toInt(), -123456);
            break;
        }

        case DataType::UInt32:
        {
            quint16 lo = 0, hi = 0;
            breakUInt32(0x89ABCDEFu, lo, hi, bo);
            regs = order == RegisterOrder::LSRF ? QVector<quint16>{lo, hi} : QVector<quint16>{hi, lo};
            QCOMPARE(makeValue(regs, dataType, order, bo).toUInt(), 0x89ABCDEFu);
            break;
        }

        case DataType::Float64:
        {
            quint16 r[4] = {0, 0, 0, 0};
            breakDouble(6.25, r[0], r[1], r[2], r[3], bo);
            regs = order == RegisterOrder::LSRF
                ? QVector<quint16>{r[0], r[1], r[2], r[3]}
                : QVector<quint16>{r[3], r[2], r[1], r[0]};
            QCOMPARE(makeValue(regs, dataType, order, bo).toDouble(), 6.25);
            break;
        }

        case DataType::Int64:
        {
            quint16 r[4] = {0, 0, 0, 0};
            breakInt64(Q_INT64_C(-1234567890123), r[0], r[1], r[2], r[3], bo);
            regs = order == RegisterOrder::LSRF
                ? QVector<quint16>{r[0], r[1], r[2], r[3]}
                : QVector<quint16>{r[3], r[2], r[1], r[0]};
            QCOMPARE(makeValue(regs, dataType, order, bo).toLongLong(), Q_INT64_C(-1234567890123));
            break;
        }

        case DataType::UInt64:
        {
            quint16 r[4] = {0, 0, 0, 0};
            breakUInt64(Q_UINT64_C(0xFEDCBA9876543210), r[0], r[1], r[2], r[3], bo);
            regs = order == RegisterOrder::LSRF
                ? QVector<quint16>{r[0], r[1], r[2], r[3]}
                : QVector<quint16>{r[3], r[2], r[1], r[0]};
            QCOMPARE(makeValue(regs, dataType, order, bo).toULongLong(), Q_UINT64_C(0xFEDCBA9876543210));
            break;
        }

        default:
            QFAIL("Unexpected data type in test table.");
    }
}

QTEST_GUILESS_MAIN(TestNumericUtils)
#include "test_numericutils.moc"
