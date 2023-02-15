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

    void normalize()
    {
        UpdateRate = qBound(20U, UpdateRate, 10000U);
        DeviceId = qMax<quint8>(ModbusLimits::slaveRange().from(), DeviceId);
        PointAddress = qMax<quint16>(ModbusLimits::addressRange().from(), PointAddress);
        PointType = qBound(QModbusDataUnit::DiscreteInputs, PointType, QModbusDataUnit::HoldingRegisters);
        Length = qBound<quint16>(ModbusLimits::lengthRange().from(), Length, ModbusLimits::lengthRange().to());
    }
};
Q_DECLARE_METATYPE(DisplayDefinition)

inline QSettings& operator <<(QSettings& out, const DisplayDefinition& dd)
{
    out.setValue("DisplayDefinition/UpdateRate",    dd.UpdateRate);
    out.setValue("DisplayDefinition/DeviceId",      dd.DeviceId);
    out.setValue("DisplayDefinition/PointAddress",  dd.PointAddress);
    out.setValue("DisplayDefinition/PointType",     dd.PointType);
    out.setValue("DisplayDefinition/Length",        dd.Length);

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
    dd.UpdateRate = in.value("DisplayDefinition/UpdateRate").toUInt();
    dd.DeviceId = in.value("DisplayDefinition/DeviceId").toUInt();
    dd.PointAddress = in.value("DisplayDefinition/PointAddress").toUInt();
    dd.PointType = (QModbusDataUnit::RegisterType)in.value("DisplayDefinition/PointType").toUInt();
    dd.Length = in.value("DisplayDefinition/Length").toUInt();

    dd.normalize();
    return in;
}

#endif // DISPLAYDEFINITION_H
