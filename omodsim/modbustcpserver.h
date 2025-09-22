#ifndef MODBUSTCPSERVER_H
#define MODBUSTCPSERVER_H

#include <QTcpServer>
#include <QModbusTcpConnectionObserver>
#include "modbusserver.h"

///
/// \brief The ModbusTcpServer class
///
class ModbusTcpServer : public ModbusServer
{
    Q_OBJECT

public:
    explicit ModbusTcpServer(QObject *parent = nullptr);

    QVariant connectionParameter(ConnectionParameter parameter) const override;
    void setConnectionParameter(ConnectionParameter parameter, const QVariant &value) override;

signals:
    void request(const QModbusRequest& req, int transactionId);
    void response(const QModbusRequest& req, const QModbusResponse& resp, int transactionId);
    void modbusClientDisconnected(QTcpSocket* modbusClient);

private slots:
    void on_newConnection();
    void on_acceptError(QAbstractSocket::SocketError);

protected:
    bool open() override;
    void close() override;

    QModbusResponse processRequest(const QModbusPdu &req) override;

private:
    bool matchingServerAddress(quint8 unitId) const;
    QModbusResponse forwardProcessRequest(const QModbusRequest &r);

private:
    int _networkPort = 502;
    QString _networkAddress = QStringLiteral("127.0.0.1");

    int _transactionId = 0;
    QTcpServer* _server = nullptr;
    QModbusTcpConnectionObserver* _observer = nullptr;

    static const qint8 mbpaHeaderSize = 7;
    static const qint16 maxBytesModbusADU = 260;
};


#endif // MODBUSTCPSERVER_H
