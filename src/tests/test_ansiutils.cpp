// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_ansiutils.cpp
/// \brief Unit tests for the ANSI <-> register helpers in ansiutils.h.
///

#include <QTest>

#include "ansiutils.h"

class TestAnsiUtils : public QObject
{
    Q_OBJECT

private slots:
    void roundTrip_data();
    void roundTrip();
    void roundTripWithUnknownByteOrder();
    void fromAnsiRejectsWrongSize();
    void printableReplacesControlBytes();
    void printableHandlesBoundaryBytes();
    void printableInsertsSeparator();
    void printableSkipsNonPrintableSeparator();
    void printableHandlesEmptyInput();
    void printableFallsBackOnUnknownCodepage();
};

void TestAnsiUtils::roundTrip_data()
{
    QTest::addColumn<int>("order");
    QTest::newRow("direct") << int(ByteOrder::Direct);
    QTest::newRow("swapped") << int(ByteOrder::Swapped);
}

void TestAnsiUtils::roundTrip()
{
    QFETCH(int, order);
    const ByteOrder bo = static_cast<ByteOrder>(order);
    const QByteArray ansi = uint16ToAnsi(0x4142, bo);
    QCOMPARE(ansi.size(), 2);
    QCOMPARE(uint16FromAnsi(ansi, bo), quint16(0x4142));
}

void TestAnsiUtils::roundTripWithUnknownByteOrder()
{
    const auto unknownOrder = static_cast<ByteOrder>(99);
    const QByteArray ansi = uint16ToAnsi(0x4142, unknownOrder);

    QCOMPARE(ansi.size(), 2);
    QCOMPARE(uint16FromAnsi(ansi, unknownOrder), quint16(0x4142));
}

void TestAnsiUtils::fromAnsiRejectsWrongSize()
{
    QCOMPARE(uint16FromAnsi(QByteArray(), ByteOrder::Direct), quint16(0));
    QCOMPARE(uint16FromAnsi(QByteArray("A"), ByteOrder::Direct), quint16(0));
    QCOMPARE(uint16FromAnsi(QByteArray("ABC"), ByteOrder::Direct), quint16(0));
}

void TestAnsiUtils::printableReplacesControlBytes()
{
    const QString text = printableAnsi(QByteArray::fromHex("4101"), QStringLiteral("UTF-8"));
    QCOMPARE(text, QStringLiteral("A?"));
}

void TestAnsiUtils::printableHandlesBoundaryBytes()
{
    const QString text = printableAnsi(QByteArray::fromHex("1F2041"), QStringLiteral("UTF-8"));
    QCOMPARE(text, QStringLiteral("? A"));
}

void TestAnsiUtils::printableInsertsSeparator()
{
    const QString text = printableAnsi(QByteArray("AB"), QStringLiteral("UTF-8"), QChar(' '));
    QCOMPARE(text, QStringLiteral("A B "));
}

void TestAnsiUtils::printableSkipsNonPrintableSeparator()
{
    const QString text = printableAnsi(QByteArray("AB"), QStringLiteral("UTF-8"), QChar('\n'));
    QCOMPARE(text, QStringLiteral("AB"));
}

void TestAnsiUtils::printableHandlesEmptyInput()
{
    const QString text = printableAnsi(QByteArray(), QStringLiteral("UTF-8"), QChar(' '));
    QVERIFY(text.isEmpty());
}

void TestAnsiUtils::printableFallsBackOnUnknownCodepage()
{
    const QString text = printableAnsi(QByteArray("AB"), QStringLiteral("no-such-codec"));
    QCOMPARE(text, QStringLiteral("AB"));
}

QTEST_GUILESS_MAIN(TestAnsiUtils)
#include "test_ansiutils.moc"
