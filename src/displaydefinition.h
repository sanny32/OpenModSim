#ifndef DISPLAYDEFINITION_H
#define DISPLAYDEFINITION_H

#include <QSettings>
#include <QModbusDataUnit>
#include <QXmlStreamWriter>
#include <variant>
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
    quint16 LogViewLimit = 30;
    bool ZeroBasedAddress = false;
    bool HexAddress = false;
    bool HexViewAddress  = false;
    bool HexViewDeviceId = false;
    bool HexViewLength   = false;
    bool AutoscrollLog = false;
    bool VerboseLogging = true;
    AddressSpace AddrSpace = AddressSpace::Addr6Digits;
    quint16 DataViewColumnsDistance = 16;
    bool LeadingZeros = true;

    void normalize()
    {
        DeviceId = qMax<quint8>(ModbusLimits::slaveRange().from(), DeviceId);
        PointAddress = qMax<quint16>(ModbusLimits::addressRange(AddrSpace, ZeroBasedAddress).from(), PointAddress);
        PointType = qBound(QModbusDataUnit::DiscreteInputs, PointType, QModbusDataUnit::HoldingRegisters);
        Length = qBound<quint16>(ModbusLimits::lengthRange().from(), Length,
                                 (quint16)ModbusLimits::lengthRange(PointAddress, ZeroBasedAddress, AddrSpace).to());
        LogViewLimit = qBound<quint16>(4, LogViewLimit, 1000);
        DataViewColumnsDistance = qBound<quint16>(1, DataViewColumnsDistance, 32);
    }
};

///
/// \brief The TrafficViewDefinitions struct
///
struct TrafficViewDefinitions
{
    QString FormName;
    quint8 DeviceId = 1;
    quint16 PointAddress = 1;
    QModbusDataUnit::RegisterType PointType = QModbusDataUnit::HoldingRegisters;
    quint16 Length = 100;
    quint16 LogViewLimit = 30;
    bool ZeroBasedAddress = false;
    bool HexAddress = false;
    bool HexViewAddress  = false;
    bool HexViewDeviceId = false;
    bool HexViewLength   = false;
    bool AutoscrollLog = false;
    bool VerboseLogging = true;
    AddressSpace AddrSpace = AddressSpace::Addr6Digits;
    quint16 DataViewColumnsDistance = 16;
    bool LeadingZeros = true;
    ScriptSettings ScriptCfg;

    void normalize()
    {
        DeviceId = qMax<quint8>(ModbusLimits::slaveRange().from(), DeviceId);
        PointAddress = qMax<quint16>(ModbusLimits::addressRange(AddrSpace, ZeroBasedAddress).from(), PointAddress);
        PointType = qBound(QModbusDataUnit::DiscreteInputs, PointType, QModbusDataUnit::HoldingRegisters);
        Length = qBound<quint16>(ModbusLimits::lengthRange().from(), Length,
                                 (quint16)ModbusLimits::lengthRange(PointAddress, ZeroBasedAddress, AddrSpace).to());
        LogViewLimit = qBound<quint16>(4, LogViewLimit, 1000);
        DataViewColumnsDistance = qBound<quint16>(1, DataViewColumnsDistance, 32);
        ScriptCfg.normalize();
    }
};

///
/// \brief The ScriptViewDefinitions struct
///
struct ScriptViewDefinitions
{
    QString FormName;
    quint8 DeviceId = 1;
    quint16 PointAddress = 1;
    QModbusDataUnit::RegisterType PointType = QModbusDataUnit::HoldingRegisters;
    quint16 Length = 100;
    quint16 LogViewLimit = 30;
    bool ZeroBasedAddress = false;
    bool HexAddress = false;
    bool HexViewAddress  = false;
    bool HexViewDeviceId = false;
    bool HexViewLength   = false;
    bool AutoscrollLog = false;
    bool VerboseLogging = true;
    AddressSpace AddrSpace = AddressSpace::Addr6Digits;
    quint16 DataViewColumnsDistance = 16;
    bool LeadingZeros = true;
    ScriptSettings ScriptCfg;

