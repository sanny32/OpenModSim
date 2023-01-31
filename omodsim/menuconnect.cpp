#include <QSerialPortInfo>
#include "menuconnect.h"

///
/// \brief MenuConnect::MenuConnect
/// \param parent
///
MenuConnect::MenuConnect(QWidget *parent)
    : QMenu(parent)
{
    addAction("Modbus/TCP Srv", [=]{
        emit connectAction(ConnectionType::Tcp, QString());
    });

    QStringList ports;
    for(auto&& port: QSerialPortInfo::availablePorts())
    {
        const auto text = QString("Port %1").arg(port.portName());
        addAction(text, [this, port]{
            emit connectAction(ConnectionType::Serial, port.portName());
        });
    }
}
