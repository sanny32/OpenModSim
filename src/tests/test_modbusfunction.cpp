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
    void allFunctionNames_data();
    void allFunctionNames();
    void unknownFunctionHasEmptyName();
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

void TestModbusFunction::allFunctionNames_data()
{
    QTest::addColumn<int>("code");
    QTest::addColumn<QString>("name");

    QTest::newRow("ReadCoils")                   << int(QModbusPdu::ReadCoils)                   << "READ COILS";
    QTest::newRow("ReadDiscreteInputs")          << int(QModbusPdu::ReadDiscreteInputs)          << "READ INPUTS";
    QTest::newRow("ReadHoldingRegisters")        << int(QModbusPdu::ReadHoldingRegisters)        << "READ HOLDING REGS";
    QTest::newRow("ReadInputRegisters")          << int(QModbusPdu::ReadInputRegisters)          << "READ INPUT REGS";
    QTest::newRow("WriteSingleCoil")             << int(QModbusPdu::WriteSingleCoil)             << "WRITE SINGLE COIL";
    QTest::newRow("WriteSingleRegister")         << int(QModbusPdu::WriteSingleRegister)         << "WRITE SINGLE REG";
    QTest::newRow("ReadExceptionStatus")         << int(QModbusPdu::ReadExceptionStatus)         << "READ EXCEPTION STAT";
    QTest::newRow("Diagnostics")                 << int(QModbusPdu::Diagnostics)                 << "DIAGNOSTICS";
    QTest::newRow("GetCommEventCounter")         << int(QModbusPdu::GetCommEventCounter)         << "GET COMM EVENT CNT";
    QTest::newRow("GetCommEventLog")             << int(QModbusPdu::GetCommEventLog)             << "GET COMM EVENT LOG";
    QTest::newRow("WriteMultipleCoils")          << int(QModbusPdu::WriteMultipleCoils)          << "WRITE MULT COILS";
    QTest::newRow("WriteMultipleRegisters")      << int(QModbusPdu::WriteMultipleRegisters)      << "WRITE MULT REGS";
    QTest::newRow("ReportServerId")              << int(QModbusPdu::ReportServerId)              << "REPORT SLAVE ID";
    QTest::newRow("ReadFileRecord")              << int(QModbusPdu::ReadFileRecord)              << "READ FILE RECORD";
    QTest::newRow("WriteFileRecord")             << int(QModbusPdu::WriteFileRecord)             << "WRITE FILE RECORD";
    QTest::newRow("MaskWriteRegister")           << int(QModbusPdu::MaskWriteRegister)           << "MASK WRITE REG";
    QTest::newRow("ReadWriteMultipleRegisters")  << int(QModbusPdu::ReadWriteMultipleRegisters)  << "READ WRITE MULT REGS";
    QTest::newRow("ReadFifoQueue")               << int(QModbusPdu::ReadFifoQueue)               << "READ FIFO QUEUE";
    QTest::newRow("EncapsulatedInterfaceTransport") << int(QModbusPdu::EncapsulatedInterfaceTransport) << "ENC IFACE TRANSPORT";
}

void TestModbusFunction::allFunctionNames()
{
    QFETCH(int, code);
    QFETCH(QString, name);
    QCOMPARE(QString(ModbusFunction(static_cast<QModbusPdu::FunctionCode>(code))), name);
}

void TestModbusFunction::unknownFunctionHasEmptyName()
{
    QCOMPARE(QString(ModbusFunction(static_cast<QModbusPdu::FunctionCode>(0x00))), QString());
    QCOMPARE(QString(ModbusFunction(static_cast<QModbusPdu::FunctionCode>(0x65))), QString());
}

QTEST_GUILESS_MAIN(TestModbusFunction)
#include "test_modbusfunction.moc"
