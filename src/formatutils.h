#ifndef FORMATUTILS_H
#define FORMATUTILS_H

#include <QString>
#include <QLocale>
#include <QModbusPdu>
#include <QModbusDataUnit>
#include "enums.h"
#include "ansiutils.h"
#include "numericutils.h"
#include "byteorderutils.h"

///
/// \brief wrapValue
/// \param s
/// \param brackets
/// \return
///
inline QString wrapValue(const QString& s, bool brackets)
{
    return brackets ? QStringLiteral("<%1>").arg(s) : s;
}

///
/// \brief formatUInt8Value
/// \param mode
/// \param leadingZeros
/// \param c
/// \return
///
inline QString formatUInt8Value(DataType type, bool leadingZeros, quint8 c)
{
    switch(type)
    {
        case DataType::UInt16:
        case DataType::Int16:
            return QString("%1").arg(QString::number(c), 3, QLatin1Char(leadingZeros ? '0' : ' '));

        default:
            return QString("0x%1").arg(QString::number(c, 16).toUpper(), 2, QLatin1Char('0'));
    }
}

///
/// \brief formatUInt8Array
/// \param mode
/// \param leadingZeros
/// \param ar
/// \return
///
inline QString formatUInt8Array(DataType type, bool leadingZeros, const QByteArray& ar)
{
    QStringList values;
    for(quint8 i : ar)
        switch(type)
        {
            case DataType::UInt16:
            case DataType::Int16:
                values += QString("%1").arg(QString::number(i), 3, QLatin1Char(leadingZeros ? '0' : ' '));
            break;

            default:
                values += QString("%1").arg(QString::number(i, 16).toUpper(), 2, QLatin1Char('0'));
            break;
        }

    return values.join(" ");
}

///
/// \brief formatUInt16Array
/// \param mode
/// \param leadingZeros
/// \param ar
/// \param order
/// \return
///
inline QString formatUInt16Array(DataType type, bool leadingZeros, const QByteArray& ar, ByteOrder order)
{
    QStringList values;
    for(int i = 0; i < ar.size(); i+=2)
    {
        const quint16 value = makeUInt16(ar[i+1], ar[i], order);
        switch(type)
        {
            case DataType::UInt16:
            case DataType::Int16:
                values += QString("%1").arg(QString::number(value), 5, QLatin1Char(leadingZeros ? '0' : ' '));
                break;

            default:
                values += QString("0x%1").arg(QString::number(value, 16).toUpper(), 4, QLatin1Char('0'));
                break;
        }
    }

    return values.join(" ");
}

///
/// \brief formatUInt16Value
/// \param mode
/// \param leadingZeros
/// \param v
/// \return
///
inline QString formatUInt16Value(DataType type, bool leadingZeros, quint16 v)
{
    switch(type)
    {
        case DataType::UInt16:
        case DataType::Int16:
            return QString("%1").arg(QString::number(v), 5, QLatin1Char(leadingZeros ? '0' : ' '));

        default:
            return QString("0x%1").arg(QString::number(v, 16).toUpper(), 4, QLatin1Char('0'));
    }
}

///
/// \brief formatBinaryValue
/// \param pointType
/// \param value
/// \param order
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatBinaryValue(QModbusDataUnit::RegisterType pointType, quint16 value, ByteOrder order, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            result = wrapValue(QString::number(value), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
        {
            value = toByteOrderValue(value, order);

            const QString binStr = QStringLiteral("%1").arg(value, 16, 2, QLatin1Char('0'));
            QStringList groups;
            for (int i = 0; i < binStr.size(); i += 4)
                groups << binStr.mid(i, 4);

            result = wrapValue(groups.join(' '), brackets);
        }
        break;
        default:
            break;
    }
    outValue = value;
    return result;
}

///
/// \brief formatUInt16Value
/// \param pointType
/// \param value
/// \param order
/// \param leadingZeros
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatUInt16Value(QModbusDataUnit::RegisterType pointType, quint16 value, ByteOrder order, bool leadingZeros, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            result = wrapValue(QString::number(value), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
            value = toByteOrderValue(value, order);
            result = wrapValue(QString("%1").arg(value, 5, 10, QLatin1Char(leadingZeros ? '0' : ' ')), brackets);
            break;
        default:
            break;
    }
    outValue = value;
    return result;
}

///
/// \brief formatInt16Value
/// \param pointType
/// \param value
/// \param order
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatInt16Value(QModbusDataUnit::RegisterType pointType, qint16 value, ByteOrder order, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            result = wrapValue(QString::number(value), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
            value = toByteOrderValue(value, order);
            result = wrapValue(QString("%1").arg(value, 6, 10, QLatin1Char(' ')), brackets);
            break;
        default:
            break;
    }
    outValue = value;
    return result;
}

///
/// \brief formatHexValue
/// \param pointType
/// \param value
/// \param order
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatHexValue(QModbusDataUnit::RegisterType pointType, quint16 value, ByteOrder order, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            result = wrapValue(QString::number(value), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
            value = toByteOrderValue(value, order);
            result = wrapValue(QStringLiteral("0x%1").arg(QString::number(value, 16).toUpper(), 4, '0'), brackets);
            break;
        default:
            break;
    }
    outValue = value;
    return result;
}

///
/// \brief formatAnsiValue
/// \param pointType
/// \param value
/// \param order
/// \param codepage
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatAnsiValue(QModbusDataUnit::RegisterType pointType, quint16 value, ByteOrder order, const QString& codepage, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            result = wrapValue(QString::number(value), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
            value = toByteOrderValue(value, order);
            result = wrapValue(printableAnsi(uint16ToAnsi(value), codepage), brackets);
            break;
        default:
            break;
    }
    outValue = value;
    return result;
}