    void normalize()
    {
        DeviceId = qMax<quint8>(ModbusLimits::slaveRange().from(), DeviceId);
        PointAddress = qMax<quint16>(ModbusLimits::addressRange(AddrSpace, ZeroBasedAddress).from(), PointAddress);
        PointType = qBound(QModbusDataUnit::DiscreteInputs, PointType, QModbusDataUnit::HoldingRegisters);
        Length = qBound<quint16>(ModbusLimits::lengthRange().from(), Length,
                                 (quint16)ModbusLimits::lengthRange(PointAddress, ZeroBasedAddress, AddrSpace).to());
        LogViewLimit = qBound<quint16>(4, LogViewLimit, 1000);
        DataViewColumnsDistance = qBound<quint16>(1, DataViewColumnsDistance, 32);
        ScriptCfg.normalize();
    }
};
Q_DECLARE_METATYPE(DataViewDefinitions)
Q_DECLARE_METATYPE(TrafficViewDefinitions)
Q_DECLARE_METATYPE(ScriptViewDefinitions)
using FormDisplayDefinition = std::variant<DataViewDefinitions, TrafficViewDefinitions, ScriptViewDefinitions>;
Q_DECLARE_METATYPE(FormDisplayDefinition)

inline DataViewDefinitions toDataViewDefinitions(const DataViewDefinitions& src)
{
    return src;
}

inline DataViewDefinitions toDataViewDefinitions(const TrafficViewDefinitions& src)
{
    DataViewDefinitions out;
    out.FormName = src.FormName;
    out.DeviceId = src.DeviceId;
    out.PointAddress = src.PointAddress;
    out.PointType = src.PointType;
    out.Length = src.Length;
    out.LogViewLimit = src.LogViewLimit;
    out.ZeroBasedAddress = src.ZeroBasedAddress;
    out.HexAddress = src.HexAddress;
    out.HexViewAddress = src.HexViewAddress;
    out.HexViewDeviceId = src.HexViewDeviceId;
    out.HexViewLength = src.HexViewLength;
    out.AutoscrollLog = src.AutoscrollLog;
    out.VerboseLogging = src.VerboseLogging;
    out.AddrSpace = src.AddrSpace;
    out.DataViewColumnsDistance = src.DataViewColumnsDistance;
    out.LeadingZeros = src.LeadingZeros;
    return out;
}

inline DataViewDefinitions toDataViewDefinitions(const ScriptViewDefinitions& src)
{
    DataViewDefinitions out;
    out.FormName = src.FormName;
    out.DeviceId = src.DeviceId;
    out.PointAddress = src.PointAddress;
    out.PointType = src.PointType;
    out.Length = src.Length;
    out.LogViewLimit = src.LogViewLimit;
    out.ZeroBasedAddress = src.ZeroBasedAddress;
    out.HexAddress = src.HexAddress;
    out.HexViewAddress = src.HexViewAddress;
    out.HexViewDeviceId = src.HexViewDeviceId;
    out.HexViewLength = src.HexViewLength;
    out.AutoscrollLog = src.AutoscrollLog;
    out.VerboseLogging = src.VerboseLogging;
    out.AddrSpace = src.AddrSpace;
    out.DataViewColumnsDistance = src.DataViewColumnsDistance;
    out.LeadingZeros = src.LeadingZeros;
    return out;
}

inline TrafficViewDefinitions toTrafficViewDefinitions(const TrafficViewDefinitions& src)
{
    return src;
}

inline TrafficViewDefinitions toTrafficViewDefinitions(const ScriptViewDefinitions& src)
{
    TrafficViewDefinitions out;
    out.FormName = src.FormName;
    out.DeviceId = src.DeviceId;
    out.PointAddress = src.PointAddress;
    out.PointType = src.PointType;
    out.Length = src.Length;
    out.LogViewLimit = src.LogViewLimit;
    out.ZeroBasedAddress = src.ZeroBasedAddress;
    out.HexAddress = src.HexAddress;
    out.HexViewAddress = src.HexViewAddress;
    out.HexViewDeviceId = src.HexViewDeviceId;
    out.HexViewLength = src.HexViewLength;
    out.AutoscrollLog = src.AutoscrollLog;
    out.VerboseLogging = src.VerboseLogging;
    out.AddrSpace = src.AddrSpace;
    out.DataViewColumnsDistance = src.DataViewColumnsDistance;
    out.LeadingZeros = src.LeadingZeros;
    out.ScriptCfg = src.ScriptCfg;
    return out;
}

