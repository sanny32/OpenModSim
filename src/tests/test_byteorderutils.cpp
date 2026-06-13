// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_byteorderutils.cpp
/// \brief Unit tests for toByteOrderValue() in byteorderutils.h.
///

#include <QTest>
#include <QtEndian>

#include "byteorderutils.h"

class TestByteOrderUtils : public QObject
{
    Q_OBJECT

private slots:
    void directMatchesLittleEndian();
    void swappedMatchesBigEndian();
    void isInvolutionPerOrder();
};

void TestByteOrderUtils::directMatchesLittleEndian()
{
    QCOMPARE(toByteOrderValue<quint16>(0x1234, ByteOrder::Direct), qToLittleEndian<quint16>(0x1234));
    QCOMPARE(toByteOrderValue<quint32>(0x11223344u, ByteOrder::Direct), qToLittleEndian<quint32>(0x11223344u));
}

void TestByteOrderUtils::swappedMatchesBigEndian()
{
    QCOMPARE(toByteOrderValue<quint16>(0x1234, ByteOrder::Swapped), qToBigEndian<quint16>(0x1234));
    QCOMPARE(toByteOrderValue<quint32>(0x11223344u, ByteOrder::Swapped), qToBigEndian<quint32>(0x11223344u));
}

void TestByteOrderUtils::isInvolutionPerOrder()
{
    for (const ByteOrder bo : {ByteOrder::Direct, ByteOrder::Swapped})
    {
        const quint16 v16 = 0xABCD;
        QCOMPARE(toByteOrderValue<quint16>(toByteOrderValue<quint16>(v16, bo), bo), v16);

        const quint32 v32 = 0xDEADBEEFu;
        QCOMPARE(toByteOrderValue<quint32>(toByteOrderValue<quint32>(v32, bo), bo), v32);
    }
}

QTEST_GUILESS_MAIN(TestByteOrderUtils)
#include "test_byteorderutils.moc"
