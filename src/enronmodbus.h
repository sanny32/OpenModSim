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

enum class RegisterType { None, Group5x, Group7x };

///
/// \brief Returns the first protocol address of the selected Enron register group.
///
inline quint16 baseAddress(RegisterType type)
{
    return type == RegisterType::Group7x ? FloatBase : LongBase;
}

///
/// \brief Resolves a logical Enron register to its first backing word.
///
inline int wordAddress(RegisterType type, int address)
{
    const int base = baseAddress(type);
    return base + 2 * (address - base);
}

///
/// \brief Resolves a backing word to its logical Enron register.
///
inline int registerAddress(RegisterType type, int address)
{
    const int base = baseAddress(type);
    return base + (address - base) / 2;
}

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
