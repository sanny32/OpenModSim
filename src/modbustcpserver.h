#ifndef MODBUSTCPSERVER_H
#define MODBUSTCPSERVER_H

#include <QList>
#include <QMap>
#include <QPointer>
#include <QTcpServer>
#include <QModbusTcpConnectionObserver>
#include "modbusserver.h"

class QTimer;

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
    int connectedClientCount() const { return _connections.size(); }

    QIODevice *device() const override {return nullptr; }

signals:
    void modbusClientConnected(const QString& clientAddress, quint16 clientPort);
    void modbusClientDisconnected(const QString& clientAddress, quint16 clientPort);

private slots:
    void on_newConnection();
    void on_rawDataReceived(const QDateTime& time, const QByteArray& data);
    void on_acceptError(QAbstractSocket::SocketError);

protected:
    bool open() override;
    void close() override;

private:
    struct PendingTcpResponse {
        QPointer<QTcpSocket> Socket;
        quint16 TransactionId = 0;
        quint16 ProtocolId = 0;
        quint8 UnitId = 0;
        QModbusResponse Response;
        QSharedPointer<const ModbusMessage> RequestMessage;
    };

    void sendTcpResponse(const PendingTcpResponse& pending);
    void enqueueDelayedResponse(const PendingTcpResponse& pending, int delayMs);
    void flushDelayedResponses();
    void armDelayedResponseTimer();
    QModbusResponse forwardProcessRequest(const QModbusRequest &r, int serverAddress);

private:
    int _networkPort = 502;
    QString _networkAddress = QStringLiteral("127.0.0.1");

    QTcpServer* _server = nullptr;
    QVector<QTcpSocket*> _connections;
    QModbusTcpConnectionObserver* _observer = nullptr;

    static const qint8 mbpaHeaderSize = 7;
    static const qint16 maxBytesModbusADU = 260;

    QTimer* _delayedResponseTimer = nullptr;
    QMap<qint64, QList<PendingTcpResponse>> _delayedResponses;
    int _delayedResponseCount = 0;
    int _maxDelayedResponses = 2048;
    bool _delayedResponseOverflowLogged = false;
};


#endif // MODBUSTCPSERVER_H

