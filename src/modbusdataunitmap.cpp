#include "modbusdataunitmap.h"

static bool unitMapsEqual(const QModbusDataUnitMap& a, const QModbusDataUnitMap& b)
{
    if (a.keys() != b.keys())
        return false;
    for (auto it = a.constBegin(); it != a.constEnd(); ++it) {
        const auto& u = it.value();
        const auto& v = b[it.key()];
        if (u.startAddress() != v.startAddress() || u.valueCount() != v.valueCount())
            return false;
    }
    return true;
}

static const QDateTime kApplicationStartTimestamp = QDateTime::currentDateTime();

static bool unitMapContainsAddress(const QModbusDataUnitMap& map, QModbusDataUnit::RegisterType type, quint16 address)
{
    const auto it = map.constFind(type);
    if (it == map.constEnd())
        return false;

    const auto& unit = it.value();
    const quint32 start = unit.startAddress();
    const quint32 end = start + unit.valueCount();
    const quint32 point = address;
    return point >= start && point < end;
}

///
/// \brief getDataValue
/// \param modbusMap
/// \param pointType
/// \param pointAddress
/// \return
///
quint16 getDataValue(const QModbusDataUnitMap& modbusMap, QModbusDataUnit::RegisterType pointType, quint16 pointAddress)
{
    const auto it = modbusMap.constFind(pointType);
    if(it == modbusMap.constEnd())
        return 0;

    const auto& dataUnit = it.value();
    const int idx = pointAddress - dataUnit.startAddress();
    if(idx < 0 || idx >= dataUnit.valueCount())
        return 0;

    return dataUnit.value(idx);
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
    auto it = modbusMap.find(pointType);
    if(it == modbusMap.end())
        return;

    auto& dataUnit = it.value();
    const int idx = pointAddress - dataUnit.startAddress();
    if(idx >= 0 && idx < dataUnit.valueCount())
        dataUnit.setValue(idx, value);
}

ItemMapKey makeLocalKey(QModbusDataUnit::RegisterType type, quint16 address)
{
    return {0, type, address};
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
bool ModbusDataUnitMap::addUnitMap(QUuid id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length)
{
    const auto before = _modbusDataUnitMap;
    _dataUnits.insert(id, {pointType, pointAddress, length});
    updateDataUnitMap();
    return !unitMapsEqual(_modbusDataUnitMap, before);
}

///
/// \brief ModbusDataUnitMap::removeUnitMap
/// \param pointType
///
bool ModbusDataUnitMap::removeUnitMap(QUuid id)
{
    const auto before = _modbusDataUnitMap;
    _dataUnits.remove(id);
    updateDataUnitMap();
    return !unitMapsEqual(_modbusDataUnitMap, before);
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
    const QDateTime now = QDateTime::currentDateTime();

    for(uint i = 0; i < length; i++)
    {
        const quint16 newValue = data.value(i);
        const quint16 oldValue = getDataValue(_modbusDataUnitGlobalMap, type, addr + i);
        if(newValue != oldValue)
            setTimestamp(type, static_cast<quint16>(addr + i), now);

        setDataValue(_modbusDataUnitMap, type, addr + i, newValue);
        setDataValue(_modbusDataUnitGlobalMap, type, addr + i, newValue);
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
/// \brief ModbusDataUnitMap::timestamp
/// \param type
/// \param address
/// \return
///
QDateTime ModbusDataUnitMap::timestamp(QModbusDataUnit::RegisterType type, quint16 address) const
{
    const auto key = makeLocalKey(type, address);
    const auto it = _timestamps.constFind(key);
    if (it != _timestamps.constEnd())
        return it.value();

    return unitMapContainsAddress(_modbusDataUnitMap, type, address)
        ? kApplicationStartTimestamp
        : QDateTime();
}

///
/// \brief ModbusDataUnitMap::setTimestamp
/// \param type
/// \param address
/// \param timestamp
///
void ModbusDataUnitMap::setTimestamp(QModbusDataUnit::RegisterType type, quint16 address, const QDateTime& timestamp)
{
    const auto key = makeLocalKey(type, address);
    if (timestamp.isValid())
        _timestamps.insert(key, timestamp);
    else
        _timestamps.remove(key);
}

///
/// \brief ModbusDataUnitMap::timestampMap
/// \return
///
AddressTimestampMap ModbusDataUnitMap::timestampMap() const
{
    return _timestamps;
}

///
/// \brief ModbusDataUnitMap::timestampMap
/// \param type
/// \param startAddress
/// \param length
/// \return
///
AddressTimestampMap ModbusDataUnitMap::timestampMap(QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 length) const
{
    AddressTimestampMap result;
    if(length == 0)
        return result;

    const quint32 endAddress = static_cast<quint32>(startAddress) + length;
    for (auto it = _timestamps.constBegin(); it != _timestamps.constEnd(); ++it) {
        if (it.key().Type != type)
            continue;

        const auto address = it.key().Address;
        if (address < startAddress || static_cast<quint32>(address) >= endAddress)
            continue;

        if (it.value().isValid())
            result.insert(it.key(), it.value());
    }

    return result;
}

///
/// \brief ModbusDataUnitMap::description
/// \param type
/// \param address
/// \return
///
QString ModbusDataUnitMap::description(QModbusDataUnit::RegisterType type, quint16 address) const
{
    return _descriptions.value(makeLocalKey(type, address));
}

///
/// \brief ModbusDataUnitMap::setDescription
/// \param type
/// \param address
/// \param description
///
void ModbusDataUnitMap::setDescription(QModbusDataUnit::RegisterType type, quint16 address, const QString& description)
{
    const auto key = makeLocalKey(type, address);
    if(description.isEmpty())
        _descriptions.remove(key);
    else
        _descriptions.insert(key, description);
}

///
/// \brief ModbusDataUnitMap::descriptionMap
/// \return
///
AddressDescriptionMap ModbusDataUnitMap::descriptionMap() const
{
    return _descriptions;
}

///
/// \brief ModbusDataUnitMap::descriptionMap
/// \param type
/// \param startAddress
/// \param length
/// \return
///
AddressDescriptionMap ModbusDataUnitMap::descriptionMap(QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 length) const
{
    AddressDescriptionMap result;
    if(length == 0)
        return result;

    const quint32 endAddress = static_cast<quint32>(startAddress) + length;
    for(auto it = _descriptions.constBegin(); it != _descriptions.constEnd(); ++it)
    {
        if(it.key().Type != type)
            continue;

        const auto addr = it.key().Address;
        if(addr < startAddress || static_cast<quint32>(addr) >= endAddress)
            continue;

        result.insert(it.key(), it.value());
    }

    return result;
}

///
/// \brief ModbusDataUnitMap::clearDescriptions
///
void ModbusDataUnitMap::clearDescriptions()
{
    _descriptions.clear();
}

///
/// \brief ModbusDataUnitMap::clearTimestamps
///
void ModbusDataUnitMap::clearTimestamps()
{
    _timestamps.clear();
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
