#ifndef DISPLAYDEFINITION_H
#define DISPLAYDEFINITION_H

#include <QSettings>
#include <QModbusDataUnit>
#include <QXmlStreamWriter>
#include <QSet>
#include "modbuslimits.h"
#include "modbusserver.h"
#include "scriptsettings.h"

///
/// \brief The DataViewDefinitions struct
///
struct DataViewDefinitions
{
    QString FormName;
    quint8 DeviceId = 1;
    quint16 PointAddress = 1;
    QModbusDataUnit::RegisterType PointType = QModbusDataUnit::HoldingRegisters;
    quint16 Length = 100;
    bool ZeroBasedAddress = false;
    bool HexAddress = false;
    bool HexViewAddress  = false;
    bool HexViewDeviceId = false;
    bool HexViewLength   = false;
    AddressSpace AddrSpace = AddressSpace::Addr6Digits;
    quint16 DataViewColumnsDistance = 16;
    bool LeadingZeros = true;

    void normalize()
    {
        DeviceId = qMax<quint8>(ModbusLimits::slaveRange().from(), DeviceId);
        PointAddress = qMax<quint16>(ModbusLimits::addressRange(AddrSpace, ZeroBasedAddress).from(), PointAddress);
        PointType = qBound(QModbusDataUnit::DiscreteInputs, PointType, QModbusDataUnit::HoldingRegisters);

        const bool bitPointType = (PointType == QModbusDataUnit::Coils ||
                                   PointType == QModbusDataUnit::DiscreteInputs);
        const int offset = PointAddress - (ZeroBasedAddress ? 0 : 1);
        const int maxByAddress = ModbusLimits::addressSpaceSize(AddrSpace) - offset;
        const int maxByType = bitPointType ? 2000 : ModbusLimits::lengthRange().to();
        const int maxLen = qMax(1, qMin(maxByType, maxByAddress));
        Length = qBound<quint16>(ModbusLimits::lengthRange().from(), Length, static_cast<quint16>(maxLen));

        DataViewColumnsDistance = qBound<quint16>(1, DataViewColumnsDistance, 32);
    }
};

///
/// \brief The TrafficViewDefinitions struct
///
struct TrafficViewDefinitions
{
    QString FormName;
    quint8 UnitFilter = 0;
    qint16 FunctionCodeFilter = -1;
    quint16 LogViewLimit = 30;
    bool ExceptionsOnly = false;

    void normalize()
    {
        UnitFilter = qBound<quint8>(0, UnitFilter, 247);
        static const QSet<qint16> kAllowedFunctionCodes = {
            1, 2, 3, 4, 5, 6, 15, 16, 22, 23
        };
        if (FunctionCodeFilter != -1 && !kAllowedFunctionCodes.contains(FunctionCodeFilter))
            FunctionCodeFilter = -1;
        LogViewLimit = qBound<quint16>(4, LogViewLimit, 1000);
    }
};

///
/// \brief The ScriptViewDefinitions struct
///
struct ScriptViewDefinitions
{
    QString FormName;
    ScriptSettings ScriptCfg;

    void normalize()
    {
        ScriptCfg.normalize();
    }
};
Q_DECLARE_METATYPE(DataViewDefinitions)
Q_DECLARE_METATYPE(TrafficViewDefinitions)
Q_DECLARE_METATYPE(ScriptViewDefinitions)

///
/// \brief QSettings writer for DataViewDefinitions
/// \param out
/// \param dd
/// \return
///
inline QSettings& operator <<(QSettings& out, const DataViewDefinitions& dd)
{
    out.setValue("DataViewDefinitions/FormName",              dd.FormName);
    out.setValue("DataViewDefinitions/DeviceId",              dd.DeviceId);
    out.setValue("DataViewDefinitions/PointAddress",          dd.PointAddress);
    out.setValue("DataViewDefinitions/PointType",             dd.PointType);
    out.setValue("DataViewDefinitions/Length",                dd.Length);
    out.setValue("DataViewDefinitions/DataViewColumnSpace",   dd.DataViewColumnsDistance);
    out.setValue("DataViewDefinitions/LeadingZeros",          dd.LeadingZeros);
    out.setValue("DataViewDefinitions/ZeroBasedAddress",      dd.ZeroBasedAddress);
    out.setValue("DataViewDefinitions/HexAddress",            dd.HexAddress);
    out.setValue("DataViewDefinitions/HexViewAddress",        dd.HexViewAddress);
    out.setValue("DataViewDefinitions/HexViewDeviceId",       dd.HexViewDeviceId);
    out.setValue("DataViewDefinitions/HexViewLength",         dd.HexViewLength);

    return out;
}

