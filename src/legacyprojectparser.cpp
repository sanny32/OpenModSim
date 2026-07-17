// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file legacyprojectparser.cpp
/// \brief Implements legacy project file data parsing helpers.
///

#include <QModbusDataUnit>
#include <QXmlStreamReader>

#include "formatutils.h"
#include "legacyprojectparser.h"

namespace {
///
/// \brief readDataViewDefinitions reads current or legacy data view definitions.
/// \param xml XML reader positioned on the definitions element.
/// \param formTitle Fallback form title.
/// \return Parsed data view definitions.
///
DataViewDefinitions readDataViewDefinitions(QXmlStreamReader& xml, const QString& formTitle)
{
    DataViewDefinitions dd;
    const auto attributes = xml.attributes();

    if (attributes.hasAttribute("FormName")) {
        dd.FormName = attributes.value("FormName").toString();
    }

    if (attributes.hasAttribute("DeviceId")) {
        bool ok;
        const quint8 deviceId = attributes.value("DeviceId").toUShort(&ok);
        if (ok)
            dd.DeviceId = deviceId;
    }

    if (attributes.hasAttribute("PointType")) {
        dd.PointType = enumFromString<QModbusDataUnit::RegisterType>(attributes.value("PointType").toString());
    }

    if (attributes.hasAttribute("PointAddress")) {
        bool ok;
        const quint16 pointAddress = attributes.value("PointAddress").toUShort(&ok);
        if (ok)
            dd.PointAddress = pointAddress;
    }

    if (attributes.hasAttribute("Length")) {
        bool ok;
        const quint16 length = attributes.value("Length").toUShort(&ok);
        if (ok)
            dd.Length = length;
    }

    if (attributes.hasAttribute("DataViewColumnsDistance")) {
        bool ok;
        const quint16 distance = attributes.value("DataViewColumnsDistance").toUShort(&ok);
        if (ok)
            dd.DataViewColumnsDistance = distance;
    }

    if (attributes.hasAttribute("LeadingZeros")) {
        dd.LeadingZeros = stringToBool(attributes.value("LeadingZeros").toString());
    }

    xml.skipCurrentElement();

    if (dd.FormName.isEmpty() && !formTitle.isEmpty())
        dd.FormName = formTitle;

    dd.normalize();

    return dd;
}
}

///
/// \brief LegacyProjectParser::isDataViewElement
/// \param name Element name to test.
/// \return true for legacy data view elements.
///
bool LegacyProjectParser::isDataViewElement(const QString& name) noexcept
{
    return name == QLatin1String("FormModSim");
}

///
/// \brief LegacyProjectParser::readDataViewState
/// \param xml XML reader positioned on the legacy data view element.
/// \return Parsed legacy data view state.
///
LegacyDataViewState LegacyProjectParser::readDataViewState(QXmlStreamReader& xml)
{
    LegacyDataViewState state;
    if (!isDataViewElement(xml.name().toString())) {
        xml.skipCurrentElement();
        return state;
    }

    const QXmlStreamAttributes attributes = xml.attributes();
    const QString formTitle = attributes.value("Title").toString();

    if (attributes.hasAttribute("DataType")) {
        state.DataTypeValue = enumFromString<DataType>(attributes.value("DataType").toString(), DataType::UInt16);
    }

    if (attributes.hasAttribute("DataDisplayMode")) {
        state.DataTypeValue = enumFromString<DataType>(attributes.value("DataDisplayMode").toString(), DataType::UInt16);
    }

    if (attributes.hasAttribute("RegisterOrder")) {
        state.RegisterOrderValue = enumFromString<RegisterOrder>(attributes.value("RegisterOrder").toString(),
                                                                 RegisterOrder::MSRF);
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("DataViewDefinitions") ||
            xml.name() == QLatin1String("DisplayDefinition")) {
            state.Definitions = readDataViewDefinitions(xml, formTitle);
        }
        else if (xml.name() == QLatin1String("ModbusSimulationMap")) {
            state.Simulations = readSimulationMap(xml);
        }
        else if (xml.name() == QLatin1String("AddressDescriptionMap")) {
            xml >> state.Descriptions;
        }
        else if (xml.name() == QLatin1String("AddressColorMap")) {
            state.Colors = readAddressColors(xml);
        }
        else if (xml.name() == QLatin1String("ModbusDataUnit")) {
            state.Data = readDataUnit(xml);
        }
        else {
            xml.skipCurrentElement();
        }
    }

    return state;
}

