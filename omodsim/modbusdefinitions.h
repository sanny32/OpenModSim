#ifndef MODBUSDEFINITIONS_H
#define MODBUSDEFINITIONS_H

#include "enums.h"
#include "modbuserrorsimulations.h"

///
/// \brief The ModbusDefinitions class
///
struct ModbusDefinitions
{
    AddressSpace AddrSpace = AddressSpace::Addr6Digits;
    bool UseGlobalUnitMap = false;
    ModbusErrorSimulations ErrorSimulations;
};

#endif // MODBUSDEFINITIONS_H
