#ifndef MODBUSTCPPROXYSERVER_H
#define MODBUSTCPPROXYSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QModbusTcpServer>

///
/// \brief The ModbusTcpProxyServer class
///
class ModbusTcpProxyServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit ModbusTcpProxyServer(QObject* parent = nullptr);
    ~ModbusTcpProxyServer();

    bool startProxy(quint16 port = 502, const QHostAddress& address = QHostAddress::Any);
    void setBackendServer(QModbusTcpServer* server);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QModbusTcpServer* _backendServer = nullptr;
};

#endif // MODBUSTCPPROXYSERVER_H
