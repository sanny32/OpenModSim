#include <QDebug>
#include "modbusdataunitmap.h"

///
/// \brief ModbusDataUnitMap::addUnitMap
/// \param id
/// \param pointType
/// \param pointAddress
/// \param length
///
void ModbusDataUnitMap::addUnitMap(int id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length)
{
    _dataUnits.insert(id, {pointType, pointAddress, length});
    updateDataUnitMap();
}

///
/// \brief ModbusDataUnitMap::removeUnitMap
/// \param pointType
///
void ModbusDataUnitMap::removeUnitMap(int id)
{
    _dataUnits.remove(id);
    updateDataUnitMap();
}

///
/// \brief ModbusDataUnitMap::begin
/// \return
///
QModbusDataUnitMap::ConstIterator ModbusDataUnitMap::begin()
{
    return _modbusDataUnitMap.begin();
}

///
/// \brief ModbusDataUnitMap::end
/// \return
///
QModbusDataUnitMap::Iterator ModbusDataUnitMap::end()
{
    return _modbusDataUnitMap.end();
}

///
/// \brief ModbusDataUnitMap::setData
/// \param data
///
void ModbusDataUnitMap::setData(const QModbusDataUnit& data)
{
    const auto addr = data.startAddress();
    const auto length = data.valueCount();
    const auto type = data.registerType();

    const auto idx = addr - _modbusDataUnitMap[type].startAddress();
    for(int i = 0; i < length; i++)
    {
        _modbusDataUnitMap[type].setValue(idx + i, data.value(i));
    }
}

///
/// \brief ModbusDataUnitMap::updateDataUnitMap
///
void ModbusDataUnitMap::updateDataUnitMap()
{
    QMultiMap<QModbusDataUnit::RegisterType, QModbusDataUnit> multimap;
    for(auto&& unit : _dataUnits)
    {
        multimap.insert(unit.registerType(), unit);
    }

    QModbusDataUnitMap modbusMap;
    for(auto&& type: multimap.uniqueKeys())
    {
        quint16 startAddress = 65535;
        quint16 endAddress = 0;
        for(auto&& unit : multimap.values(type))
        {
            startAddress = qMin<quint16>(startAddress, unit.startAddress());
            endAddress = qMax<quint16>(endAddress, unit.startAddress() + unit.valueCount());
        }

        const quint16 length = endAddress - startAddress;
        if(length > 0)
            modbusMap.insert(type, {type, startAddress, length});

        const auto idx = qAbs(_modbusDataUnitMap[type].startAddress() - startAddress);
        for(int i = 0; i < length; i++)
        {
            modbusMap[type].setValue(i, _modbusDataUnitMap[type].value(i));
        }
    }
    _modbusDataUnitMap = modbusMap;
}
