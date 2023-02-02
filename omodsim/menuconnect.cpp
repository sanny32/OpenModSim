#include <QSerialPortInfo>
#include "menuconnect.h"

///
/// \brief MenuConnect::MenuConnect
/// \param type
/// \param server
/// \param parent
///
MenuConnect::MenuConnect(MenuType type, ModbusMultiServer& server, QWidget *parent)
    : QMenu(parent)
    ,_menuType(type)
    ,_mbMultiServer(server)
{
    addAction("Modbus/TCP Srv", ConnectionType::Tcp, QString());

    for(auto&& port: QSerialPortInfo::availablePorts())
    {
        const auto text = QString("Port %1").arg(port.portName());
        addAction(text, ConnectionType::Serial, port.portName());
    }

    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, [&](const ConnectionDetails&)
    {
        for(auto&& a : actions())
        {
            const auto data = a->data().value<QPair<ConnectionType, QString>>();
            const bool isConnected = _mbMultiServer.isConnected(data.first, data.second);
            a->setEnabled(_menuType == ConnectMenu ? !isConnected : isConnected);
        }
    });

    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, [&](const ConnectionDetails&)
    {
        for(auto&& a : actions())
        {
            const auto data = a->data().value<QPair<ConnectionType, QString>>();
            const bool isConnected = _mbMultiServer.isConnected(data.first, data.second);
            a->setEnabled(_menuType == ConnectMenu ? !isConnected : isConnected);
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
        if(_menuType == ConnectMenu)
            emit connectAction(type, port);
        else
            emit disconnectAction(type, port);
    });

    const auto data = QPair<ConnectionType, QString>(type, port);
    action->setData(QVariant::fromValue(data));
    action->setEnabled(_menuType == ConnectMenu);
}
