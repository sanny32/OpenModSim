#ifndef MODBUSWRITEPARAMS_H
#define MODBUSWRITEPARAMS_H

#include <QVariant>
#include "enums.h"

///
/// \brief The ModbusWriteParams class
///
struct ModbusWriteParams
{
    quint32 Address;
    QVariant Value;
    DataDisplayMode DisplayMode;
};
Q_DECLARE_METATYPE(ModbusWriteParams)

///
/// \brief The ModbusMaskWriteParams class
///
struct ModbusMaskWriteParams
{
    quint32 Address;
    quint16 AndMask;
    quint16 OrMask;
};
Q_DECLARE_METATYPE(ModbusMaskWriteParams)

#endif // MODBUSWRITEPARAMS_H
