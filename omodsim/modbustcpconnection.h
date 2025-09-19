#ifndef MODBUSTCPCONNECTION_H
#define MODBUSTCPCONNECTION_H

#include <QTimer>
#include <QModbusTcpServer>

///
/// \brief The ModbusTcpConnection class
///
class ModbusTcpConnection : public QObject
{
    Q_OBJECT

public:
    explicit ModbusTcpConnection(qintptr socketDescriptor, QModbusTcpServer* backend, QObject* parent = nullptr);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onBytesWritten(qint64 bytes);

private:
    void processModbusMessage(const QByteArray &data);
    quint16 readUInt16(const QByteArray &data, int offset) const;
    void writeUInt16(QByteArray &data, int offset, quint16 value) const;

private:
    QTcpSocket* _socket = nullptr;
    QModbusTcpServer* _backend = nullptr;

    QByteArray _buffer;
    bool _isProcessing = false;

    QTimer _responseTimer;
};

#endif // MODBUSTCPCONNECTION_H
