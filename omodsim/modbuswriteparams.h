#ifndef MODBUSWRITEPARAMS_H
#define MODBUSWRITEPARAMS_H

#include <QVariant>
#include "modbussimulationparams.h"

///
/// \brief The ModbusWriteParams class
///
struct ModbusWriteParams
{
    quint32 Address;
    QVariant Value;
    DataDisplayMode DisplayMode;
    ModbusSimulationParams SimulationParams;
};

///
/// \brief The ModbusMaskWriteParams class
///
struct ModbusMaskWriteParams
{
    quint32 Address;
    quint16 AndMask;
    quint16 OrMask;
};

#endif // MODBUSWRITEPARAMS_H