///
/// \brief formatFloatValue
/// \param pointType
/// \param value1
/// \param value2
/// \param order
/// \param flag
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatFloatValue(QModbusDataUnit::RegisterType pointType, quint16 value1, quint16 value2, ByteOrder order, bool flag, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            outValue = value1;
            result = wrapValue(QString::number(value1), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
        {
            if(flag) break;

            const float value = makeFloat(value1, value2, order);
            outValue = value;
            result = QString("%1").arg(QLocale().toString(value), -14, QLatin1Char(' '));
        }
        break;
        default:
            break;
    }
    return result;
}

///
/// \brief formatInt32Value
/// \param pointType
/// \param value1
/// \param value2
/// \param order
/// \param flag
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatInt32Value(QModbusDataUnit::RegisterType pointType, quint16 value1, quint16 value2, ByteOrder order, bool flag, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            outValue = value1;
            result = wrapValue(QString::number(value1), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
        {
            if(flag) break;

            const qint32 value = makeInt32(value1, value2, order);
            outValue = value;
            result = wrapValue(QString("%1").arg(value, 11, 10, QLatin1Char(' ')), brackets);
        }
        break;
        default:
            break;
    }
    return result;
}

///
/// \brief formatUInt32Value
/// \param pointType
/// \param value1
/// \param value2
/// \param order
/// \param leadingZeros
/// \param flag
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatUInt32Value(QModbusDataUnit::RegisterType pointType, quint16 value1, quint16 value2, ByteOrder order, bool leadingZeros, bool flag, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            outValue = value1;
            result = wrapValue(QString::number(value1), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
        {
            if(flag) break;

            const quint32 value = makeUInt32(value1, value2, order);
            outValue = value;
            result = wrapValue(QString("%1").arg(value, 10, 10, QLatin1Char(leadingZeros ? '0' : ' ')), brackets);
        }
        break;
        default:
            break;
    }
    return result;
}

///
/// \brief formatDoubleValue
/// \param pointType
/// \param value1
/// \param value2
/// \param value3
/// \param value4
/// \param order
/// \param flag
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatDoubleValue(QModbusDataUnit::RegisterType pointType, quint16 value1, quint16 value2, quint16 value3, quint16 value4, ByteOrder order, bool flag, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            outValue = value1;
            result = wrapValue(QString::number(value1), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
        {
            if(flag) break;

            const double value = makeDouble(value1, value2, value3, value4, order);
            outValue = value;
            result = QString("%1").arg(QLocale().toString(value, 'g', 16), -25, QLatin1Char(' '));
        }
        break;
        default:
            break;
    }
    return result;
}

///
/// \brief formatInt64Value
/// \param pointType
/// \param value1
/// \param value2
/// \param value3
/// \param value4
/// \param order
/// \param flag
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatInt64Value(QModbusDataUnit::RegisterType pointType, quint16 value1, quint16 value2, quint16 value3, quint16 value4, ByteOrder order, bool flag, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            outValue = value1;
            result = wrapValue(QString::number(value1), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
        {
            if(flag) break;

            const qint64 value = makeInt64(value1, value2, value3, value4, order);
            outValue = value;
            result = wrapValue(QString("%1").arg(value, 20, 10, QLatin1Char(' ')), brackets);
        }
        break;
        default:
        break;
    }
    return result;
}

///
/// \brief formatUInt64Value
/// \param pointType
/// \param value1
/// \param value2
/// \param value3
/// \param value4
/// \param order
/// \param leadingZeros
/// \param flag
/// \param outValue
/// \param brackets
/// \return
///
inline QString formatUInt64Value(QModbusDataUnit::RegisterType pointType, quint16 value1, quint16 value2, quint16 value3, quint16 value4, ByteOrder order, bool leadingZeros, bool flag, QVariant& outValue, bool brackets = true)
{
    QString result;
    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            outValue = value1;
            result = wrapValue(QString::number(value1), brackets);
            break;
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
        {
            if(flag) break;

            const quint64 value = makeUInt64(value1, value2, value3, value4, order);
            outValue = value;
            result = wrapValue(QString("%1").arg(value, 20, 10, QLatin1Char(leadingZeros ? '0' : ' ')), brackets);
        }
        break;
        default:
        break;
    }
    return result;
}

///
/// \brief formatAddress
/// \param pointType
/// \param address
/// \param hexFormat
/// \return
///
inline QString formatAddress(QModbusDataUnit::RegisterType pointType, quint16 address, AddressSpace space, bool hexFormat)
{
    char str[8];
    if(hexFormat)
    {
        snprintf(str, sizeof(str), "0x%04X", address);
    }
    else
    {
	    switch(pointType)
	    {
	        case QModbusDataUnit::Coils:
	            *str = '0';
	            break;
	        case QModbusDataUnit::DiscreteInputs:
	            *str = '1';
	            break;
	        case QModbusDataUnit::HoldingRegisters:
	            *str = '4';
	            break;
	        case QModbusDataUnit::InputRegisters:
	            *str = '3';
	            break;
	        default:
	            *str = ' ';
	            break;
	    }
        const int width = space == AddressSpace::Addr6Digits ? 5 : 4;
        snprintf(str + 1, sizeof(str) - 1, "%0*u", width, address);
    }
    return str;
}

#endif // FORMATUTILS_H
