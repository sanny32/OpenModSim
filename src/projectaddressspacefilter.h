// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectaddressspacefilter.h
/// \brief Declares helpers for filtering saved project address-space data.
///

#ifndef PROJECTADDRESSSPACEFILTER_H
#define PROJECTADDRESSSPACEFILTER_H

#include <QModbusDataUnit>
#include <QVector>

#include "controls/outputtypes.h"
#include "datasimulator.h"

///
/// \brief The ProjectAddressSpaceRange struct describes an address range used by project forms.
///
struct ProjectAddressSpaceRange
{
    quint8 DeviceId = 0;
    QModbusDataUnit::RegisterType Type = QModbusDataUnit::Invalid;
    quint16 StartAddress = 0;
    quint16 Length = 0;
};

///
/// \brief The ProjectAddressSpaceValue struct stores one saved Modbus value.
///
struct ProjectAddressSpaceValue
{
    ItemMapKey Key = { 0, QModbusDataUnit::Invalid, 0 };
    quint16 Value = 0;
};

using ProjectAddressSpaceRanges = QVector<ProjectAddressSpaceRange>;
using ProjectAddressSpaceValues = QVector<ProjectAddressSpaceValue>;

bool projectAddressSpaceContains(const ProjectAddressSpaceRanges& ranges, const ItemMapKey& key);
bool projectAddressSpaceOverlaps(const ProjectAddressSpaceRanges& ranges, const ProjectAddressSpaceRange& range);
AddressDescriptionMap filterProjectAddressDescriptions(const AddressDescriptionMap& descriptions,
                                                       const ProjectAddressSpaceRanges& ranges);
AddressTimestampMap filterProjectAddressTimestamps(const AddressTimestampMap& timestamps,
                                                   const ProjectAddressSpaceRanges& ranges);
ProjectAddressSpaceValues filterProjectAddressValues(const ProjectAddressSpaceValues& values,
                                                     const ProjectAddressSpaceRanges& ranges);
ModbusSimulationMap2 filterProjectAddressSimulations(const ModbusSimulationMap2& simulations,
                                                     const ProjectAddressSpaceRanges& ranges);
ProjectAddressSpaceRanges projectAddressSpaceModifiedRanges(const AddressDescriptionMap& descriptions,
                                                            const AddressTimestampMap& timestamps,
                                                            const ModbusSimulationMap2& simulations);

#endif // PROJECTADDRESSSPACEFILTER_H
