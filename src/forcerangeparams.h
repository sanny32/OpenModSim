// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file forcerangeparams.h
/// \brief Declares the force/preset register range helpers.
///

#ifndef FORCERANGEPARAMS_H
#define FORCERANGEPARAMS_H

#include <limits>
#include <QHash>
#include <QModbusDataUnit>
#include "displaydefinition.h"
#include "modbuswriteparams.h"

///
/// \brief The ForceRangeParams struct describes the register range last edited
/// in a force/preset dialog.
///
struct ForceRangeParams
{
    quint32 DeviceId = 1;
    quint16 Address = 1;
    quint16 Length = 100;
    bool ZeroBasedAddress = false;
    AddressSpace AddrSpace = AddressSpace::Addr6Digits;
    bool LeadingZeros = true;
};

using ForceRangeParamsMap = QHash<QModbusDataUnit::RegisterType, ForceRangeParams>;

///
/// \brief resolveForceRange resolves the register range for a force/preset dialog
/// from the active view definitions, falling back to the last remembered range
/// (migrating its address across an address-base change) or to defaults.
/// \param type The register type the dialog is opened for.
/// \param dd The active view definitions, or the preference defaults when no view is open.
/// \param formPresent True when a data view is currently active.
/// \param zeroBasedAddress The current address base.
/// \param addrSpace The current address space of the Modbus definitions.
/// \param remembered Ranges remembered from previously accepted dialogs.
/// \return The resolved range.
///
inline ForceRangeParams resolveForceRange(QModbusDataUnit::RegisterType type,
                                          const DataViewDefinitions& dd,
                                          bool formPresent,
                                          bool zeroBasedAddress,
                                          AddressSpace addrSpace,
                                          const ForceRangeParamsMap& remembered)
{
    ForceRangeParams range{
        dd.DeviceId,
        dd.PointAddress,
        formPresent && dd.PointType == type ? dd.Length : ForceRangeParams{}.Length,
        zeroBasedAddress,
        addrSpace,
        dd.LeadingZeros
    };
    if(!formPresent && remembered.contains(type)) {
        range = remembered.value(type);
        if (range.ZeroBasedAddress != zeroBasedAddress) {
            int adjustedAddress = static_cast<int>(range.Address);
            if (zeroBasedAddress) {
                adjustedAddress = qMax(0, adjustedAddress - 1);
            } else if (adjustedAddress < std::numeric_limits<quint16>::max()) {
                adjustedAddress += 1;
            }
            range.Address = static_cast<quint16>(adjustedAddress);
            range.ZeroBasedAddress = zeroBasedAddress;
        }
        range.AddrSpace = addrSpace;
    }
    return range;
}

///
/// \brief forceRangeFromWriteParams builds the range to remember from the write
/// parameters accepted in a force/preset dialog.
/// \param params The write parameters after the dialog was accepted.
/// \return The range to remember, its length taken from the edited value vector.
///
inline ForceRangeParams forceRangeFromWriteParams(const ModbusWriteParams& params)
{
    return ForceRangeParams{
        params.DeviceId,
        params.Address,
        static_cast<quint16>(params.Value.value<QVector<quint16>>().size()),
        params.ZeroBasedAddress,
        params.AddrSpace,
        params.LeadingZeros
    };
}

#endif // FORCERANGEPARAMS_H
