#ifndef CONNECTIONDETAILS_H
#define CONNECTIONDETAILS_H

#include <QHostAddress>
#include <QSerialPort>
#include <QModbusDevice>
#include <QDataStream>
#include <QSettings>
#include <QXmlStreamWriter>
#include "enums.h"

///
/// \brief The TcpConnectionParams class
///
struct TcpConnectionParams
{
    quint16 ServicePort = 502;
    QString IPAddress = "0.0.0.0";

    void normalize()
    {
        const auto addr = QHostAddress(IPAddress);
        IPAddress = addr.isNull() ? "0.0.0.0" : addr.toString();
        ServicePort = qMax<quint16>(1, ServicePort);
    }

    TcpConnectionParams& operator=(const TcpConnectionParams& params)
    {
        IPAddress = params.IPAddress;
        ServicePort = params.ServicePort;
        return *this;
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
    out << params.IPAddress;
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
    in >> params.IPAddress;
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
    out.setValue("TcpParams/IPAddress",     params.IPAddress);
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
    params.IPAddress = in.value("TcpParams/IPAddress", "0.0.0.0").toString();
    params.ServicePort = in.value("TcpParams/ServicePort", 502).toUInt();
    params.normalize();
    return in;
}

///
/// \brief operator <<
/// \param xml
/// \param dd
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const TcpConnectionParams& params)
{
    xml.writeStartElement("TcpConnectionParams");
    xml.writeAttribute("IPAddress", params.IPAddress);
    xml.writeAttribute("ServicePort", QString::number(params.ServicePort));
    xml.writeEndElement();

    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param params
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, TcpConnectionParams& params)
{
    if (xml.isStartElement() && xml.name() == QLatin1String("TcpConnectionParams")) {
        const QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("IPAddress")) {
            params.IPAddress = attributes.value("IPAddress").toString();
        }

        if (attributes.hasAttribute("ServicePort")) {
            bool ok; const quint16 port = attributes.value("ServicePort").toUShort(&ok);
            if (ok) params.ServicePort = port;
        }

        xml.skipCurrentElement();

        params.normalize();
    }

    return xml;
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

    SerialConnectionParams& operator=(const SerialConnectionParams& params)
    {
        PortName = params.PortName;
        BaudRate = params.BaudRate;
        WordLength = params.WordLength;
        Parity = params.Parity;
        StopBits = params.StopBits;
        FlowControl = params.FlowControl;
        SetDTR = params.SetDTR;
        SetRTS = params.SetRTS;
        return *this;
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
DECLARE_ENUM_STRINGS(QSerialPort::Parity,
                     {   QSerialPort::NoParity,     "NO"        },
                     {   QSerialPort::EvenParity,   "EVEN"      },
                     {   QSerialPort::OddParity,    "ODD"       },
                     {   QSerialPort::SpaceParity,  "SPACE"     },
                     {   QSerialPort::MarkParity,   "MARK"      }
)
DECLARE_ENUM_STRINGS(QSerialPort::StopBits,
                     {   QSerialPort::OneStop,          "1"         },
                     {   QSerialPort::OneAndHalfStop,   "1.5"       },
                     {   QSerialPort::TwoStop,          "2"         }
)
DECLARE_ENUM_STRINGS(QSerialPort::FlowControl,
                     {   QSerialPort::NoFlowControl,        "NO"        },
                     {   QSerialPort::HardwareControl,      "HARDWARE"  },
                     {   QSerialPort::SoftwareControl,      "SOFTWARE"  }
)

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
    out.setValue("SerialParams/StopBits",       params.StopBits);
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
    params.StopBits    = (QSerialPort::StopBits)in.value("SerialParams/StopBits", 1).toUInt();
    params.FlowControl = (QSerialPort::FlowControl)in.value("SerialParams/FlowControl", 0).toUInt();
    params.SetDTR      = in.value("SerialParams/DTR", false).toBool();
    params.SetRTS      = in.value("SerialParams/RTS", false).toBool();

    params.normalize();
    return in;
}

