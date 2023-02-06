#ifndef MODBUSSIMULATIONPARAMS_H
#define MODBUSSIMULATIONPARAMS_H

#include "qrange.h"
#include "enums.h"

///
/// \brief The RandomSimulationParams class
///
struct RandomSimulationParams
{
    QRange<quint16> Range;
};
Q_DECLARE_METATYPE(RandomSimulationParams)

///
/// \brief The IncrementSimulationParams class
///
struct IncrementSimulationParams
{
    quint16 Step = 1;
    QRange<quint16> Range;
};
Q_DECLARE_METATYPE(IncrementSimulationParams)

///
/// \brief The DecrementSimulationParams class
///
struct DecrementSimulationParams
{
    quint16 Step = 1;
    QRange<quint16> Range;
};
Q_DECLARE_METATYPE(DecrementSimulationParams)

///
/// \brief The ModbusSimulationParams class
///
struct ModbusSimulationParams
{
    SimulationMode Mode = SimulationMode::No;
    RandomSimulationParams RandomParams;
    IncrementSimulationParams IncrementParams;
    DecrementSimulationParams DecrementParams;
    quint32 Interval = 1;
};
Q_DECLARE_METATYPE(ModbusSimulationParams)

#endif // MODBUSSIMULATIONPARAMS_H
