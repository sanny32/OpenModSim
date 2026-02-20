#ifndef MODBUSWRITEPARAMS_H
#define MODBUSWRITEPARAMS_H

#include <QVariant>
#include "enums.h"

///
/// \brief The ModbusWriteParams class
///
struct ModbusWriteParams
{
    quint16 Address;
    QVariant Value;
    DataDisplayMode DisplayMode;
    ByteOrder Order;
    QString Codepage;
    bool ZeroBasedAddress;
    AddressSpace AddrSpace;
};
Q_DECLARE_METATYPE(ModbusWriteParams)

///
/// \brief The ModbusMaskWriteParams class
///
struct ModbusMaskWriteParams
{
    quint16 Address;
    quint16 AndMask;
    quint16 OrMask;
    bool ZeroBasedAddress;
    AddressSpace AddrSpace;
};
Q_DECLARE_METATYPE(ModbusMaskWriteParams)

#endif // MODBUSWRITEPARAMS_H
