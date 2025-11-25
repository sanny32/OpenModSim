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
    ~ModbusTcpServer();

    QVariant connectionParameter(QModbusDevice::ConnectionParameter parameter) const override;
    void setConnectionParameter(QModbusDevice::ConnectionParameter parameter, const QVariant &value) override;

    QIODevice *device() const override {return nullptr; }

signals:
    void modbusClientDisconnected(QTcpSocket* modbusClient);

private slots:
    void on_newConnection();
    void on_rawDataReceived(const QByteArray& data);
    void on_acceptError(QAbstractSocket::SocketError);

protected:
    bool open() override;
    void close() override;

private:
    QModbusResponse forwardProcessRequest(const QModbusRequest &r, int serverAddress);

private:
    int _networkPort = 502;
    QString _networkAddress = QStringLiteral("127.0.0.1");

    QTcpServer* _server = nullptr;
    QVector<QTcpSocket*> _connections;
    QModbusTcpConnectionObserver* _observer = nullptr;

    static const qint8 mbpaHeaderSize = 7;
    static const qint16 maxBytesModbusADU = 260;
};


#endif // MODBUSTCPSERVER_H
