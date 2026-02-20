#ifndef MODBUSLIMITS_H
#define MODBUSLIMITS_H

#include "enums.h"
#include "qrange.h"

class ModbusLimits final
{
public:
    static QRange<int> addressRange(AddressSpace space, bool zeroBased = false) {
        switch(space) {
        case AddressSpace::Addr5Digits:
            return zeroBased ? QRange<int>(0, 9998) : QRange<int>(1, 9999);
        case AddressSpace::Addr6Digits:
            return zeroBased ? QRange<int>(0, 65535) : QRange<int>(1, 65536);
        }
        return zeroBased ? QRange<int>(0, 65535) : QRange<int>(1, 65536);
    }
    static QRange<int> lengthRange()   { return { 1, 200   }; }
    static QRange<int> slaveRange()    { return { 1, 255   }; }
};

#endif // MODBUSLIMITS_H
