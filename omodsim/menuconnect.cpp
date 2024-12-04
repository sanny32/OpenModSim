#include <QEvent>
#include "serialportutils.h"
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
    if(type == MenuType::ConnectMenu)
    {
        addAction(tr("Modbus/TCP Srv"), ConnectionType::Tcp, QString());
        for(auto&& port: getAvailableSerialPorts())
        {
            const auto text = QString(tr("Port %1")).arg(port);
            addAction(text, ConnectionType::Serial, port);
        }
    }

    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, [&](const ConnectionDetails& cd)
    {
        if(_menuType == MenuType::DisconnectMenu)
        {
            switch(cd.Type)
            {
                case ConnectionType::Tcp:
                {
                    const auto port = QString("%1:%2").arg(cd.TcpParams.IPAddress, QString::number(cd.TcpParams.ServicePort));
                    addAction(QString(tr("Modbus/TCP Srv %1")).arg(port), ConnectionType::Tcp, port);
                }
                break;

                case ConnectionType::Serial:
                    addAction(QString(tr("Port %1")).arg(cd.SerialParams.PortName), ConnectionType::Serial, cd.SerialParams.PortName);
                break;
            }
        }
        else
        {
            for(auto&& a : actions())
            {
                const auto data = a->data().value<QPair<ConnectionType, QString>>();
                const bool isConnected = _mbMultiServer.isConnected(data.first, data.second);
                a->setEnabled(_menuType == ConnectMenu ? !isConnected : isConnected);
            }
        }
    });

    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, [&](const ConnectionDetails& cd)
    {
        for(auto&& a : actions())
        {
            const auto data = a->data().value<QPair<ConnectionType, QString>>();
            if(_menuType == MenuType::DisconnectMenu)
            {
                const auto port = (data.first == ConnectionType::Tcp) ?
                    QString("%1:%2").arg(cd.TcpParams.IPAddress, QString::number(cd.TcpParams.ServicePort)) :
                    cd.SerialParams.PortName;

                if(data.first == cd.Type && data.second == port)
                {
                    removeAction(a);
                }
            }
            else
            {
                const bool isConnected = _mbMultiServer.isConnected(data.first, data.second);
                a->setEnabled(_menuType == ConnectMenu ? !isConnected : isConnected);
            }
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
}
