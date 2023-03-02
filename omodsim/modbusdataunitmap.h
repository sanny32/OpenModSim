#ifndef MODBUSDATAUNITMAP_H
#define MODBUSDATAUNITMAP_H

#include <QModbusDataUnit>

///
/// \brief The ModbusDataUnitMap class
///
class ModbusDataUnitMap
{
public:
    explicit ModbusDataUnitMap() = default;

    void addUnitMap(int id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length);
    void removeUnitMap(int id);

    void setData(const QModbusDataUnit& data);

    QModbusDataUnitMap::ConstIterator begin();
    QModbusDataUnitMap::Iterator end();

    operator QModbusDataUnitMap(){
        return _modbusDataUnitMap;
    }

private:
    void updateDataUnitMap();

private:
    QMap<int, QModbusDataUnit> _dataUnits;
    QModbusDataUnitMap _modbusDataUnitMap;
};


#endif // MODBUSDATAUNITMAP_H
