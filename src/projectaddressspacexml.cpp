// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectaddressspacexml.cpp
/// \brief Implements the project AddressSpace element XML reader, writer and applier.
///

#include <algorithm>

#include "datasimulator.h"
#include "modbusmultiserver.h"
#include "projectaddressspacexml.h"

namespace {

///
/// \brief containsProjectAddressSpaceValue
/// \param values
/// \param key
/// \return
///
bool containsProjectAddressSpaceValue(const ProjectAddressSpaceValues& values, const ItemMapKey& key)
{
    return std::any_of(values.constBegin(), values.constEnd(), [&key](const ProjectAddressSpaceValue& value) {
        return value.Key.DeviceId == key.DeviceId &&
               value.Key.Type == key.Type &&
               value.Key.Address == key.Address;
    });
}

///
/// \brief appendProjectAddressSpaceValues
/// \param values
/// \param mbServer
/// \param range
///
void appendProjectAddressSpaceValues(ProjectAddressSpaceValues& values,
                                     ModbusMultiServer& mbServer,
                                     const ProjectAddressSpaceRange& range,
                                     bool runtimeValues)
{
    if (range.Length == 0)
        return;

    if (!runtimeValues) {
        const auto configured = mbServer.configuredValueMap(range.DeviceId, range.Type, range.StartAddress, range.Length);
        for (auto it = configured.constBegin(); it != configured.constEnd(); ++it) {
            if (it.value() != 0 && !containsProjectAddressSpaceValue(values, it.key()))
                values.append({ it.key(), it.value() });
        }
        return;
    }

    const auto unit = mbServer.data(range.DeviceId, range.Type, range.StartAddress, range.Length);
    quint16 address = range.StartAddress;
    for (const auto value : unit.values()) {
        const ItemMapKey key = { range.DeviceId, range.Type, address };
        if (value != 0 && !containsProjectAddressSpaceValue(values, key))
            values.append({ key, value });
        address++;
    }
}

///
/// \brief appendProjectAddressSpaceMetadata
/// \param descriptions
/// \param timestamps
/// \param mbServer
/// \param range
///
void appendProjectAddressSpaceMetadata(AddressDescriptionMap& descriptions,
                                       AddressTimestampMap& timestamps,
                                       ModbusMultiServer& mbServer,
                                       const ProjectAddressSpaceRange& range)
{
    const auto rangeDescriptions = mbServer.descriptionMap(range.DeviceId, range.Type, range.StartAddress, range.Length);
    for (auto it = rangeDescriptions.constBegin(); it != rangeDescriptions.constEnd(); ++it)
        descriptions.insert(it.key(), it.value());

    const auto rangeTimestamps = mbServer.timestampMap(range.DeviceId, range.Type, range.StartAddress, range.Length);
    for (auto it = rangeTimestamps.constBegin(); it != rangeTimestamps.constEnd(); ++it)
        timestamps.insert(it.key(), it.value());
}

}

///
/// \brief readProjectAddressSpace reads the AddressSpace element the reader is
/// positioned on, appending parsed entries to the payload and skipping malformed ones.
/// \param xml The reader positioned on the AddressSpace start element.
/// \param payload The payload the parsed entries are appended to.
///
void readProjectAddressSpace(QXmlStreamReader& xml, ProjectAddressSpacePayload& payload)
{
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("AddressDescriptionMap")) {
            payload.HasDescriptions = true;
            xml >> payload.Descriptions;
        }
        else if (xml.name() == QLatin1String("AddressTimestampMap")) {
            payload.HasTimestamps = true;
            xml >> payload.Timestamps;
        }
        else if (xml.name() == QLatin1String("ModbusSimulationMap")) {
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("Simulation")) {
                    const auto attrs = xml.attributes();
                    bool ok;
                    const quint8 deviceId = static_cast<quint8>(attrs.value("DeviceId").toUShort(&ok));
                    if (ok) {
                        const auto type = static_cast<QModbusDataUnit::RegisterType>(attrs.value("Type").toInt(&ok));
                        if (ok) {
                            const quint16 addr = attrs.value("Address").toUShort(&ok);
                            if (ok && xml.readNextStartElement()) {
                                ModbusSimulationParams params;
                                xml >> params;
                                payload.Simulations.append({deviceId, type, addr, params});
                            }
                        }
                    }
                    xml.skipCurrentElement();
                } else {
                    xml.skipCurrentElement();
                }
            }
        }
        else if (xml.name() == QLatin1String("ModbusDataValues")) {
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("Value")) {
                    const auto attrs = xml.attributes();
                    bool ok;
                    const quint8 deviceId = static_cast<quint8>(attrs.value("DeviceId").toUShort(&ok));
                    if (ok) {
                        const auto type = static_cast<QModbusDataUnit::RegisterType>(attrs.value("Type").toInt(&ok));
                        if (ok) {
                            const quint16 address = attrs.value("Address").toUShort(&ok);
                            if (ok) {
                                const quint16 value = xml.readElementText().toUShort(&ok);
                                if (ok) payload.Values.append({deviceId, type, address, value});
                                continue;
                            }
                        }
                    }
                    xml.skipCurrentElement();
                } else {
                    xml.skipCurrentElement();
                }
            }
        }
        else {
            xml.skipCurrentElement();
        }
    }
}

