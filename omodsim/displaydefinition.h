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
    quint32 UpdateRate = 1000;
    quint8 DeviceId = 1;
    quint16 PointAddress = 1;
    QModbusDataUnit::RegisterType PointType = QModbusDataUnit::HoldingRegisters;
    quint16 Length = 100;
    quint16 LogViewLimit = 30;

    void normalize()
    {
        UpdateRate = qBound(20U, UpdateRate, 10000U);
        DeviceId = qMax<quint8>(ModbusLimits::slaveRange().from(), DeviceId);
        PointAddress = qMax<quint16>(ModbusLimits::addressRange().from(), PointAddress);
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
    out.setValue("DisplayDefinition/UpdateRate",    dd.UpdateRate);
    out.setValue("DisplayDefinition/DeviceId",      dd.DeviceId);
    out.setValue("DisplayDefinition/PointAddress",  dd.PointAddress);
    out.setValue("DisplayDefinition/PointType",     dd.PointType);
    out.setValue("DisplayDefinition/Length",        dd.Length);
    out.setValue("DisplayDefinition/LogViewLimit",  dd.LogViewLimit);

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
    dd.UpdateRate = in.value("DisplayDefinition/UpdateRate", 1000).toUInt();
    dd.DeviceId = in.value("DisplayDefinition/DeviceId", 1).toUInt();
    dd.PointAddress = in.value("DisplayDefinition/PointAddress", 1).toUInt();
    dd.PointType = (QModbusDataUnit::RegisterType)in.value("DisplayDefinition/PointType", 4).toUInt();
    dd.Length = in.value("DisplayDefinition/Length", 100).toUInt();
    dd.LogViewLimit = in.value("DisplayDefinition/LogViewLimit", 30).toUInt();

    dd.normalize();
    return in;
}

///
/// \brief operator <<
/// \param out
/// \param dd
/// \return
///
inline QDataStream& operator <<(QDataStream& out, const DisplayDefinition& dd)
{
    out << dd.UpdateRate;
    out << dd.DeviceId;
    out << dd.PointType;
    out << dd.PointAddress;
    out << dd.Length;

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param dd
/// \return
///
inline QDataStream& operator >>(QDataStream& in, DisplayDefinition& dd)
{
    in >> dd.UpdateRate;
    in >> dd.DeviceId;
    in >> dd.PointType;
    in >> dd.PointAddress;
    in >> dd.Length;

    return in;
}

#endif // DISPLAYDEFINITION_H
