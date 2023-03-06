#ifndef MODBUSDATAUNITMAP_H
#define MODBUSDATAUNITMAP_H

#include <QModbusDataUnit>

///
/// \brief The ModbusDataUnitMap class
///
class ModbusDataUnitMap
{
public:
    explicit ModbusDataUnitMap();

    void addUnitMap(int id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length);
    void removeUnitMap(int id);

    void setData(const QModbusDataUnit& data);
    QModbusDataUnit getData(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;

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
    QModbusDataUnitMap _modbusDataUnitGlobalMap;
};


#endif // MODBUSDATAUNITMAP_H