inline TrafficViewDefinitions toTrafficViewDefinitions(const DataViewDefinitions& src)
{
    TrafficViewDefinitions out;
    out.FormName = src.FormName;
    out.DeviceId = src.DeviceId;
    out.PointAddress = src.PointAddress;
    out.PointType = src.PointType;
    out.Length = src.Length;
    out.LogViewLimit = src.LogViewLimit;
    out.ZeroBasedAddress = src.ZeroBasedAddress;
    out.HexAddress = src.HexAddress;
    out.HexViewAddress = src.HexViewAddress;
    out.HexViewDeviceId = src.HexViewDeviceId;
    out.HexViewLength = src.HexViewLength;
    out.AutoscrollLog = src.AutoscrollLog;
    out.VerboseLogging = src.VerboseLogging;
    out.AddrSpace = src.AddrSpace;
    out.DataViewColumnsDistance = src.DataViewColumnsDistance;
    out.LeadingZeros = src.LeadingZeros;
    return out;
}

inline ScriptViewDefinitions toScriptViewDefinitions(const ScriptViewDefinitions& src)
{
    return src;
}

inline ScriptViewDefinitions toScriptViewDefinitions(const TrafficViewDefinitions& src)
{
    ScriptViewDefinitions out;
    out.FormName = src.FormName;
    out.DeviceId = src.DeviceId;
    out.PointAddress = src.PointAddress;
    out.PointType = src.PointType;
    out.Length = src.Length;
    out.LogViewLimit = src.LogViewLimit;
    out.ZeroBasedAddress = src.ZeroBasedAddress;
    out.HexAddress = src.HexAddress;
    out.HexViewAddress = src.HexViewAddress;
    out.HexViewDeviceId = src.HexViewDeviceId;
    out.HexViewLength = src.HexViewLength;
    out.AutoscrollLog = src.AutoscrollLog;
    out.VerboseLogging = src.VerboseLogging;
    out.AddrSpace = src.AddrSpace;
    out.DataViewColumnsDistance = src.DataViewColumnsDistance;
    out.LeadingZeros = src.LeadingZeros;
    out.ScriptCfg = src.ScriptCfg;
    return out;
}

inline ScriptViewDefinitions toScriptViewDefinitions(const DataViewDefinitions& src)
{
    ScriptViewDefinitions out;
    out.FormName = src.FormName;
    out.DeviceId = src.DeviceId;
    out.PointAddress = src.PointAddress;
    out.PointType = src.PointType;
    out.Length = src.Length;
    out.LogViewLimit = src.LogViewLimit;
    out.ZeroBasedAddress = src.ZeroBasedAddress;
    out.HexAddress = src.HexAddress;
    out.HexViewAddress = src.HexViewAddress;
    out.HexViewDeviceId = src.HexViewDeviceId;
    out.HexViewLength = src.HexViewLength;
    out.AutoscrollLog = src.AutoscrollLog;
    out.VerboseLogging = src.VerboseLogging;
    out.AddrSpace = src.AddrSpace;
    out.DataViewColumnsDistance = src.DataViewColumnsDistance;
    out.LeadingZeros = src.LeadingZeros;
    return out;
}

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
    out.setValue("DataViewDefinitions/LogViewLimit",          dd.LogViewLimit);
    out.setValue("DataViewDefinitions/DataViewColumnSpace",   dd.DataViewColumnsDistance);
    out.setValue("DataViewDefinitions/LeadingZeros",          dd.LeadingZeros);
    out.setValue("DataViewDefinitions/ZeroBasedAddress",      dd.ZeroBasedAddress);
    out.setValue("DataViewDefinitions/HexAddress",            dd.HexAddress);
    out.setValue("DataViewDefinitions/HexViewAddress",        dd.HexViewAddress);
    out.setValue("DataViewDefinitions/HexViewDeviceId",       dd.HexViewDeviceId);
    out.setValue("DataViewDefinitions/HexViewLength",         dd.HexViewLength);
    out.setValue("DataViewDefinitions/AutoscrollLog",         dd.AutoscrollLog);
    out.setValue("DataViewDefinitions/VerboseLogging",        dd.VerboseLogging);

    return out;
}

