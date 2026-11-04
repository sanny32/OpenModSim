// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file enronmodbus.h
/// \brief Defines the standard Enron 32-bit register ranges.
///

#ifndef ENRONMODBUS_H
#define ENRONMODBUS_H

#include <QtGlobal>

namespace EnronModbus {

constexpr quint16 LongBase = 5001;
constexpr quint16 FloatBase = 7001;
constexpr quint16 RangeLength = 999;

///
/// \brief Identifies addresses with four-byte register values on the wire.
///
inline bool is32BitAddress(quint16 address)
{
    return (address >= LongBase && address < LongBase + RangeLength)
        || (address >= FloatBase && address < FloatBase + RangeLength);
}

}

#endif // ENRONMODBUS_H
