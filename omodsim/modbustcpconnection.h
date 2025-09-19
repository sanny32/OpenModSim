#ifndef MODBUSTCPCONNECTION_H
#define MODBUSTCPCONNECTION_H

#include <QTimer>
#include <QModbusTcpServer>

class ModbusTcpServer;

///
/// \brief The ModbusTcpConnection class
///
class ModbusTcpConnection : public QObject
{
    Q_OBJECT

public:
    explicit ModbusTcpConnection(qintptr socketDescriptor, ModbusTcpServer* backend, QObject* parent = nullptr);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onBytesWritten(qint64 bytes);

private:
    void processModbusMessage();

private:
    QTcpSocket* _socket = nullptr;
    ModbusTcpServer* _backend = nullptr;

    QByteArray _buffer;
    bool _isProcessing = false;

    QTimer _responseTimer;

    static const qint8 mbpaHeaderSize = 7;
    static const qint16 maxBytesModbusADU = 260;
};

#endif // MODBUSTCPCONNECTION_H