///
/// \brief QSettings writer for TrafficViewDefinitions
///
inline QSettings& operator <<(QSettings& out, const TrafficViewDefinitions& dd)
{
    out.setValue("TrafficViewDefinitions/FormName",              dd.FormName);
    out.setValue("TrafficViewDefinitions/DeviceId",              dd.DeviceId);
    out.setValue("TrafficViewDefinitions/PointAddress",          dd.PointAddress);
    out.setValue("TrafficViewDefinitions/PointType",             dd.PointType);
    out.setValue("TrafficViewDefinitions/Length",                dd.Length);
    out.setValue("TrafficViewDefinitions/LogViewLimit",          dd.LogViewLimit);
    out.setValue("TrafficViewDefinitions/DataViewColumnSpace",   dd.DataViewColumnsDistance);
    out.setValue("TrafficViewDefinitions/LeadingZeros",          dd.LeadingZeros);
    out.setValue("TrafficViewDefinitions/ZeroBasedAddress",      dd.ZeroBasedAddress);
    out.setValue("TrafficViewDefinitions/HexAddress",            dd.HexAddress);
    out.setValue("TrafficViewDefinitions/HexViewAddress",        dd.HexViewAddress);
    out.setValue("TrafficViewDefinitions/HexViewDeviceId",       dd.HexViewDeviceId);
    out.setValue("TrafficViewDefinitions/HexViewLength",         dd.HexViewLength);
    out.setValue("TrafficViewDefinitions/AutoscrollLog",         dd.AutoscrollLog);
    out.setValue("TrafficViewDefinitions/VerboseLogging",        dd.VerboseLogging);

    out.beginGroup("TrafficViewDefinitions");
    out << dd.ScriptCfg;
    out.endGroup();

    return out;
}

///
/// \brief QSettings writer for ScriptViewDefinitions
///
inline QSettings& operator <<(QSettings& out, const ScriptViewDefinitions& dd)
{
    out.setValue("ScriptViewDefinitions/FormName",              dd.FormName);
    out.setValue("ScriptViewDefinitions/DeviceId",              dd.DeviceId);
    out.setValue("ScriptViewDefinitions/PointAddress",          dd.PointAddress);
    out.setValue("ScriptViewDefinitions/PointType",             dd.PointType);
    out.setValue("ScriptViewDefinitions/Length",                dd.Length);
    out.setValue("ScriptViewDefinitions/LogViewLimit",          dd.LogViewLimit);
    out.setValue("ScriptViewDefinitions/DataViewColumnSpace",   dd.DataViewColumnsDistance);
    out.setValue("ScriptViewDefinitions/LeadingZeros",          dd.LeadingZeros);
    out.setValue("ScriptViewDefinitions/ZeroBasedAddress",      dd.ZeroBasedAddress);
    out.setValue("ScriptViewDefinitions/HexAddress",            dd.HexAddress);
    out.setValue("ScriptViewDefinitions/HexViewAddress",        dd.HexViewAddress);
    out.setValue("ScriptViewDefinitions/HexViewDeviceId",       dd.HexViewDeviceId);
    out.setValue("ScriptViewDefinitions/HexViewLength",         dd.HexViewLength);
    out.setValue("ScriptViewDefinitions/AutoscrollLog",         dd.AutoscrollLog);
    out.setValue("ScriptViewDefinitions/VerboseLogging",        dd.VerboseLogging);

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
    dd.LogViewLimit = in.value("DataViewDefinitions/LogViewLimit", 30).toUInt();
    dd.DataViewColumnsDistance = in.value("DataViewDefinitions/DataViewColumnSpace", 16).toUInt();
    dd.LeadingZeros = in.value("DataViewDefinitions/LeadingZeros", true).toBool();
    dd.ZeroBasedAddress = in.value("DataViewDefinitions/ZeroBasedAddress").toBool();
    dd.HexAddress = in.value("DataViewDefinitions/HexAddress").toBool();
    dd.HexViewAddress  = in.value("DataViewDefinitions/HexViewAddress",  false).toBool();
    dd.HexViewDeviceId = in.value("DataViewDefinitions/HexViewDeviceId", false).toBool();
    dd.HexViewLength   = in.value("DataViewDefinitions/HexViewLength",   false).toBool();
    dd.AutoscrollLog = in.value("DataViewDefinitions/AutoscrollLog").toBool();
    dd.VerboseLogging = in.value("DataViewDefinitions/VerboseLogging", true).toBool();

    dd.normalize();
    return in;
}

