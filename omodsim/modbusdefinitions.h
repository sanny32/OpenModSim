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

    void normalize() {
        AddrSpace = qBound(AddressSpace::Addr6Digits, AddrSpace, AddressSpace::Addr5Digits);
    }
};
Q_DECLARE_METATYPE(ModbusDefinitions)

///
/// \brief operator <<
/// \param out
/// \param defs
/// \return
///
inline QSettings& operator <<(QSettings& out, const ModbusDefinitions& defs)
{
    out.beginGroup("ModbusDefinitions");

    out << defs.ErrorSimulations;
    out << defs.AddrSpace;
    out.setValue("UseGlobalUnitMap", defs.UseGlobalUnitMap);

    out.endGroup();

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param defs
/// \return
///
inline QSettings& operator >>(QSettings& in, ModbusDefinitions& defs)
{
    in.beginGroup("ModbusDefinitions");

    in >> defs.ErrorSimulations;
    in >> defs.AddrSpace;
    defs.UseGlobalUnitMap = in.value("UseGlobalUnitMap", false).toBool();

    in.endGroup();

    defs.normalize();
    return in;
}

#endif // MODBUSDEFINITIONS_H
