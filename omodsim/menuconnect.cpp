#include <QSerialPortInfo>
#include "menuconnect.h"

///
/// \brief MenuConnect::MenuConnect
/// \param parent
///
MenuConnect::MenuConnect(ModbusMultiServer& server, QWidget *parent)
    : QMenu(parent)
    ,_mbMultiServer(server)
{
    addAction("Modbus/TCP Srv", ConnectionType::Tcp, QString());

    for(auto&& port: QSerialPortInfo::availablePorts())
    {
        const auto text = QString("Port %1").arg(port.portName());
        addAction(text, ConnectionType::Serial, port.portName());
    }

    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, [&](const ConnectionDetails& cd)
    {
        for(auto&& a : actions())
        {
            const auto data = a->data().value<QPair<ConnectionType, QString>>();
            if(data.first == cd.Type)
            {
                a->setEnabled(data.first != ConnectionType::Tcp && data.second == cd.SerialParams.PortName);
            }
        }
    });

    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, [&](const ConnectionDetails& cd)
    {
        for(auto&& a : actions())
        {
            const auto data = a->data().value<QPair<ConnectionType, QString>>();
            if(data.first == cd.Type)
            {
                a->setEnabled(data.first == ConnectionType::Tcp || data.second != cd.SerialParams.PortName);
            }
        }
    });
}

///
/// \brief MenuConnect::addAction
/// \param text
/// \param type
/// \param port
///
void MenuConnect::addAction(const QString& text, ConnectionType type, const QString& port)
{
    auto action = QMenu::addAction(text, [this, type, port]{
        emit connectAction(type, port);
    });

    const auto data = QPair<ConnectionType, QString>(type, port);
    action->setData(QVariant::fromValue(data));
}
