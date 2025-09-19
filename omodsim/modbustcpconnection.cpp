#include <QTcpSocket>
#include "modbustcpconnection.h"
#include "modbustcpserver.h"

///
/// \brief ModbusTcpConnection::ModbusTcpConnection
/// \param socketDescriptor
/// \param backend
/// \param parent
///
ModbusTcpConnection::ModbusTcpConnection(qintptr socketDescriptor, ModbusTcpServer* backend, QObject* parent)
    : QObject{parent}
    ,_backend(backend)
{
    _socket = new QTcpSocket(this);
    connect(_socket, &QTcpSocket::readyRead, this, &ModbusTcpConnection::onReadyRead);
    connect(_socket, &QTcpSocket::disconnected, this, &ModbusTcpConnection::onDisconnected);
    connect(_socket, &QTcpSocket::bytesWritten, this, &ModbusTcpConnection::onBytesWritten);

    if (!_socket->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "(TCP server) Failed to set socket descriptor";
        deleteLater();
        return;
    }

    qDebug() << "(TCP server) New Modbus TCP connection from" << _socket->peerAddress().toString();
}

///
/// \brief ModbusTcpConnection::onReadyRead
///
void ModbusTcpConnection::onReadyRead()
{
    if (_isProcessing) return;

    _buffer.append(_socket->readAll());

     while (!_buffer.isEmpty())
    {
        if (_buffer.size() < mbpaHeaderSize) {
            qDebug() << "(TCP server) MBPA header too short. Waiting for more data.";
            return;
        }

        processModbusMessage();
    }
}

///
/// \brief ModbusTcpConnection::processModbusMessage
/// \param rawMessage
///
void ModbusTcpConnection::processModbusMessage()
{
    quint8 unitId;
    quint16 transactionId, bytesPdu, protocolId;

    QDataStream input(_buffer);
    input >> transactionId >> protocolId >> bytesPdu >> unitId;

    qDebug() << "(TCP server) Request MBPA:" << "Transaction Id:"
                << Qt::hex << transactionId << "Protocol Id:" << protocolId << "PDU bytes:"
                << bytesPdu << "Unit Id:" << unitId;

    bytesPdu--;

    const quint16 current = mbpaHeaderSize + bytesPdu;
    if (_buffer.size() < current) {
        qDebug() << "(TCP server) PDU too short. Waiting for more data";
        return;
    }

    QModbusRequest request;
    input >> request;

    _buffer.remove(0, current);

    QModbusResponse response;
    if (_backend->value(QModbusServer::DeviceBusy).value<quint16>() == 0xffff) {
        response = QModbusExceptionResponse(request.functionCode(), QModbusExceptionResponse::ServerDeviceBusy);
    }
    else {
        response = _backend->processRequest(request);
    }

    QByteArray result;
    QDataStream output(&result, QIODevice::WriteOnly);

    // The length field is the byte count of the following fields, including the Unit
    // Identifier and PDU fields, so we add one byte to the response size.
    output << transactionId << protocolId << quint16(response.size() + 1)
           << unitId << response;

    if (!_socket->isOpen()) {
        qDebug() << "(TCP server) Requesting socket has closed.";
        _backend->setError(QModbusTcpServer::tr("Requesting socket is closed"), QModbusDevice::WriteError);
        return;
    }

    qint64 writtenBytes = _socket->write(result);
    if (writtenBytes == -1 || writtenBytes < result.size()) {
        qDebug() << "(TCP server) Cannot write requested response to socket.";
        _backend->setError(QModbusTcpServer::tr("Could not write response to client"), QModbusDevice::WriteError);
    }
}

///
/// \brief ModbusTcpConnection::onDisconnected
///
void ModbusTcpConnection::onDisconnected()
{
    qDebug() << "(TCP server) Client disconnected";
    deleteLater();
}

///
/// \brief ModbusTcpConnection::onBytesWritten
/// \param bytes
///
void ModbusTcpConnection::onBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes)
}