///
/// \brief QSettings writer for TrafficViewDefinitions
///
inline QSettings& operator <<(QSettings& out, const TrafficViewDefinitions& dd)
{
    out.setValue("TrafficViewDefinitions/FormName",              dd.FormName);
    out.setValue("TrafficViewDefinitions/UnitFilter",            dd.UnitFilter);
    out.setValue("TrafficViewDefinitions/FunctionCodeFilter",    dd.FunctionCodeFilter);
    out.setValue("TrafficViewDefinitions/LogViewLimit",          dd.LogViewLimit);
    out.setValue("TrafficViewDefinitions/ExceptionsOnly",        dd.ExceptionsOnly);

    return out;
}

///
/// \brief QSettings writer for ScriptViewDefinitions
///
inline QSettings& operator <<(QSettings& out, const ScriptViewDefinitions& dd)
{
    out.setValue("ScriptViewDefinitions/FormName",              dd.FormName);

    out.beginGroup("ScriptViewDefinitions");
    out << dd.ScriptCfg;
    out.endGroup();

    return out;
}

///
/// \brief QSettings reader for DataViewDefinitions
/// \param in
/// \param dd
/// \return
///
inline QSettings& operator >>(QSettings& in, DataViewDefinitions& dd)
{
    dd.FormName = in.value("DataViewDefinitions/FormName").toString();
    dd.DeviceId = in.value("DataViewDefinitions/DeviceId", 1).toUInt();
    dd.PointAddress = in.value("DataViewDefinitions/PointAddress", 1).toUInt();
    dd.PointType = (QModbusDataUnit::RegisterType)in.value("DataViewDefinitions/PointType", 4).toUInt();
    dd.Length = in.value("DataViewDefinitions/Length", 100).toUInt();
    dd.DataViewColumnsDistance = in.value("DataViewDefinitions/DataViewColumnSpace", 16).toUInt();
    dd.LeadingZeros = in.value("DataViewDefinitions/LeadingZeros", true).toBool();
    dd.ZeroBasedAddress = in.value("DataViewDefinitions/ZeroBasedAddress").toBool();
    dd.HexAddress = in.value("DataViewDefinitions/HexAddress").toBool();
    dd.HexViewAddress  = in.value("DataViewDefinitions/HexViewAddress",  false).toBool();
    dd.HexViewDeviceId = in.value("DataViewDefinitions/HexViewDeviceId", false).toBool();
    dd.HexViewLength   = in.value("DataViewDefinitions/HexViewLength",   false).toBool();

    dd.normalize();
    return in;
}

///
/// \brief QSettings reader for TrafficViewDefinitions
///
inline QSettings& operator >>(QSettings& in, TrafficViewDefinitions& dd)
{
    dd.FormName = in.value("TrafficViewDefinitions/FormName").toString();
    dd.UnitFilter = in.value("TrafficViewDefinitions/UnitFilter", 0).toUInt();
    dd.FunctionCodeFilter = in.value("TrafficViewDefinitions/FunctionCodeFilter", -1).toInt();
    dd.LogViewLimit = in.value("TrafficViewDefinitions/LogViewLimit", 30).toUInt();
    dd.ExceptionsOnly = in.value("TrafficViewDefinitions/ExceptionsOnly", false).toBool();

    dd.normalize();
    return in;
}

///
/// \brief QSettings reader for ScriptViewDefinitions
///
inline QSettings& operator >>(QSettings& in, ScriptViewDefinitions& dd)
{
    dd.FormName = in.value("ScriptViewDefinitions/FormName").toString();

    in.beginGroup("ScriptViewDefinitions");
    in >> dd.ScriptCfg;
    in.endGroup();

    dd.normalize();
    return in;
}

///
/// \brief XML writer for DataViewDefinitions
/// \param xml
/// \param dd
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const DataViewDefinitions& dd)
{
    xml.writeStartElement("DataViewDefinitions");
    xml.writeAttribute("FormName", dd.FormName);
    xml.writeAttribute("DeviceId", QString::number(dd.DeviceId));
    xml.writeAttribute("PointType", enumToString<QModbusDataUnit::RegisterType>(dd.PointType));
    xml.writeAttribute("PointAddress", QString::number(dd.PointAddress));
    xml.writeAttribute("Length", QString::number(dd.Length));
    xml.writeAttribute("ZeroBasedAddress", boolToString(dd.ZeroBasedAddress));
    xml.writeAttribute("DataViewColumnsDistance", QString::number(dd.DataViewColumnsDistance));
    xml.writeAttribute("LeadingZeros", boolToString(dd.LeadingZeros));
    xml.writeAttribute("HexAddress",       boolToString(dd.HexAddress));
    xml.writeAttribute("HexViewAddress",  boolToString(dd.HexViewAddress));
    xml.writeAttribute("HexViewDeviceId", boolToString(dd.HexViewDeviceId));
    xml.writeAttribute("HexViewLength",   boolToString(dd.HexViewLength));
    xml.writeEndElement();

    return xml;
}

