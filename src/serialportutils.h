#ifndef SERIALPORTUTILS_H
#define SERIALPORTUTILS_H

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QRegularExpression>


///
/// \brief Parity_toString
/// \param parity
/// \return
///
inline QString Parity_toString(QSerialPort::Parity parity)
{
    switch(parity)
    {
    case QSerialPort::NoParity:
        return QSerialPort::tr("NONE");

    case QSerialPort::EvenParity:
        return QSerialPort::tr("EVEN");

    case QSerialPort::OddParity:
        return QSerialPort::tr("ODD");

    case QSerialPort::SpaceParity:
        return QSerialPort::tr("SPACE");

    case QSerialPort::MarkParity:
        return QSerialPort::tr("MARK");

    default:
        break;
    }

    return QString();
}

///
/// \brief getAvailableSerialPorts
/// \return
///
inline QStringList getAvailableSerialPorts()
{
    QStringList ports;
    for(auto&& port: QSerialPortInfo::availablePorts())
        ports << port.portName();

    static QRegularExpression re( "[^\\d]");
    std::sort(ports.begin(), ports.end(), [](QString p1, QString p2)
              {
                  return p1.remove(re).toInt() < p2.remove(re).toInt();
              });

    return ports;
}

#endif // SERIALPORTUTILS_H
