#include "modbustcpconnection.h"
#include "modbustcpproxyserver.h"
#include "modbustcpserver.h"

///
/// \brief ModbusTcpProxyServer::ModbusTcpProxyServer
/// \param parent
///
ModbusTcpProxyServer::ModbusTcpProxyServer(QObject* parent)
    : QTcpServer(parent)
{
}

///
/// \brief ModbusTcpProxyServer::~ModbusTcpProxyServer
///
ModbusTcpProxyServer::~ModbusTcpProxyServer()
{
    if (_backendServer) {
        _backendServer->disconnect();
    }
}

///
/// \brief ModbusTcpProxyServer::startProxy
/// \param port
/// \param address
/// \return
///
bool ModbusTcpProxyServer::startProxy(quint16 port, const QHostAddress& address)
{
    return listen(address, port);
}

///
/// \brief ModbusTcpProxyServer::setBackendServer
/// \param server
///
void ModbusTcpProxyServer::setBackendServer(ModbusTcpServer* server)
{
    _backendServer = server;
}

///
/// \brief ModbusTcpProxyServer::incomingConnection
/// \param socketDescriptor
///
void ModbusTcpProxyServer::incomingConnection(qintptr socketDescriptor)
{
    if (_backendServer) {
        return;
    }

    new ModbusTcpConnection(socketDescriptor, _backendServer, this);
}
