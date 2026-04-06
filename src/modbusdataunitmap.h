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

    AddressSpace addressSpace() const;
    void setAddressSpace(AddressSpace space);

    bool contains(QModbusDataUnit::RegisterType pointType) const;
    QModbusDataUnit value(QModbusDataUnit::RegisterType pointType) const;

    bool isGlobalMap() const;
    void setGlobalMap(bool set);

    void setData(const QModbusDataUnit& data);
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

private:
    bool _isGlobal;
    AddressSpace _addressSpace;
    QMap<QUuid, QModbusDataUnit> _dataUnits;
    QModbusDataUnitMap _modbusDataUnitMap;
    QModbusDataUnitMap _modbusDataUnitGlobalMap;
    AddressDescriptionMap _descriptions;
    AddressTimestampMap _timestamps;
};


#endif // MODBUSDATAUNITMAP_H

