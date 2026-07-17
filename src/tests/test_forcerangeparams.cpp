// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_forcerangeparams.cpp
/// \brief Unit tests for the force/preset register range helpers in forcerangeparams.h.
///

#include <QTest>

#include "forcerangeparams.h"

class TestForceRangeParams : public QObject
{
    Q_OBJECT

private slots:
    void usesFormLengthWhenPointTypeMatches();
    void usesDefaultLengthWhenFormTypeDiffers();
    void ignoresRememberedRangeWhenFormPresent();
    void usesRememberedRangeWithoutForm();
    void adjustsRememberedAddressToZeroBased();
    void adjustsRememberedAddressToOneBased();
    void defaultsWithoutFormOrRememberedRange();
    void buildsForceRangeFromWriteParams();
};

///
/// \brief Creates view definitions preset for the tests.
/// \return The initialized definitions.
///
static DataViewDefinitions testDefinitions()
{
    DataViewDefinitions dd;
    dd.DeviceId = 7;
    dd.PointAddress = 42;
    dd.PointType = QModbusDataUnit::HoldingRegisters;
    dd.Length = 25;
    dd.LeadingZeros = false;
    return dd;
}

void TestForceRangeParams::usesFormLengthWhenPointTypeMatches()
{
    const auto dd = testDefinitions();
    const auto range = resolveForceRange(QModbusDataUnit::HoldingRegisters, dd, true,
                                         false, AddressSpace::Addr5Digits, {});
    QCOMPARE(range.DeviceId, quint32(7));
    QCOMPARE(range.Address, quint16(42));
    QCOMPARE(range.Length, quint16(25));
    QCOMPARE(range.ZeroBasedAddress, false);
    QCOMPARE(range.AddrSpace, AddressSpace::Addr5Digits);
    QCOMPARE(range.LeadingZeros, false);
}

void TestForceRangeParams::usesDefaultLengthWhenFormTypeDiffers()
{
    const auto dd = testDefinitions();
    const auto range = resolveForceRange(QModbusDataUnit::Coils, dd, true,
                                         false, AddressSpace::Addr6Digits, {});
    QCOMPARE(range.Length, ForceRangeParams{}.Length);
    QCOMPARE(range.Address, quint16(42));
}

void TestForceRangeParams::ignoresRememberedRangeWhenFormPresent()
{
    const auto dd = testDefinitions();
    ForceRangeParamsMap remembered;
    remembered[QModbusDataUnit::HoldingRegisters] = ForceRangeParams{2, 100, 10, false, AddressSpace::Addr6Digits, true};
    const auto range = resolveForceRange(QModbusDataUnit::HoldingRegisters, dd, true,
                                         false, AddressSpace::Addr6Digits, remembered);
    QCOMPARE(range.DeviceId, quint32(7));
    QCOMPARE(range.Address, quint16(42));
    QCOMPARE(range.Length, quint16(25));
}

void TestForceRangeParams::usesRememberedRangeWithoutForm()
{
    const auto dd = testDefinitions();
    ForceRangeParamsMap remembered;
    remembered[QModbusDataUnit::Coils] = ForceRangeParams{2, 100, 10, false, AddressSpace::Addr5Digits, true};
    const auto range = resolveForceRange(QModbusDataUnit::Coils, dd, false,
                                         false, AddressSpace::Addr6Digits, remembered);
    QCOMPARE(range.DeviceId, quint32(2));
    QCOMPARE(range.Address, quint16(100));
    QCOMPARE(range.Length, quint16(10));
    // The address space always follows the current Modbus definitions.
    QCOMPARE(range.AddrSpace, AddressSpace::Addr6Digits);
}

void TestForceRangeParams::adjustsRememberedAddressToZeroBased()
{
    const auto dd = testDefinitions();
    ForceRangeParamsMap remembered;
    remembered[QModbusDataUnit::Coils] = ForceRangeParams{1, 100, 10, false, AddressSpace::Addr6Digits, true};
    auto range = resolveForceRange(QModbusDataUnit::Coils, dd, false,
                                   true, AddressSpace::Addr6Digits, remembered);
    QCOMPARE(range.Address, quint16(99));
    QCOMPARE(range.ZeroBasedAddress, true);

    remembered[QModbusDataUnit::Coils] = ForceRangeParams{1, 0, 10, false, AddressSpace::Addr6Digits, true};
    range = resolveForceRange(QModbusDataUnit::Coils, dd, false,
                              true, AddressSpace::Addr6Digits, remembered);
    QCOMPARE(range.Address, quint16(0));
}

void TestForceRangeParams::adjustsRememberedAddressToOneBased()
{
    const auto dd = testDefinitions();
    ForceRangeParamsMap remembered;
    remembered[QModbusDataUnit::Coils] = ForceRangeParams{1, 99, 10, true, AddressSpace::Addr6Digits, true};
    auto range = resolveForceRange(QModbusDataUnit::Coils, dd, false,
                                   false, AddressSpace::Addr6Digits, remembered);
    QCOMPARE(range.Address, quint16(100));
    QCOMPARE(range.ZeroBasedAddress, false);

    const auto maxAddress = std::numeric_limits<quint16>::max();
    remembered[QModbusDataUnit::Coils] = ForceRangeParams{1, maxAddress, 10, true, AddressSpace::Addr6Digits, true};
    range = resolveForceRange(QModbusDataUnit::Coils, dd, false,
                              false, AddressSpace::Addr6Digits, remembered);
    QCOMPARE(range.Address, maxAddress);
}

void TestForceRangeParams::defaultsWithoutFormOrRememberedRange()
{
    const auto dd = testDefinitions();
    const auto range = resolveForceRange(QModbusDataUnit::Coils, dd, false,
                                         true, AddressSpace::Addr5Digits, {});
    QCOMPARE(range.DeviceId, quint32(7));
    QCOMPARE(range.Address, quint16(42));
    QCOMPARE(range.Length, ForceRangeParams{}.Length);
    QCOMPARE(range.ZeroBasedAddress, true);
    QCOMPARE(range.AddrSpace, AddressSpace::Addr5Digits);
}

void TestForceRangeParams::buildsForceRangeFromWriteParams()
{
    ModbusWriteParams params{};
    params.DeviceId = 3;
    params.Address = 55;
    params.ZeroBasedAddress = true;
    params.AddrSpace = AddressSpace::Addr5Digits;
    params.LeadingZeros = true;
    params.Value = QVariant::fromValue(QVector<quint16>({1, 2, 3, 4}));

    const auto range = forceRangeFromWriteParams(params);
    QCOMPARE(range.DeviceId, quint32(3));
    QCOMPARE(range.Address, quint16(55));
    QCOMPARE(range.Length, quint16(4));
    QCOMPARE(range.ZeroBasedAddress, true);
    QCOMPARE(range.AddrSpace, AddressSpace::Addr5Digits);
    QCOMPARE(range.LeadingZeros, true);
}

QTEST_GUILESS_MAIN(TestForceRangeParams)
#include "test_forcerangeparams.moc"
