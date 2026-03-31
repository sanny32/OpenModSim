#ifndef MODBUSDATAUNITMAP_H
#define MODBUSDATAUNITMAP_H

#include <QDateTime>
#include <QHash>
#include <QModbusDataUnit>
#include <QUuid>
#include "enums.h"

///
/// \brief The ModbusDataUnitMap class
///
class ModbusDataUnitMap
{
public:
    explicit ModbusDataUnitMap();

    void addUnitMap(QUuid id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length);
    void removeUnitMap(QUuid id);

    AddressSpace addressSpace() const;
    void setAddressSpace(AddressSpace space);

    bool contains(QModbusDataUnit::RegisterType pointType) const;
    QModbusDataUnit value(QModbusDataUnit::RegisterType pointType) const;

    bool isGlobalMap() const;
    void setGlobalMap(bool set);

    void setData(const QModbusDataUnit& data);
    QModbusDataUnit getData(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;
    QDateTime timestamp(QModbusDataUnit::RegisterType type, quint16 address) const;

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
    // Key: (RegisterType << 16) | address
    QHash<quint32, QDateTime> _timestamps;
};


#endif // MODBUSDATAUNITMAP_H