///
/// \brief QSettings reader for TrafficViewDefinitions
///
inline QSettings& operator >>(QSettings& in, TrafficViewDefinitions& dd)
{
    dd.FormName = in.value("TrafficViewDefinitions/FormName").toString();
    dd.DeviceId = in.value("TrafficViewDefinitions/DeviceId", 1).toUInt();
    dd.PointAddress = in.value("TrafficViewDefinitions/PointAddress", 1).toUInt();
    dd.PointType = (QModbusDataUnit::RegisterType)in.value("TrafficViewDefinitions/PointType", 4).toUInt();
    dd.Length = in.value("TrafficViewDefinitions/Length", 100).toUInt();
    dd.LogViewLimit = in.value("TrafficViewDefinitions/LogViewLimit", 30).toUInt();
    dd.DataViewColumnsDistance = in.value("TrafficViewDefinitions/DataViewColumnSpace", 16).toUInt();
    dd.LeadingZeros = in.value("TrafficViewDefinitions/LeadingZeros", true).toBool();
    dd.ZeroBasedAddress = in.value("TrafficViewDefinitions/ZeroBasedAddress").toBool();
    dd.HexAddress = in.value("TrafficViewDefinitions/HexAddress").toBool();
    dd.HexViewAddress  = in.value("TrafficViewDefinitions/HexViewAddress",  false).toBool();
    dd.HexViewDeviceId = in.value("TrafficViewDefinitions/HexViewDeviceId", false).toBool();
    dd.HexViewLength   = in.value("TrafficViewDefinitions/HexViewLength",   false).toBool();
    dd.AutoscrollLog = in.value("TrafficViewDefinitions/AutoscrollLog").toBool();
    dd.VerboseLogging = in.value("TrafficViewDefinitions/VerboseLogging", true).toBool();

    in.beginGroup("TrafficViewDefinitions");
    in >> dd.ScriptCfg;
    in.endGroup();

    dd.normalize();
    return in;
}