///
/// \brief operator <<
/// \param xml
/// \param params
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const SerialConnectionParams& params)
{
    xml.writeStartElement("SerialConnectionParams");

    xml.writeAttribute("PortName", params.PortName);
    xml.writeAttribute("BaudRate", QString::number(params.BaudRate));
    xml.writeAttribute("DataBits", QString::number(params.WordLength));
    xml.writeAttribute("Parity", enumToString(params.Parity));
    xml.writeAttribute("StopBits", enumToString(params.StopBits));
    xml.writeAttribute("FlowControl", enumToString(params.FlowControl));
    xml.writeAttribute("SetDTR", boolToString(params.SetDTR));
    xml.writeAttribute("SetRTS", boolToString(params.SetRTS));

    xml.writeEndElement();
    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param params
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, SerialConnectionParams& params)
{
    if (xml.isStartElement() && xml.name() == QLatin1String("SerialConnectionParams")) {
        const QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("PortName")) {
            params.PortName = attributes.value("PortName").toString();
        }

        if (attributes.hasAttribute("BaudRate")) {
            bool ok; const auto baudRate = attributes.value("ServicePort").toUInt(&ok);
            if (ok) params.BaudRate = static_cast<QSerialPort::BaudRate>(baudRate);
        }

        if (attributes.hasAttribute("DataBits")) {
            bool ok; const auto wordLength = attributes.value("DataBits").toUInt(&ok);
            if (ok) params.WordLength = static_cast<QSerialPort::DataBits>(wordLength);
        }

        if (attributes.hasAttribute("Parity")) {
            params.Parity = enumFromString<QSerialPort::Parity>(attributes.value("Parity").toString());
        }

        if (attributes.hasAttribute("StopBits")) {
            params.StopBits = enumFromString<QSerialPort::StopBits>(attributes.value("StopBits").toString());
        }

        if (attributes.hasAttribute("FlowControl")) {
            params.FlowControl = enumFromString<QSerialPort::FlowControl>(attributes.value("FlowControl").toString());
        }

        if (attributes.hasAttribute("SetDTR")) {
            params.SetDTR = stringToBool(attributes.value("SetDTR").toString());
        }

        if (attributes.hasAttribute("SetRTS")) {
            params.SetRTS = stringToBool(attributes.value("SetRTS").toString());
        }

        xml.skipCurrentElement();

        params.normalize();
    }

    return xml;
}

///
/// \brief The ConnectionDetails class
///
struct ConnectionDetails
{
    ConnectionType Type = ConnectionType::Tcp;
    TcpConnectionParams TcpParams;
    SerialConnectionParams SerialParams;

    ConnectionDetails& operator=(const ConnectionDetails& cd) noexcept
    {
        Type = cd.Type;
        TcpParams = cd.TcpParams;
        SerialParams = cd.SerialParams;
        return *this;
    }

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

///
/// \brief operator <<
/// \param xml
/// \param cd
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const ConnectionDetails& cd)
{
    xml.writeStartElement("ConnectionDetails");
    xml.writeAttribute("ConnectionType", enumToString(cd.Type));

    switch(cd.Type) {
        case ConnectionType::Tcp:
            xml << cd.TcpParams;
            break;
        case ConnectionType::Serial:
            xml << cd.SerialParams;
            break;
    }

    xml.writeEndElement();
    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param cd
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, ConnectionDetails& cd)
{
    if (xml.isStartElement() && xml.name() == QLatin1String("ConnectionDetails")) {
        const QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("ConnectionType")) {
            cd.Type = enumFromString<ConnectionType>(attributes.value("ConnectionType").toString());
        }

        switch(cd.Type) {
            case ConnectionType::Tcp:
                if(xml.readNextStartElement() && xml.name() == QLatin1String("TcpConnectionParams")) {
                    xml >> cd.TcpParams;
                }
                break;
            case ConnectionType::Serial:
                if(xml.readNextStartElement() && xml.name() == QLatin1String("SerialConnectionParams")) {
                    xml >> cd.SerialParams;
                }
                break;
        }

        xml.skipCurrentElement();
    }

    return xml;
}

#endif // CONNECTIONDETAILS_H
