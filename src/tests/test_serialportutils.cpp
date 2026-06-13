// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_serialportutils.cpp
/// \brief Unit tests for Parity_toString in serialportutils.h.
///

#include <QTest>

#include "serialportutils.h"

class TestSerialPortUtils : public QObject
{
    Q_OBJECT

private slots:
    void parityNames_data();
    void parityNames();
    void unknownParityIsEmpty();
};

void TestSerialPortUtils::parityNames_data()
{
    QTest::addColumn<int>("parity");
    QTest::addColumn<QString>("name");

    QTest::newRow("none")  << int(QSerialPort::NoParity)    << "NONE";
    QTest::newRow("even")  << int(QSerialPort::EvenParity)  << "EVEN";
    QTest::newRow("odd")   << int(QSerialPort::OddParity)   << "ODD";
    QTest::newRow("space") << int(QSerialPort::SpaceParity) << "SPACE";
    QTest::newRow("mark")  << int(QSerialPort::MarkParity)  << "MARK";
}

void TestSerialPortUtils::parityNames()
{
    QFETCH(int, parity);
    QFETCH(QString, name);
    QCOMPARE(Parity_toString(static_cast<QSerialPort::Parity>(parity)), name);
}

void TestSerialPortUtils::unknownParityIsEmpty()
{
    QCOMPARE(Parity_toString(static_cast<QSerialPort::Parity>(99)), QString());
}

QTEST_GUILESS_MAIN(TestSerialPortUtils)
#include "test_serialportutils.moc"