///
/// \brief LegacyProjectParser::readSimulationMap
/// \param xml XML reader positioned on the ModbusSimulationMap element.
/// \return Parsed simulation map keyed by legacy address.
///
QHash<quint16, ModbusSimulationParams> LegacyProjectParser::readSimulationMap(QXmlStreamReader& xml)
{
    QHash<quint16, ModbusSimulationParams> simulations;

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("Simulation")) {
            const QXmlStreamAttributes attributes = xml.attributes();
            bool ok;
            const quint16 address = attributes.value("Address").toUShort(&ok);

            ModbusSimulationParams params;
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("ModbusSimulationParams")) {
                    xml >> params;
                } else {
                    xml.skipCurrentElement();
                }
            }

            if(ok) {
                simulations[address] = params;
            }
        } else {
            xml.skipCurrentElement();
        }
    }

    return simulations;
}

///
/// \brief LegacyProjectParser::readDataUnit
/// \param xml XML reader positioned on the ModbusDataUnit element.
/// \return Parsed values keyed by legacy address.
///
QHash<quint16, quint16> LegacyProjectParser::readDataUnit(QXmlStreamReader& xml)
{
    QHash<quint16, quint16> data;

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("Value")) {
            QXmlStreamAttributes attributes = xml.attributes();
            bool ok;
            const quint16 address = attributes.value("Address").toUShort(&ok);
            if(ok) {
                const quint16 value = xml.readElementText().toUShort(&ok);
                if (ok) {
                    data[address] = value;
                }
            }
        } else {
            xml.skipCurrentElement();
        }
    }

    return data;
}

///
/// \brief LegacyProjectParser::readAddressColors
/// \param xml XML reader positioned on the AddressColorMap element.
/// \return Parsed color map with fallback keys for old files.
///
AddressColorMap LegacyProjectParser::readAddressColors(QXmlStreamReader& xml)
{
    AddressColorMap map;

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("Color")) {
            const auto attributes = xml.attributes();
            bool ok;
            auto deviceId = static_cast<quint8>(attributes.value("DeviceId").toUShort(&ok));
            if (!ok)
                deviceId = 0;

            auto type = static_cast<QModbusDataUnit::RegisterType>(attributes.value("Type").toInt(&ok));
            if (!ok)
                type = QModbusDataUnit::RegisterType::Invalid;

            const auto address = attributes.value("Address").toUShort(&ok);
            if (ok) {
                const auto value = attributes.value("Value").toString();
                if (!value.isEmpty())
                    map.insert({ deviceId, type, address }, value);
            }
        }

        xml.skipCurrentElement();
    }

    return map;
}

///
/// \brief LegacyProjectParser::dataUnitValues
/// \param definitions Legacy data view definitions.
/// \param data Legacy values keyed by address.
/// \return Values aligned to the definitions range.
///
QVector<quint16> LegacyProjectParser::dataUnitValues(const DataViewDefinitions& definitions,
                                                     const QHash<quint16, quint16>& data)
{
    QVector<quint16> values(definitions.Length);

    QHashIterator it(data);
    while(it.hasNext()) {
        const auto item = it.next();
        const auto index = item.key() - definitions.PointAddress;
        if (index < 0 || index >= definitions.Length) {
            continue;
        }

        switch(definitions.PointType) {
            case QModbusDataUnit::Coils:
            case QModbusDataUnit::DiscreteInputs:
                values[index] = qBound<quint16>(0, item.value(), 1);
                break;
            case QModbusDataUnit::InputRegisters:
            case QModbusDataUnit::HoldingRegisters:
                values[index] = item.value();
                break;
            default: break;
        }
    }

    return values;
}
