// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbusfunction.cpp
/// \brief Unit tests for the ModbusFunction wrapper in modbusfunction.h.
///

#include <QTest>

#include "modbusfunction.h"

class TestModbusFunction : public QObject
{
    Q_OBJECT

private slots:
    void validCodesAreRecognised();
    void invalidCodeIsRejected();
    void exceptionBitDetected();
    void nameMatchesFunction();
    void exceptionNameStripsExceptionBit();
    void intConversion();
};

void TestModbusFunction::validCodesAreRecognised()
{
    QVERIFY(ModbusFunction(QModbusPdu::ReadCoils).isValid());
    QVERIFY(ModbusFunction(QModbusPdu::WriteMultipleRegisters).isValid());
    QVERIFY(ModbusFunction(QModbusPdu::MaskWriteRegister).isValid());
}

void TestModbusFunction::invalidCodeIsRejected()
{
    QVERIFY(!ModbusFunction(static_cast<QModbusPdu::FunctionCode>(0x00)).isValid());
    QVERIFY(!ModbusFunction(static_cast<QModbusPdu::FunctionCode>(0x42)).isValid());
}

void TestModbusFunction::exceptionBitDetected()
{
    const auto raised = static_cast<QModbusPdu::FunctionCode>(QModbusPdu::ReadCoils | QModbusPdu::ExceptionByte);
    QVERIFY(ModbusFunction(raised).isException());
    QVERIFY(!ModbusFunction(QModbusPdu::ReadCoils).isException());
}

void TestModbusFunction::nameMatchesFunction()
{
    QCOMPARE(QString(ModbusFunction(QModbusPdu::ReadCoils)), QStringLiteral("READ COILS"));
    QCOMPARE(QString(ModbusFunction(QModbusPdu::WriteSingleRegister)), QStringLiteral("WRITE SINGLE REG"));
    QCOMPARE(QString(ModbusFunction(QModbusPdu::ReadHoldingRegisters)), QStringLiteral("READ HOLDING REGS"));
}

void TestModbusFunction::exceptionNameStripsExceptionBit()
{
    const auto raised = static_cast<QModbusPdu::FunctionCode>(QModbusPdu::ReadHoldingRegisters | QModbusPdu::ExceptionByte);
    QCOMPARE(QString(ModbusFunction(raised)), QStringLiteral("READ HOLDING REGS"));
}

void TestModbusFunction::intConversion()
{
    QCOMPARE(int(ModbusFunction(QModbusPdu::WriteSingleCoil)), int(QModbusPdu::WriteSingleCoil));
}

QTEST_GUILESS_MAIN(TestModbusFunction)
#include "test_modbusfunction.moc"
