// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file legacyprojectparser.h
/// \brief Declares helpers for parsing legacy project file data.
///

#ifndef LEGACYPROJECTPARSER_H
#define LEGACYPROJECTPARSER_H

#include <QHash>
#include <QString>
#include <QVector>

#include "displaydefinition.h"
#include "modbussimulationparams.h"
#include "outputtypes.h"

class QXmlStreamReader;

///
/// \brief The LegacyDataViewState struct stores parsed legacy data view data.
///
struct LegacyDataViewState
{
    DataType DataTypeValue = DataType::UInt16;
    RegisterOrder RegisterOrderValue = RegisterOrder::MSRF;
    DataViewDefinitions Definitions;
    QHash<quint16, quint16> Data;
    QHash<quint16, ModbusSimulationParams> Simulations;
    AddressDescriptionMap Descriptions;
    AddressColorMap Colors;
};

///
/// \brief The LegacyProjectParser class reads legacy project data without UI dependencies.
///
class LegacyProjectParser
{
public:
    static bool isDataViewElement(const QString& name) noexcept;
    static LegacyDataViewState readDataViewState(QXmlStreamReader& xml);
    static QHash<quint16, ModbusSimulationParams> readSimulationMap(QXmlStreamReader& xml);
    static QHash<quint16, quint16> readDataUnit(QXmlStreamReader& xml);
    static AddressColorMap readAddressColors(QXmlStreamReader& xml);
    static QVector<quint16> dataUnitValues(const DataViewDefinitions& definitions,
                                           const QHash<quint16, quint16>& data);
};

#endif // LEGACYPROJECTPARSER_H
