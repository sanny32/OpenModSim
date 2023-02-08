#ifndef CONNECTIONDETAILS_H
#define CONNECTIONDETAILS_H

#include <QHostAddress>
#include <QSerialPort>
#include <QModbusDevice>
#include <QDataStream>
#include <QSettings>
#include "enums.h"

///
/// \brief The TcpConnectionParams class
///
struct TcpConnectionParams
{
    quint16 ServicePort = 502;
    const QString IPAddress = "0.0.0.0";

    void normalize()
    {
        ServicePort = qMax<quint16>(1, ServicePort);
    }

    friend bool operator==(const TcpConnectionParams& params1, const TcpConnectionParams& params2) noexcept
    {
        return params1.ServicePort == params2.ServicePort && params1.IPAddress == params2.IPAddress;
    }
};
Q_DECLARE_METATYPE(TcpConnectionParams)

///
/// \brief operator <<
/// \param out
/// \param params
/// \return
///
inline QDataStream& operator <<(QDataStream& out, const TcpConnectionParams& params)
{
    out << params.ServicePort;
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param params
/// \return
///
inline QDataStream& operator >>(QDataStream& in, TcpConnectionParams& params)
{
    in >> params.ServicePort;
    params.normalize();
    return in;
}

///
/// \brief operator <<
/// \param out
/// \param params
/// \return
///
inline QSettings& operator <<(QSettings& out, const TcpConnectionParams& params)
{
    out.setValue("TcpParams/ServicePort",   params.ServicePort);
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param params
/// \return
///
inline QSettings& operator >>(QSettings& in, TcpConnectionParams& params)
{
    params.ServicePort = in.value("TcpParams/ServicePort", 502).toUInt();
    params.normalize();
    return in;
}

///
/// \brief The SerialConnectionParams class
///
struct SerialConnectionParams
{
    QString PortName;
    QSerialPort::BaudRate BaudRate = QSerialPort::Baud9600;
    QSerialPort::DataBits WordLength = QSerialPort::Data8;
    QSerialPort::Parity Parity = QSerialPort::NoParity;
    QSerialPort::StopBits StopBits = QSerialPort::OneStop;
    QSerialPort::FlowControl FlowControl = QSerialPort::NoFlowControl;
    bool SetDTR = true;
    bool SetRTS = true;

    void normalize()
    {
        BaudRate = qBound(QSerialPort::Baud1200, BaudRate, QSerialPort::Baud115200);
        WordLength = qBound(QSerialPort::Data5, WordLength, QSerialPort::Data8);
        Parity = qBound(QSerialPort::NoParity, Parity, QSerialPort::MarkParity);
        FlowControl = qBound(QSerialPort::NoFlowControl, FlowControl, QSerialPort::SoftwareControl);
    }

    friend bool operator==(const SerialConnectionParams& params1, const SerialConnectionParams& params2) noexcept
    {
        return params1.PortName == params2.PortName &&
               params1.BaudRate == params2.BaudRate &&
               params1.WordLength == params2.WordLength &&
               params1.Parity == params2.Parity &&
               params1.StopBits == params2.StopBits &&
               params1.FlowControl == params2.FlowControl &&
               params1.SetDTR == params2.SetDTR &&
               params1.SetRTS == params2.SetRTS;
    }
};
Q_DECLARE_METATYPE(SerialConnectionParams)

///
/// \brief operator <<
/// \param out
/// \param params
/// \return
///
inline QDataStream& operator <<(QDataStream& out, const SerialConnectionParams& params)
{
    out << params.PortName;
    out << params.BaudRate;
    out << params.WordLength;
    out << params.Parity;
    out << params.StopBits;
    out << params.FlowControl;
    out << params.SetDTR;
    out << params.SetRTS;

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param params
/// \return
///
inline QDataStream& operator >>(QDataStream& in, SerialConnectionParams& params)
{
    in >> params.PortName;
    in >> params.BaudRate;
    in >> params.WordLength;
    in >> params.Parity;
    in >> params.StopBits;
    in >> params.FlowControl;
    in >> params.SetDTR;
    in >> params.SetRTS;

    params.normalize();
    return in;
}

///
/// \brief operator <<
/// \param out
/// \param params
/// \return
///
inline QSettings& operator <<(QSettings& out, const SerialConnectionParams& params)
{
    out.setValue("SerialParams/PortName",       params.PortName);
    out.setValue("SerialParams/BaudRate",       params.BaudRate);
    out.setValue("SerialParams/WordLength",     params.WordLength);
    out.setValue("SerialParams/Parity",         params.Parity);
    out.setValue("SerialParams/FlowControl",    params.FlowControl);
    out.setValue("SerialParams/DTR",            params.SetDTR);
    out.setValue("SerialParams/RTS",            params.SetRTS);

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param params
/// \return
///
inline QSettings& operator >>(QSettings& in, SerialConnectionParams& params)
{
    params.PortName    = in.value("SerialParams/PortName").toString();
    params.BaudRate    = (QSerialPort::BaudRate)in.value("SerialParams/BaudRate", 9600).toUInt();
    params.WordLength  = (QSerialPort::DataBits)in.value("SerialParams/WordLength", 8).toUInt();
    params.Parity      = (QSerialPort::Parity)in.value("SerialParams/Parity", 0).toUInt();
    params.FlowControl = (QSerialPort::FlowControl)in.value("SerialParams/FlowControl", 0).toUInt();
    params.SetDTR      = in.value("SerialParams/DTR", false).toBool();
    params.SetRTS      = in.value("SerialParams/RTS", false).toBool();

    params.normalize();
    return in;
}

///
/// \brief The ConnectionDetails class
///
struct ConnectionDetails
{
    ConnectionType Type = ConnectionType::Tcp;
    TcpConnectionParams TcpParams;
    SerialConnectionParams SerialParams;

    friend bool operator==(const ConnectionDetails& cd1, const ConnectionDetails& cd2) noexcept
    {
        switch(cd1.Type)
        {
            case ConnectionType::Tcp:
            return cd2.Type == ConnectionType::Tcp && cd1.TcpParams == cd2.TcpParams;

            case ConnectionType::Serial:
            return cd2.Type == ConnectionType::Serial && cd1.SerialParams == cd2.SerialParams;
        }

        return false;
    }
};
Q_DECLARE_METATYPE(ConnectionDetails)

///
/// \brief operator <<
/// \param out
/// \param params
/// \return
///
inline QDataStream& operator <<(QDataStream& out, const ConnectionDetails& params)
{
    out << params.Type;
    out << params.TcpParams;
    out << params.SerialParams;

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param params
/// \return
///
inline QDataStream& operator >>(QDataStream& in, ConnectionDetails& params)
{
    in >> params.Type;
    in >> params.TcpParams;
    in >> params.SerialParams;

    return in;
}

///
/// \brief operator <<
/// \param out
/// \param params
/// \return
///
inline QSettings& operator <<(QSettings& out, const ConnectionDetails& params)
{
    out.setValue("ConnectionParams/Type", (uint)params.Type);
    out << params.TcpParams;
    out << params.SerialParams;

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param params
/// \return
///
inline QSettings& operator >>(QSettings& in, ConnectionDetails& params)
{
    params.Type = (ConnectionType)in.value("ConnectionParams/Type", 0).toUInt();
    in >> params.TcpParams;
    in >> params.SerialParams;

    return in;
}

#endif // CONNECTIONDETAILS_H
