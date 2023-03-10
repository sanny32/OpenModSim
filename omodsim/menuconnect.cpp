#include <QEvent>
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
    addAction(tr("Modbus/TCP Srv"), ConnectionType::Tcp, QString());

    for(auto&& port: QSerialPortInfo::availablePorts())
    {
        const auto text = QString(tr("Port %1")).arg(port.portName());
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
/// \brief MenuConnect::changeEvent
/// \param event
///
void MenuConnect::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        for(auto&& a : actions())
        {
            const auto data = a->data().value<QPair<ConnectionType, QString>>();
            switch(data.first)
            {
                case ConnectionType::Tcp:
                    a->setText(tr("Modbus/TCP Srv"));
                break;
                case ConnectionType::Serial:
                    a->setText(QString(tr("Port %1")).arg(data.second));
                break;
            }
        }
    }

    QMenu::changeEvent(event);
}

///
/// \brief MenuConnect::canConnect
/// \param cd
/// \return
///
bool MenuConnect::canConnect(const ConnectionDetails& cd)
{
    for(auto& c : _connectionDetailsMap)
    {
        if(c.Type != cd.Type) continue;
        if(c.Type == ConnectionType::Tcp ||
          (c.Type == ConnectionType::Serial && c.SerialParams.PortName == cd.SerialParams.PortName))
        {
            return true;
        }
    }

    return false;
}

///
/// \brief MenuConnect::updateConnectionDetails
/// \param conns
///
void MenuConnect::updateConnectionDetails(const QList<ConnectionDetails>& conns)
{
    if(_menuType != ConnectMenu)
        return;

    for(auto&& cd : conns)
    {
        for(auto& c : _connectionDetailsMap)
        {
            if(cd.Type != c.Type) continue;
            if(c.Type == ConnectionType::Tcp ||
              (c.Type == ConnectionType::Serial && c.SerialParams.PortName == cd.SerialParams.PortName))
            {
                c = cd;
            }
        }
    }
}

///
/// \brief MenuConnect::addAction
/// \param text
/// \param type
/// \param port
///
void MenuConnect::addAction(const QString& text, ConnectionType type, const QString& port)
{
    auto action = QMenu::addAction(text);
    connect(action, &QAction::triggered, this, [this, action, type, port]
    {
        if(_menuType == ConnectMenu)
            emit connectAction(_connectionDetailsMap[action]);
        else
            emit disconnectAction(type, port);
    });

    if(_menuType == ConnectMenu)
    {
        ConnectionDetails cd;
        cd.Type = type;
        if(type == ConnectionType::Serial)
            cd.SerialParams.PortName = port;

        _connectionDetailsMap[action] = cd;
    }

    const auto data = QPair<ConnectionType, QString>(type, port);
    action->setData(QVariant::fromValue(data));
    action->setEnabled(_menuType == ConnectMenu);
}
