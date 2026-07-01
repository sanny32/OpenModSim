// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectaddressspacefilter.cpp
/// \brief Implements helpers for filtering saved project address-space data.
///

#include "projectaddressspacefilter.h"

#include "enums.h"

///
/// \brief projectAddressSpaceRangeEnd
/// \param range
/// \return
///
static quint32 projectAddressSpaceRangeEnd(const ProjectAddressSpaceRange& range)
{
    return static_cast<quint32>(range.StartAddress) + range.Length;
}

///
/// \brief projectAddressSpaceContains
/// \param ranges
/// \param key
/// \return
///
bool projectAddressSpaceContains(const ProjectAddressSpaceRanges& ranges, const ItemMapKey& key)
{
    for (const auto& range : ranges) {
        if (range.Length == 0 ||
            range.DeviceId != key.DeviceId ||
            range.Type != key.Type) {
            continue;
        }

        const quint32 address = key.Address;
        if (address >= range.StartAddress && address < projectAddressSpaceRangeEnd(range))
            return true;
    }

    return false;
}

///
/// \brief projectAddressSpaceOverlaps
/// \param ranges
/// \param range
/// \return
///
bool projectAddressSpaceOverlaps(const ProjectAddressSpaceRanges& ranges, const ProjectAddressSpaceRange& range)
{
    if (range.Length == 0)
        return false;

    const quint32 rangeStart = range.StartAddress;
    const quint32 rangeEnd = projectAddressSpaceRangeEnd(range);

    for (const auto& projectRange : ranges) {
        if (projectRange.Length == 0 ||
            projectRange.DeviceId != range.DeviceId ||
            projectRange.Type != range.Type) {
            continue;
        }

        const quint32 projectStart = projectRange.StartAddress;
        const quint32 projectEnd = projectAddressSpaceRangeEnd(projectRange);
        if (rangeStart < projectEnd && projectStart < rangeEnd)
            return true;
    }

    return false;
}

///
/// \brief filterProjectAddressDescriptions
/// \param descriptions
/// \param ranges
/// \return
///
AddressDescriptionMap filterProjectAddressDescriptions(const AddressDescriptionMap& descriptions,
                                                       const ProjectAddressSpaceRanges& ranges)
{
    AddressDescriptionMap result;
    for (auto it = descriptions.constBegin(); it != descriptions.constEnd(); ++it) {
        if (projectAddressSpaceContains(ranges, it.key()))
            result.insert(it.key(), it.value());
    }

    return result;
}

///
/// \brief filterProjectAddressTimestamps
/// \param timestamps
/// \param ranges
/// \return
///
AddressTimestampMap filterProjectAddressTimestamps(const AddressTimestampMap& timestamps,
                                                   const ProjectAddressSpaceRanges& ranges)
{
    AddressTimestampMap result;
    for (auto it = timestamps.constBegin(); it != timestamps.constEnd(); ++it) {
        if (projectAddressSpaceContains(ranges, it.key()))
            result.insert(it.key(), it.value());
    }

    return result;
}

///
/// \brief filterProjectAddressValues
/// \param values
/// \param ranges
/// \return
///
ProjectAddressSpaceValues filterProjectAddressValues(const ProjectAddressSpaceValues& values,
                                                     const ProjectAddressSpaceRanges& ranges)
{
    ProjectAddressSpaceValues result;
    result.reserve(values.size());
    for (const auto& value : values) {
        if (projectAddressSpaceContains(ranges, value.Key))
            result.append(value);
    }

    return result;
}

///
/// \brief filterProjectAddressSimulations
/// \param simulations
/// \param ranges
/// \return
///
ModbusSimulationMap2 filterProjectAddressSimulations(const ModbusSimulationMap2& simulations,
                                                     const ProjectAddressSpaceRanges& ranges)
{
    ModbusSimulationMap2 result;
    for (auto it = simulations.constBegin(); it != simulations.constEnd(); ++it) {
        const auto& key = it.key();
        const auto& params = it.value();
        const auto simulationLength = static_cast<quint16>(registersCount(params.DataMode));
        const ProjectAddressSpaceRange simulationRange = {
            key.DeviceId,
            key.Type,
            key.Address,
            simulationLength
        };

        if (projectAddressSpaceOverlaps(ranges, simulationRange))
            result.insert(key, params);
    }

    return result;
}

///
/// \brief projectAddressSpaceModifiedRanges
/// \param descriptions
/// \param timestamps
/// \param simulations
/// \return
///
ProjectAddressSpaceRanges projectAddressSpaceModifiedRanges(const AddressDescriptionMap& descriptions,
                                                            const AddressTimestampMap& timestamps,
                                                            const ModbusSimulationMap2& simulations)
{
    ProjectAddressSpaceRanges result;
    result.reserve(descriptions.size() + timestamps.size() + simulations.size());

    for (auto it = descriptions.constBegin(); it != descriptions.constEnd(); ++it) {
        result.append({ it.key().DeviceId, it.key().Type, it.key().Address, 1 });
    }

    for (auto it = timestamps.constBegin(); it != timestamps.constEnd(); ++it) {
        result.append({ it.key().DeviceId, it.key().Type, it.key().Address, 1 });
    }

    for (auto it = simulations.constBegin(); it != simulations.constEnd(); ++it) {
        const auto& key = it.key();
        const auto& params = it.value();
        if (params.Mode == SimulationMode::Off || params.Mode == SimulationMode::Disabled)
            continue;

        result.append({
            key.DeviceId,
            key.Type,
            key.Address,
            static_cast<quint16>(registersCount(params.DataMode))
        });
    }

    return result;
}
