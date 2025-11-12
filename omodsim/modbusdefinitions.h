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

///
/// \brief operator <<
/// \param xml
/// \param definitions
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const ModbusDefinitions& definitions)
{
    xml.writeStartElement("ModbusDefinitions");

    xml.writeAttribute("AddrSpace", enumToString(definitions.AddrSpace));
    xml.writeAttribute("UseGlobalUnitMap", boolToString(definitions.UseGlobalUnitMap));

    xml << definitions.ErrorSimulations;

    xml.writeEndElement();
    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param definitions
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, ModbusDefinitions& definitions)
{
    if (xml.isStartElement() && xml.name() == QLatin1String("ModbusDefinitions")) {
        QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("AddrSpace")) {
            definitions.AddrSpace = enumFromString<AddressSpace>(attributes.value("AddrSpace").toString());
        }

        if (attributes.hasAttribute("UseGlobalUnitMap")) {
            definitions.UseGlobalUnitMap = stringToBool(attributes.value("UseGlobalUnitMap").toString());
        }

        if (xml.readNextStartElement() && xml.name() == QLatin1String("ModbusErrorSimulations")) {
            xml >> definitions.ErrorSimulations;
        }

        xml.skipCurrentElement();

        definitions.normalize();
    }

    return xml;
}

#endif // MODBUSDEFINITIONS_H