///
/// \brief applyProjectAddressSpace applies the parsed values, simulations and
/// metadata to the runtime server and simulator.
/// \param payload The parsed AddressSpace payload.
/// \param mbServer The server the values and metadata are applied to.
/// \param dataSimulator The simulator the simulations are started on.
/// \param replace True to replace existing metadata, false to merge into it.
///
void applyProjectAddressSpace(const ProjectAddressSpacePayload& payload,
                              ModbusMultiServer& mbServer,
                              DataSimulator* dataSimulator,
                              bool replace)
{
    // Values are grouped into contiguous runs so a large project needs one setData
    // per run instead of one per register (issue #126).
    QMap<QPair<quint8, QModbusDataUnit::RegisterType>, QMap<quint16, quint16>> valuesByUnit;
    for (const auto& pv : payload.Values)
        valuesByUnit[{pv.DeviceId, pv.Type}][pv.Address] = pv.RegisterValue;

    for (auto it = valuesByUnit.constBegin(); it != valuesByUnit.constEnd(); ++it) {
        const quint8 deviceId = it.key().first;
        const auto type = it.key().second;
        const auto& values = it.value();

        for (auto jt = values.constBegin(); jt != values.constEnd(); ) {
            const quint16 startAddress = jt.key();
            QVector<quint16> run;

            quint16 expected = startAddress;
            while (jt != values.constEnd() && jt.key() == expected) {
                run.append(jt.value());
                ++jt;
                if (expected == 0xFFFF)
                    break;
                ++expected;
            }

            QModbusDataUnit range(type, startAddress, static_cast<quint16>(run.size()));
            range.setValues(run);
            mbServer.setData(deviceId, range, WriteSource::ProjectLoad);
        }
    }

    // Apply simulations after initial values are restored.
    if (dataSimulator) {
        for (const auto& ps : payload.Simulations)
            dataSimulator->startSimulation(ps.DeviceId, ps.Type, ps.Address, ps.Params);
    }

    if (payload.HasDescriptions)
        mbServer.setDescriptionMap(payload.Descriptions, WriteSource::ProjectLoad, replace);
    if (payload.HasTimestamps)
        mbServer.setTimestampMap(payload.Timestamps, replace);
}

///
/// \brief writeProjectAddressSpace writes the AddressSpace element covering the
/// given ranges: descriptions, timestamps, active simulations and non-zero values.
/// \param w The writer the element is written to.
/// \param mbServer The server the values and metadata are read from.
/// \param simulations All currently configured simulations.
/// \param ranges The address ranges to serialize.
///
void writeProjectAddressSpace(QXmlStreamWriter& w,
                              ModbusMultiServer& mbServer,
                              const ModbusSimulationMap2& simulations,
                              const ProjectAddressSpaceRanges& ranges,
                              const ProjectAddressSpaceWriteOptions& options)
{
    AddressDescriptionMap projectDescriptionMap;
    AddressTimestampMap projectTimestampMap;
    ProjectAddressSpaceValues projectValues;
    for (const auto& range : ranges) {
        appendProjectAddressSpaceMetadata(projectDescriptionMap, projectTimestampMap, mbServer, range);
        appendProjectAddressSpaceValues(projectValues, mbServer, range, options.SaveRuntimeValues);
    }

    const auto projectSimulationMap = filterProjectAddressSimulations(simulations, ranges);

    w.writeStartElement("AddressSpace");

    w << filterProjectAddressDescriptions(projectDescriptionMap, ranges);
    if (options.SaveTimestamps)
        w << filterProjectAddressTimestamps(projectTimestampMap, ranges);

    {
        w.writeStartElement("ModbusSimulationMap");
        for (auto it = projectSimulationMap.constBegin(); it != projectSimulationMap.constEnd(); ++it) {
            const auto& key = it.key();
            const auto& params = it.value();
            if (params.Mode != SimulationMode::Off && params.Mode != SimulationMode::Disabled) {
                w.writeStartElement("Simulation");
                w.writeAttribute("DeviceId", QString::number(key.DeviceId));
                w.writeAttribute("Type", QString::number(key.Type));
                w.writeAttribute("Address", QString::number(key.Address));
                w << params;
                w.writeEndElement(); // Simulation
            }
        }
        w.writeEndElement(); // ModbusSimulationMap
    }

    {
        const auto values = filterProjectAddressValues(projectValues, ranges);
        w.writeStartElement("ModbusDataValues");
        for (const auto& value : values) {
            w.writeStartElement("Value");
            w.writeAttribute("DeviceId", QString::number(value.Key.DeviceId));
            w.writeAttribute("Type", QString::number(value.Key.Type));
            w.writeAttribute("Address", QString::number(value.Key.Address));
            w.writeCharacters(QString::number(value.Value));
            w.writeEndElement(); // Value
        }
        w.writeEndElement(); // ModbusDataValues
    }

    w.writeEndElement(); // AddressSpace
}
