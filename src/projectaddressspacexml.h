// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectaddressspacexml.h
/// \brief Declares the project AddressSpace element XML reader, writer and applier.
///

#ifndef PROJECTADDRESSSPACEXML_H
#define PROJECTADDRESSSPACEXML_H

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include "modbussimulationparams.h"
#include "projectaddressspacefilter.h"

class ModbusMultiServer;
class DataSimulator;

///
/// \brief The ProjectAddressSpacePayload struct holds the parsed content of the
/// project AddressSpace element.
///
struct ProjectAddressSpacePayload
{
    struct Value {
        quint8 DeviceId = 0;
        QModbusDataUnit::RegisterType Type = QModbusDataUnit::Invalid;
        quint16 Address = 0;
        quint16 RegisterValue = 0;
    };
    struct Simulation {
        quint8 DeviceId = 0;
        QModbusDataUnit::RegisterType Type = QModbusDataUnit::Invalid;
        quint16 Address = 0;
        ModbusSimulationParams Params;
    };

    AddressDescriptionMap Descriptions;
    bool HasDescriptions = false;
    AddressTimestampMap Timestamps;
    bool HasTimestamps = false;
    QVector<Value> Values;
    QVector<Simulation> Simulations;
};

///
/// \brief readProjectAddressSpace reads the AddressSpace element the reader is
/// positioned on, appending parsed entries to the payload and skipping malformed ones.
/// \param xml The reader positioned on the AddressSpace start element.
/// \param payload The payload the parsed entries are appended to.
///
void readProjectAddressSpace(QXmlStreamReader& xml, ProjectAddressSpacePayload& payload);

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
                              bool replace);

///
/// \brief The ProjectAddressSpaceWriteOptions struct selects which parts of the runtime
/// state reach the project file.
///
struct ProjectAddressSpaceWriteOptions
{
    /// Write the AddressTimestampMap element. Timestamps carry millisecond precision and
    /// change on every client write, which makes projects noisy under version control.
    bool SaveTimestamps = true;

    /// Write the live register values. When false the values configured by the project
    /// and by the user are written instead, so a running simulation leaves the file alone.
    bool SaveRuntimeValues = true;
};

///
/// \brief writeProjectAddressSpace writes the AddressSpace element covering the
/// given ranges: descriptions, timestamps, active simulations and non-zero values.
/// \param w The writer the element is written to.
/// \param mbServer The server the values and metadata are read from.
/// \param simulations All currently configured simulations.
/// \param ranges The address ranges to serialize.
/// \param options Selects which runtime state is written.
///
void writeProjectAddressSpace(QXmlStreamWriter& w,
                              ModbusMultiServer& mbServer,
                              const ModbusSimulationMap2& simulations,
                              const ProjectAddressSpaceRanges& ranges,
                              const ProjectAddressSpaceWriteOptions& options = {});

#endif // PROJECTADDRESSSPACEXML_H
