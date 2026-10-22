// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modbusdataunitmap.h
/// \brief Declares the modbusdataunitmap interfaces.
///

#ifndef MODBUSDATAUNITMAP_H
#define MODBUSDATAUNITMAP_H

#include <QDateTime>
#include <QModbusDataUnit>
#include <QUuid>
#include "enums.h"
#include "controls/outputtypes.h"

///
/// \brief The ModbusDataUnitMap class
///
class ModbusDataUnitMap
{
public:
    explicit ModbusDataUnitMap();

    bool addUnitMap(QUuid id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length);
    bool removeUnitMap(QUuid id);
    bool unitMap(QUuid id, QModbusDataUnit& unit) const;
    bool ensureRange(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length);

    AddressSpace addressSpace() const;
    void setAddressSpace(AddressSpace space);

    bool contains(QModbusDataUnit::RegisterType pointType) const;
    bool containsRange(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;

    ///
    /// \brief Checks that every address of the range belongs to a registered unit map or an auto-added range,
    /// unlike containsRange() which also accepts the gaps of the merged per-type range.
    /// \return True when the whole range may be served to a Modbus client.
    ///
    bool isMapped(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;

    ///
    /// \brief Resets value, timestamp, description and configured value of every address in the range
    /// that is no longer covered by any unit map.
    /// \return True when at least one address was reset.
    ///
    bool clearUnmappedData(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length);

    QModbusDataUnit value(QModbusDataUnit::RegisterType pointType) const;

    bool isGlobalMap() const;
    void setGlobalMap(bool set);

    void setData(const QModbusDataUnit& data, bool updateUnchangedTimestamps = false);
    QModbusDataUnit getData(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;
    QDateTime timestamp(QModbusDataUnit::RegisterType type, quint16 address) const;
    void setTimestamp(QModbusDataUnit::RegisterType type, quint16 address, const QDateTime& timestamp);
    AddressTimestampMap timestampMap() const;
    AddressTimestampMap timestampMap(QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 length) const;
    QString description(QModbusDataUnit::RegisterType type, quint16 address) const;
    void setDescription(QModbusDataUnit::RegisterType type, quint16 address, const QString& description);
    AddressDescriptionMap descriptionMap() const;
    AddressDescriptionMap descriptionMap(QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 length) const;
    void clearDescriptions();
    void clearTimestamps();
    void setConfiguredValues(const QModbusDataUnit& data);
    AddressValueMap configuredValueMap(QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 length) const;
    void clearConfiguredValues();

    QModbusDataUnitMap::ConstIterator begin();
    QModbusDataUnitMap::Iterator end();


    operator QModbusDataUnitMap(){
        return _isGlobal ? _modbusDataUnitGlobalMap : _modbusDataUnitMap;
    }

    QModbusDataUnit& operator[](QModbusDataUnit::RegisterType pointType) {
        return _isGlobal ? _modbusDataUnitGlobalMap[pointType] : _modbusDataUnitMap[pointType];
    }

private:
    void updateDataUnitMap();
    bool coveredByUnits(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;

private:
    bool _isGlobal;
    AddressSpace _addressSpace;
    QMap<QUuid, QModbusDataUnit> _dataUnits;
    QList<QModbusDataUnit> _autoRanges;
    QModbusDataUnitMap _modbusDataUnitMap;
    QModbusDataUnitMap _modbusDataUnitGlobalMap;
    AddressDescriptionMap _descriptions;
    AddressTimestampMap _timestamps;
    AddressValueMap _configuredValues;
};


#endif // MODBUSDATAUNITMAP_H
