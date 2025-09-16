#ifndef DISPLAYDEFINITION_H
#define DISPLAYDEFINITION_H

#include <QSettings>
#include <QModbusDataUnit>
#include "modbuslimits.h"

///
/// \brief The DisplayDefinition struct
///
struct DisplayDefinition
{
    quint8 DeviceId = 1;
    quint16 PointAddress = 1;
    QModbusDataUnit::RegisterType PointType = QModbusDataUnit::HoldingRegisters;
    quint16 Length = 100;
    quint16 LogViewLimit = 30;
    bool ZeroBasedAddress = false;
    bool UseGlobalUnitMap = false;
    bool HexAddress = false;
    bool AutoscrollLog = false;
    bool VerboseLogging = true;

    void normalize()
    {
        DeviceId = qMax<quint8>(ModbusLimits::slaveRange().from(), DeviceId);
        PointAddress = qMax<quint16>(ModbusLimits::addressRange(ZeroBasedAddress).from(), PointAddress);
        PointType = qBound(QModbusDataUnit::DiscreteInputs, PointType, QModbusDataUnit::HoldingRegisters);
        Length = qBound<quint16>(ModbusLimits::lengthRange().from(), Length, ModbusLimits::lengthRange().to());
        LogViewLimit = qBound<quint16>(4, LogViewLimit, 1000);
    }
};
Q_DECLARE_METATYPE(DisplayDefinition)

///
/// \brief operator <<
/// \param out
/// \param dd
/// \return
///
inline QSettings& operator <<(QSettings& out, const DisplayDefinition& dd)
{
    out.setValue("DisplayDefinition/DeviceId",          dd.DeviceId);
    out.setValue("DisplayDefinition/PointAddress",      dd.PointAddress);
    out.setValue("DisplayDefinition/PointType",         dd.PointType);
    out.setValue("DisplayDefinition/Length",            dd.Length);
    out.setValue("DisplayDefinition/LogViewLimit",      dd.LogViewLimit);
    out.setValue("DisplayDefinition/ZeroBasedAddress",  dd.ZeroBasedAddress);
    out.setValue("DisplayDefinition/UseGlobalUnitMap",  dd.UseGlobalUnitMap);
    out.setValue("DisplayDefinition/HexAddress",        dd.HexAddress);
    out.setValue("DisplayDefinition/AutoscrollLog",     dd.AutoscrollLog);
    out.setValue("DisplayDefinition/VerboseLogging",    dd.VerboseLogging);


    return out;
}

///
/// \brief operator >>
/// \param in
/// \param dd
/// \return
///
inline QSettings& operator >>(QSettings& in, DisplayDefinition& dd)
{
    dd.DeviceId = in.value("DisplayDefinition/DeviceId", 1).toUInt();
    dd.PointAddress = in.value("DisplayDefinition/PointAddress", 1).toUInt();
    dd.PointType = (QModbusDataUnit::RegisterType)in.value("DisplayDefinition/PointType", 4).toUInt();
    dd.Length = in.value("DisplayDefinition/Length", 100).toUInt();
    dd.LogViewLimit = in.value("DisplayDefinition/LogViewLimit", 30).toUInt();
    dd.ZeroBasedAddress = in.value("DisplayDefinition/ZeroBasedAddress").toBool();
    dd.UseGlobalUnitMap = in.value("DisplayDefinition/UseGlobalUnitMap").toBool();
    dd.HexAddress = in.value("DisplayDefinition/HexAddress").toBool();
    dd.AutoscrollLog = in.value("DisplayDefinition/AutoscrollLog").toBool();
    dd.VerboseLogging = in.value("DisplayDefinition/VerboseLogging").toBool();

    dd.normalize();
    return in;
}

#endif // DISPLAYDEFINITION_H
