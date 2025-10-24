#include "modbusdataunitmap.h"

///
/// \brief getDataValue
/// \param modbusMap
/// \param pointType
/// \param pointAddress
/// \return
///
quint16 getDataValue(const QModbusDataUnitMap& modbusMap, QModbusDataUnit::RegisterType pointType, quint16 pointAddress)
{
    const auto length = modbusMap[pointType].valueCount();
    const auto startAddress = modbusMap[pointType].startAddress();
    if(pointAddress < startAddress || pointAddress > startAddress + length)
        return 0;
    else
        return modbusMap[pointType].value(pointAddress - startAddress);
}

///
/// \brief setDataValue
/// \param modbusMap
/// \param pointType
/// \param pointAddress
/// \param value
///
void setDataValue(QModbusDataUnitMap& modbusMap, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 value)
{
    const auto startAddress = modbusMap[pointType].startAddress();
    const auto idx = pointAddress - startAddress;
    if(idx >= 0) modbusMap[pointType].setValue(idx, value);
}

///
/// \brief ModbusDataUnitMap::ModbusDataUnitMap
///
ModbusDataUnitMap::ModbusDataUnitMap()
    :_isGlobal(false)
    ,_addressSpace(AddressSpace::Addr6Digits)
{
    _modbusDataUnitGlobalMap.insert(QModbusDataUnit::Coils,            { QModbusDataUnit::Coils,               0, 65535 });
    _modbusDataUnitGlobalMap.insert(QModbusDataUnit::DiscreteInputs,   { QModbusDataUnit::DiscreteInputs,      0, 65535 });
    _modbusDataUnitGlobalMap.insert(QModbusDataUnit::InputRegisters,   { QModbusDataUnit::InputRegisters,      0, 65535 });
    _modbusDataUnitGlobalMap.insert(QModbusDataUnit::HoldingRegisters, { QModbusDataUnit::HoldingRegisters,    0, 65535 });
}

///
/// \brief ModbusDataUnitMap::addUnitMap
/// \param id
/// \param pointType
/// \param pointAddress
/// \param length
///
void ModbusDataUnitMap::addUnitMap(int id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length)
{
    if(_isGlobal)
        return;

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
/// \brief ModbusDataUnitMap::addressSpace
/// \return
///
AddressSpace ModbusDataUnitMap::addressSpace() const
{
    return _addressSpace;
}

///
/// \brief ModbusDataUnitMap::setAddressSpace
/// \param space
///
void ModbusDataUnitMap::setAddressSpace(AddressSpace space)
{
    if(_addressSpace != space)
    {
        _addressSpace = space;

        const int count = _addressSpace == AddressSpace::Addr6Digits ? 65535 : 9999;
        _modbusDataUnitGlobalMap[QModbusDataUnit::Coils].setValueCount(count);
        _modbusDataUnitGlobalMap[QModbusDataUnit::DiscreteInputs].setValueCount(count);
        _modbusDataUnitGlobalMap[QModbusDataUnit::InputRegisters].setValueCount(count);
        _modbusDataUnitGlobalMap[QModbusDataUnit::HoldingRegisters].setValueCount(count);
    }
}

///
/// \brief ModbusDataUnitMap::contains
/// \param pointType
/// \return
///
bool ModbusDataUnitMap::contains(QModbusDataUnit::RegisterType pointType) const
{
    return _isGlobal ? true : _modbusDataUnitMap.contains(pointType);
}

///
/// \brief ModbusDataUnitMap::value
/// \param pointType
/// \return
///
QModbusDataUnit ModbusDataUnitMap::value(QModbusDataUnit::RegisterType pointType) const
{
    return _isGlobal ? _modbusDataUnitGlobalMap[pointType] : _modbusDataUnitMap[pointType];
}

///
/// \brief ModbusDataUnitMap::isGlobalMap
/// \return
///
bool ModbusDataUnitMap::isGlobalMap() const
{
    return _isGlobal;
}

///
/// \brief ModbusDataUnitMap::setGlobalMap
/// \param set
///
void ModbusDataUnitMap::setGlobalMap(bool set)
{
    _isGlobal = set;
}

///
/// \brief ModbusDataUnitMap::begin
/// \return
///
QModbusDataUnitMap::ConstIterator ModbusDataUnitMap::begin()
{
    return _isGlobal ? _modbusDataUnitGlobalMap.begin() : _modbusDataUnitMap.begin();
}

///
/// \brief ModbusDataUnitMap::end
/// \return
///
QModbusDataUnitMap::Iterator ModbusDataUnitMap::end()
{
    return _isGlobal ? _modbusDataUnitGlobalMap.end() : _modbusDataUnitMap.end();
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

    for(uint i = 0; i < length; i++)
    {
        setDataValue(_modbusDataUnitMap, type, addr + i, data.value(i));
        setDataValue(_modbusDataUnitGlobalMap, type, addr + i, data.value(i));
    }
}

///
/// \brief ModbusDataUnitMap::getData
/// \param pointType
/// \param pointAddress
/// \param length
/// \return
///
QModbusDataUnit ModbusDataUnitMap::getData(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const
{
    QModbusDataUnit data(pointType, pointAddress, length);
    for(int i = 0; i < length; i++)
    {
        const auto value = getDataValue(_modbusDataUnitGlobalMap, pointType, pointAddress + i);
        data.setValue(i, value);
    }

    return data;
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

        for(int i = 0; i < length; i++)
        {
            const auto value = getDataValue(_modbusDataUnitGlobalMap, type, startAddress + i);
            setDataValue(modbusMap, type, startAddress + i, value);
        }
    }
    _modbusDataUnitMap = modbusMap;
}