///
/// \brief XML writer for TrafficViewDefinitions
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const TrafficViewDefinitions& dd)
{
    xml.writeStartElement("TrafficViewDefinitions");
    xml.writeAttribute("FormName", dd.FormName);
    xml.writeAttribute("UnitFilter", QString::number(dd.UnitFilter));
    xml.writeAttribute("FunctionCodeFilter", QString::number(dd.FunctionCodeFilter));
    xml.writeAttribute("LogViewLimit", QString::number(dd.LogViewLimit));
    xml.writeAttribute("ExceptionsOnly", boolToString(dd.ExceptionsOnly));
    xml.writeEndElement();

    return xml;
}

///
/// \brief XML writer for ScriptViewDefinitions
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const ScriptViewDefinitions& dd)
{
    xml.writeStartElement("ScriptViewDefinitions");
    xml.writeAttribute("FormName", dd.FormName);
    xml << dd.ScriptCfg;
    xml.writeEndElement();

    return xml;
}

///
/// \brief XML reader for DataViewDefinitions
/// \param xml
/// \param dd
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, DataViewDefinitions& dd)
{
    if (xml.isStartElement() && xml.name() == QLatin1String("DataViewDefinitions")) {
        const QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("FormName")) {
            dd.FormName = attributes.value("FormName").toString();
        }

        if (attributes.hasAttribute("DeviceId")) {
            bool ok; const quint8 deviceId = attributes.value("DeviceId").toUShort(&ok);
            if (ok) dd.DeviceId = deviceId;
        }

        if (attributes.hasAttribute("PointType")) {
            dd.PointType = enumFromString<QModbusDataUnit::RegisterType>(attributes.value("PointType").toString());
        }

        if (attributes.hasAttribute("PointAddress")) {
            bool ok; const quint16 pointAddress = attributes.value("PointAddress").toUShort(&ok);
            if (ok) dd.PointAddress = pointAddress;
        }

        if (attributes.hasAttribute("Length")) {
            bool ok; const quint16 length = attributes.value("Length").toUShort(&ok);
            if (ok) dd.Length = length;
        }

        if (attributes.hasAttribute("ZeroBasedAddress")) {
            dd.ZeroBasedAddress = stringToBool(attributes.value("ZeroBasedAddress").toString());
        }

        if (attributes.hasAttribute("DataViewColumnsDistance")) {
            bool ok; const quint16 distance = attributes.value("DataViewColumnsDistance").toUShort(&ok);
            if (ok) dd.DataViewColumnsDistance = distance;
        }

        if (attributes.hasAttribute("LeadingZeros")) {
            dd.LeadingZeros = stringToBool(attributes.value("LeadingZeros").toString());
        }

        if (attributes.hasAttribute("HexAddress"))
            dd.HexAddress = stringToBool(attributes.value("HexAddress").toString());

        if (attributes.hasAttribute("HexViewAddress"))
            dd.HexViewAddress = stringToBool(attributes.value("HexViewAddress").toString());

        if (attributes.hasAttribute("HexViewDeviceId"))
            dd.HexViewDeviceId = stringToBool(attributes.value("HexViewDeviceId").toString());

        if (attributes.hasAttribute("HexViewLength"))
            dd.HexViewLength = stringToBool(attributes.value("HexViewLength").toString());

        dd.normalize();
    }

    return xml;
}

///
/// \brief XML reader for TrafficViewDefinitions
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, TrafficViewDefinitions& dd)
{
    if (xml.isStartElement() && xml.name() == QLatin1String("TrafficViewDefinitions")) {
        const QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("FormName")) {
            dd.FormName = attributes.value("FormName").toString();
        }

        if (attributes.hasAttribute("UnitFilter")) {
            bool ok; const quint8 unitFilter = attributes.value("UnitFilter").toUShort(&ok);
            if (ok) dd.UnitFilter = unitFilter;
        }

        if (attributes.hasAttribute("FunctionCodeFilter")) {
            bool ok; const qint16 functionCodeFilter = attributes.value("FunctionCodeFilter").toShort(&ok);
            if (ok) dd.FunctionCodeFilter = functionCodeFilter;
        }

        if (attributes.hasAttribute("LogViewLimit")) {
            bool ok; const quint16 logViewLimit = attributes.value("LogViewLimit").toUShort(&ok);
            if (ok) dd.LogViewLimit = logViewLimit;
        }

        if (attributes.hasAttribute("ExceptionsOnly")) {
            dd.ExceptionsOnly = stringToBool(attributes.value("ExceptionsOnly").toString());
        }

        dd.normalize();
    }

    return xml;
}

///
/// \brief XML reader for ScriptViewDefinitions
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, ScriptViewDefinitions& dd)
{
    if (xml.isStartElement() && xml.name() == QLatin1String("ScriptViewDefinitions")) {
        const QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("FormName")) {
            dd.FormName = attributes.value("FormName").toString();
        }

        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("ScriptSettings")) {
                xml >> dd.ScriptCfg;
            } else {
                xml.skipCurrentElement();
            }
        }

        dd.normalize();
    }

    return xml;
}

#endif // DISPLAYDEFINITION_H
