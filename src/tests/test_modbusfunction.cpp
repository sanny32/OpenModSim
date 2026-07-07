// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbusfunction.cpp
/// \brief Unit tests for the ModbusFunction wrapper in modbusfunction.h.
///

#include <QTest>

#include "modbusfunction.h"
#include "modbuslimits.h"

class TestModbusFunction : public QObject
{
    Q_OBJECT

private slots:
    void validCodesAreRecognised();
    void validCodesListIsComplete();
    void invalidCodeIsRejected();
    void exceptionBitDetected();
    void nameMatchesFunction();
    void exceptionNameStripsExceptionBit();
    void intConversion();
    void allFunctionNames_data();
    void allFunctionNames();
    void unknownFunctionHasEmptyName();
    void limitsAddressRanges_data();
    void limitsAddressRanges();
    void limitsLengthRangeAtAddressSpaceEdges();
    void limitsFallbackForUnknownAddressSpace();
};

void TestModbusFunction::validCodesAreRecognised()
{
    QVERIFY(ModbusFunction(QModbusPdu::ReadCoils).isValid());
    QVERIFY(ModbusFunction(QModbusPdu::WriteMultipleRegisters).isValid());
    QVERIFY(ModbusFunction(QModbusPdu::MaskWriteRegister).isValid());
}

void TestModbusFunction::validCodesListIsComplete()
{
    const auto codes = ModbusFunction::validCodes();

    QCOMPARE(codes.size(), 19);
    QVERIFY(codes.contains(QModbusPdu::ReadCoils));
    QVERIFY(codes.contains(QModbusPdu::EncapsulatedInterfaceTransport));

    for(const auto code : codes)
        QVERIFY(ModbusFunction(code).isValid());
}

void TestModbusFunction::invalidCodeIsRejected()
{
    QVERIFY(!ModbusFunction(static_cast<QModbusPdu::FunctionCode>(0x00)).isValid());
    QVERIFY(!ModbusFunction(static_cast<QModbusPdu::FunctionCode>(0x42)).isValid());
    QVERIFY(!ModbusFunction(static_cast<QModbusPdu::FunctionCode>(QModbusPdu::ReadCoils | QModbusPdu::ExceptionByte)).isValid());
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

    const auto unknown = static_cast<QModbusPdu::FunctionCode>(0x65 | QModbusPdu::ExceptionByte);
    QCOMPARE(QString(ModbusFunction(unknown)), QString());
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

void TestModbusFunction::limitsAddressRanges_data()
{
    QTest::addColumn<int>("space");
    QTest::addColumn<bool>("zeroBased");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::addColumn<int>("size");

    QTest::newRow("5-digits-one-based") << int(AddressSpace::Addr5Digits) << false << 1 << 9999 << 9999;
    QTest::newRow("5-digits-zero-based") << int(AddressSpace::Addr5Digits) << true << 0 << 9998 << 9999;
    QTest::newRow("6-digits-one-based") << int(AddressSpace::Addr6Digits) << false << 1 << 65536 << 65536;
    QTest::newRow("6-digits-zero-based") << int(AddressSpace::Addr6Digits) << true << 0 << 65535 << 65536;
}

void TestModbusFunction::limitsAddressRanges()
{
    QFETCH(int, space);
    QFETCH(bool, zeroBased);
    QFETCH(int, from);
    QFETCH(int, to);
    QFETCH(int, size);

    const auto addressSpace = static_cast<AddressSpace>(space);
    const auto range = ModbusLimits::addressRange(addressSpace, zeroBased);

    QCOMPARE(range.from(), from);
    QCOMPARE(range.to(), to);
    QCOMPARE(ModbusLimits::addressSpaceSize(addressSpace), size);
}

void TestModbusFunction::limitsLengthRangeAtAddressSpaceEdges()
{
    QCOMPARE(ModbusLimits::lengthRange().from(), 1);
    QCOMPARE(ModbusLimits::lengthRange().to(), 200);
    QCOMPARE(ModbusLimits::slaveRange().from(), 0);
    QCOMPARE(ModbusLimits::slaveRange().to(), 255);

    QCOMPARE(ModbusLimits::lengthRange(1, false, AddressSpace::Addr5Digits).to(), 200);
    QCOMPARE(ModbusLimits::lengthRange(9900, false, AddressSpace::Addr5Digits).to(), 100);
    QCOMPARE(ModbusLimits::lengthRange(9999, false, AddressSpace::Addr5Digits).to(), 1);
    QCOMPARE(ModbusLimits::lengthRange(9999, true, AddressSpace::Addr5Digits).to(), 1);
    QCOMPARE(ModbusLimits::lengthRange(65536, false, AddressSpace::Addr6Digits).to(), 1);
}

void TestModbusFunction::limitsFallbackForUnknownAddressSpace()
{
    const auto unknown = static_cast<AddressSpace>(99);

    QCOMPARE(ModbusLimits::addressSpaceSize(unknown), 65536);
    QCOMPARE(ModbusLimits::addressRange(unknown, false).from(), 1);
    QCOMPARE(ModbusLimits::addressRange(unknown, false).to(), 65536);
    QCOMPARE(ModbusLimits::addressRange(unknown, true).from(), 0);
    QCOMPARE(ModbusLimits::addressRange(unknown, true).to(), 65535);
    QCOMPARE(ModbusLimits::lengthRange(65530, false, unknown).to(), 7);
}

QTEST_GUILESS_MAIN(TestModbusFunction)
#include "test_modbusfunction.moc"