///
/// \brief QSettings reader for ScriptViewDefinitions
///
inline QSettings& operator >>(QSettings& in, ScriptViewDefinitions& dd)
{
    dd.FormName = in.value("ScriptViewDefinitions/FormName").toString();
    dd.DeviceId = in.value("ScriptViewDefinitions/DeviceId", 1).toUInt();
    dd.PointAddress = in.value("ScriptViewDefinitions/PointAddress", 1).toUInt();
    dd.PointType = (QModbusDataUnit::RegisterType)in.value("ScriptViewDefinitions/PointType", 4).toUInt();
    dd.Length = in.value("ScriptViewDefinitions/Length", 100).toUInt();
    dd.LogViewLimit = in.value("ScriptViewDefinitions/LogViewLimit", 30).toUInt();
    dd.DataViewColumnsDistance = in.value("ScriptViewDefinitions/DataViewColumnSpace", 16).toUInt();
    dd.LeadingZeros = in.value("ScriptViewDefinitions/LeadingZeros", true).toBool();
    dd.ZeroBasedAddress = in.value("ScriptViewDefinitions/ZeroBasedAddress").toBool();
    dd.HexAddress = in.value("ScriptViewDefinitions/HexAddress").toBool();
    dd.HexViewAddress  = in.value("ScriptViewDefinitions/HexViewAddress",  false).toBool();
    dd.HexViewDeviceId = in.value("ScriptViewDefinitions/HexViewDeviceId", false).toBool();
    dd.HexViewLength   = in.value("ScriptViewDefinitions/HexViewLength",   false).toBool();
    dd.AutoscrollLog = in.value("ScriptViewDefinitions/AutoscrollLog").toBool();
    dd.VerboseLogging = in.value("ScriptViewDefinitions/VerboseLogging", true).toBool();

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
    xml.writeAttribute("LogViewLimit", QString::number(dd.LogViewLimit));
    xml.writeAttribute("ZeroBasedAddress", boolToString(dd.ZeroBasedAddress));
    xml.writeAttribute("AutoscrollLog", boolToString(dd.AutoscrollLog));
    xml.writeAttribute("VerboseLogging", boolToString(dd.VerboseLogging));
    xml.writeAttribute("DataViewColumnsDistance", QString::number(dd.DataViewColumnsDistance));
    xml.writeAttribute("LeadingZeros", boolToString(dd.LeadingZeros));
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
    xml.writeAttribute("DeviceId", QString::number(dd.DeviceId));
    xml.writeAttribute("PointType", enumToString<QModbusDataUnit::RegisterType>(dd.PointType));
    xml.writeAttribute("PointAddress", QString::number(dd.PointAddress));
    xml.writeAttribute("Length", QString::number(dd.Length));
    xml.writeAttribute("LogViewLimit", QString::number(dd.LogViewLimit));
    xml.writeAttribute("ZeroBasedAddress", boolToString(dd.ZeroBasedAddress));
    xml.writeAttribute("AutoscrollLog", boolToString(dd.AutoscrollLog));
    xml.writeAttribute("VerboseLogging", boolToString(dd.VerboseLogging));
    xml.writeAttribute("DataViewColumnsDistance", QString::number(dd.DataViewColumnsDistance));
    xml.writeAttribute("LeadingZeros", boolToString(dd.LeadingZeros));
    xml.writeAttribute("HexViewAddress",  boolToString(dd.HexViewAddress));
    xml.writeAttribute("HexViewDeviceId", boolToString(dd.HexViewDeviceId));
    xml.writeAttribute("HexViewLength",   boolToString(dd.HexViewLength));
    xml << dd.ScriptCfg;
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
    xml.writeAttribute("DeviceId", QString::number(dd.DeviceId));
    xml.writeAttribute("PointType", enumToString<QModbusDataUnit::RegisterType>(dd.PointType));
    xml.writeAttribute("PointAddress", QString::number(dd.PointAddress));
    xml.writeAttribute("Length", QString::number(dd.Length));
    xml.writeAttribute("LogViewLimit", QString::number(dd.LogViewLimit));
    xml.writeAttribute("ZeroBasedAddress", boolToString(dd.ZeroBasedAddress));
    xml.writeAttribute("AutoscrollLog", boolToString(dd.AutoscrollLog));
    xml.writeAttribute("VerboseLogging", boolToString(dd.VerboseLogging));
    xml.writeAttribute("DataViewColumnsDistance", QString::number(dd.DataViewColumnsDistance));
    xml.writeAttribute("LeadingZeros", boolToString(dd.LeadingZeros));
    xml.writeAttribute("HexViewAddress",  boolToString(dd.HexViewAddress));
    xml.writeAttribute("HexViewDeviceId", boolToString(dd.HexViewDeviceId));
    xml.writeAttribute("HexViewLength",   boolToString(dd.HexViewLength));
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

        if (attributes.hasAttribute("LogViewLimit")) {
            bool ok; const quint16 logViewLimit = attributes.value("LogViewLimit").toUShort(&ok);
            if (ok) dd.LogViewLimit = logViewLimit;
        }

        if (attributes.hasAttribute("ZeroBasedAddress")) {
            dd.ZeroBasedAddress = stringToBool(attributes.value("ZeroBasedAddress").toString());
        }

        if (attributes.hasAttribute("AutoscrollLog")) {
            dd.AutoscrollLog = stringToBool(attributes.value("AutoscrollLog").toString());
        }

        if (attributes.hasAttribute("VerboseLogging")) {
            dd.VerboseLogging = stringToBool(attributes.value("VerboseLogging").toString());
        }

        if (attributes.hasAttribute("DataViewColumnsDistance")) {
            bool ok; const quint16 distance = attributes.value("DataViewColumnsDistance").toUShort(&ok);
            if (ok) dd.DataViewColumnsDistance = distance;
        }

        if (attributes.hasAttribute("LeadingZeros")) {
            dd.LeadingZeros = stringToBool(attributes.value("LeadingZeros").toString());
        }

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

        if (attributes.hasAttribute("LogViewLimit")) {
            bool ok; const quint16 logViewLimit = attributes.value("LogViewLimit").toUShort(&ok);
            if (ok) dd.LogViewLimit = logViewLimit;
        }

        if (attributes.hasAttribute("ZeroBasedAddress")) {
            dd.ZeroBasedAddress = stringToBool(attributes.value("ZeroBasedAddress").toString());
        }

        if (attributes.hasAttribute("AutoscrollLog")) {
            dd.AutoscrollLog = stringToBool(attributes.value("AutoscrollLog").toString());
        }

        if (attributes.hasAttribute("VerboseLogging")) {
            dd.VerboseLogging = stringToBool(attributes.value("VerboseLogging").toString());
        }

        if (attributes.hasAttribute("DataViewColumnsDistance")) {
            bool ok; const quint16 distance = attributes.value("DataViewColumnsDistance").toUShort(&ok);
            if (ok) dd.DataViewColumnsDistance = distance;
        }

        if (attributes.hasAttribute("LeadingZeros")) {
            dd.LeadingZeros = stringToBool(attributes.value("LeadingZeros").toString());
        }

        if (attributes.hasAttribute("HexViewAddress"))
            dd.HexViewAddress = stringToBool(attributes.value("HexViewAddress").toString());

        if (attributes.hasAttribute("HexViewDeviceId"))
            dd.HexViewDeviceId = stringToBool(attributes.value("HexViewDeviceId").toString());

        if (attributes.hasAttribute("HexViewLength"))
            dd.HexViewLength = stringToBool(attributes.value("HexViewLength").toString());

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

        if (attributes.hasAttribute("LogViewLimit")) {
            bool ok; const quint16 logViewLimit = attributes.value("LogViewLimit").toUShort(&ok);
            if (ok) dd.LogViewLimit = logViewLimit;
        }

        if (attributes.hasAttribute("ZeroBasedAddress")) {
            dd.ZeroBasedAddress = stringToBool(attributes.value("ZeroBasedAddress").toString());
        }

        if (attributes.hasAttribute("AutoscrollLog")) {
            dd.AutoscrollLog = stringToBool(attributes.value("AutoscrollLog").toString());
        }

        if (attributes.hasAttribute("VerboseLogging")) {
            dd.VerboseLogging = stringToBool(attributes.value("VerboseLogging").toString());
        }

        if (attributes.hasAttribute("DataViewColumnsDistance")) {
            bool ok; const quint16 distance = attributes.value("DataViewColumnsDistance").toUShort(&ok);
            if (ok) dd.DataViewColumnsDistance = distance;
        }

        if (attributes.hasAttribute("LeadingZeros")) {
            dd.LeadingZeros = stringToBool(attributes.value("LeadingZeros").toString());
        }

        if (attributes.hasAttribute("HexViewAddress"))
            dd.HexViewAddress = stringToBool(attributes.value("HexViewAddress").toString());

        if (attributes.hasAttribute("HexViewDeviceId"))
            dd.HexViewDeviceId = stringToBool(attributes.value("HexViewDeviceId").toString());

        if (attributes.hasAttribute("HexViewLength"))
            dd.HexViewLength = stringToBool(attributes.value("HexViewLength").toString());

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
