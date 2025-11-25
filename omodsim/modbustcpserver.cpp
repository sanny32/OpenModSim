#include <QUrl>
#include <QTimer>
#include <QTcpSocket>
#include <QRandomGenerator>
#include "modbustcpserver.h"

///
/// \brief ModbusTcpServer::ModbusTcpServer
/// \param parent
///
ModbusTcpServer::ModbusTcpServer(QObject *parent)
    : ModbusServer(parent)
{
    _server = new QTcpServer(this);

    QObject::connect(_server, &QTcpServer::newConnection, this, &ModbusTcpServer::on_newConnection);
    QObject::connect(_server, &QTcpServer::acceptError, this, &ModbusTcpServer::on_acceptError);
    QObject::connect(this, &ModbusServer::rawDataReceived, this, &ModbusTcpServer::on_rawDataReceived);
}

///
/// \brief ModbusTcpServer::~ModbusTcpServer
///
ModbusTcpServer::~ModbusTcpServer()
{
    close();
}

///
/// \brief ModbusTcpServer::connectionParameter
/// \param parameter
/// \return
///
QVariant ModbusTcpServer::connectionParameter(QModbusDevice::ConnectionParameter parameter) const
{
    switch (parameter) {
    case QModbusDevice::NetworkPortParameter:
        return _networkPort;
    case QModbusDevice::NetworkAddressParameter:
        return _networkAddress;
    default:
        break;
    }
    return {};
}

///
/// \brief ModbusTcpServer::setConnectionParameter
/// \param parameter
/// \param value
///
void ModbusTcpServer::setConnectionParameter(QModbusDevice::ConnectionParameter parameter, const QVariant &value)
{
    switch (parameter) {
    case QModbusDevice::NetworkPortParameter:
        _networkPort = value.toInt();
        break;
    case QModbusDevice::NetworkAddressParameter:
        _networkAddress = value.toString();
        break;
    default:
        Q_ASSERT_X(false, "", "Connection parameter not supported.");
        break;
    }
}

///
/// \brief ModbusTcpServer::on_newConnection
///
void ModbusTcpServer::on_newConnection()
{
    auto *socket = _server->nextPendingConnection();
    if (!socket)
        return;

    qCDebug(QT_MODBUS) << "(TCP server) Incoming socket from" << socket->peerAddress()
                       << socket->peerName() << socket->peerPort();

    if (_observer && !_observer->acceptNewConnection(socket)) {
        qCDebug(QT_MODBUS) << "(TCP server) Connection rejected by observer";
        socket->close();
        socket->deleteLater();
        return;
    }

    _connections.append(socket);

    auto buffer = new QByteArray();

    QObject::connect(socket, &QObject::destroyed, socket, [buffer]() {
        // cleanup buffer
        delete buffer;
    });
    QObject::connect(socket, &QTcpSocket::disconnected, this, [socket, this]() {
        _connections.removeAll(socket);
        emit modbusClientDisconnected(socket);
        socket->deleteLater();
    });
    QObject::connect(socket, &QTcpSocket::readyRead, this, [buffer, socket, this]() {
        if (!socket)
            return;

        const ModbusDefinitions mbDef = getDefinitions();
        buffer->append(socket->readAll());

        while (!buffer->isEmpty()) {

            // emit raw data received
            emit rawDataReceived(*buffer);

            if (buffer->size() < mbpaHeaderSize) {
                qCDebug(QT_MODBUS) << "(TCP server) MBPA header too short. Waiting for more data.";
                return;
            }

            quint8 unitId;
            quint16 transactionId, bytesPdu, protocolId;
            QDataStream input(*buffer);
            input >> transactionId >> protocolId >> bytesPdu >> unitId;

            qCDebug(QT_MODBUS_LOW) << "(TCP server) Request MBPA:" << "Transaction Id:"
                                   << Qt::hex << transactionId << "Protocol Id:" << protocolId << "PDU bytes:"
                                   << bytesPdu << "Unit Id:" << unitId;

            // The length field is the byte count of the following fields, including the Unit
            // Identifier and the PDU, so we remove on byte.
            bytesPdu--;

            const quint16 current = mbpaHeaderSize + bytesPdu;
            if (buffer->size() < current) {
                qCDebug(QT_MODBUS) << "(TCP server) PDU too short. Waiting for more data";
                return;
            }

            QModbusRequest request;
            input >> request;

            buffer->remove(0, current);

            if (!matchingServerAddress(unitId))
                continue;

            qCDebug(QT_MODBUS) << "(TCP server) Request PDU:" << request;

            const auto msgReq = ModbusMessage::create(request, ModbusMessage::Tcp, unitId, transactionId, QDateTime::currentDateTime(), true);
            emit modbusRequest(msgReq);

            if(mbDef.ErrorSimulations.noResponse())
                return;

            const QModbusResponse response = forwardProcessRequest(request, unitId);

            if(mbDef.ErrorSimulations.responseIncorrectId())
                unitId++;

            int responseDelay = 0;
            if(mbDef.ErrorSimulations.responseDelay()) {
                responseDelay = mbDef.ErrorSimulations.responseDelayTime();
            }
            else if(mbDef.ErrorSimulations.responseRandomDelay()) {
                responseDelay = QRandomGenerator::global()->bounded(mbDef.ErrorSimulations.responseRandomDelayUpToTime());
            }

            QTimer::singleShot(responseDelay, this,
                               [this, socket, transactionId, protocolId, unitId, response, request, msgReq]()
            {
                qCDebug(QT_MODBUS) << "(TCP server) Response PDU:" << response;

                QByteArray result;
                QDataStream output(&result, QIODevice::WriteOnly);
                // The length field is the byte count of the following fields, including the Unit
                // Identifier and PDU fields, so we add one byte to the response size.
                output << transactionId << protocolId << quint16(response.size() + 1)
                       << unitId << response;

                if (!socket->isOpen()) {
                    qCDebug(QT_MODBUS) << "(TCP server) Requesting socket has closed.";
                    setError(QModbusTcpServer::tr("Requesting socket is closed"),
                                 QModbusDevice::WriteError);
                    return;
                }

                qint64 writtenBytes = socket->write(result);
                if (writtenBytes == -1 || writtenBytes < result.size()) {
                    qCDebug(QT_MODBUS) << "(TCP server) Cannot write requested response to socket.";
                    setError(QModbusTcpServer::tr("Could not write response to client"),
                                 QModbusDevice::WriteError);
                }
                else {
                    const auto msgResp = ModbusMessage::create(result, ModbusMessage::Tcp, QDateTime::currentDateTime(), false);
                    emit modbusResponse(msgReq, msgResp);
                }
            });
        }
    });
}

///
/// \brief ModbusTcpServer::on_rawDataReceived
/// \param data
///
void ModbusTcpServer::on_rawDataReceived(const QByteArray& data)
{
    qCDebug(QT_MODBUS_LOW).noquote() << "(TCP server) Received data: 0x" + data.toHex();
}

///
/// \brief ModbusTcpServer::on_acceptError
///
void ModbusTcpServer::on_acceptError(QAbstractSocket::SocketError)
{
    qCWarning(QT_MODBUS) << "(TCP server) Accept error";
    setError(_server->errorString(), QModbusDevice::ConnectionError);
}

///
/// \brief ModbusTcpServer::forwardProcessRequest
/// \param r
/// \param serverAddress
/// \return
///
QModbusResponse ModbusTcpServer::forwardProcessRequest(const QModbusRequest &r, int serverAddress)
{
    const ModbusDefinitions mbDef = getDefinitions();

    if(mbDef.ErrorSimulations.responseIllegalFunction()) {
        return QModbusExceptionResponse(r.functionCode(), QModbusExceptionResponse::IllegalFunction);
    }

    if (mbDef.ErrorSimulations.responseDeviceBusy() || value(QModbusServer::DeviceBusy, serverAddress).value<quint16>() == 0xffff) {
        // If the device is busy, send an exception response without processing.
        return QModbusExceptionResponse(r.functionCode(), QModbusExceptionResponse::ServerDeviceBusy);
    }

    QModbusResponse resp;
    switch (r.functionCode()) {
    case QModbusRequest::ReadExceptionStatus:
    case QModbusRequest::Diagnostics:
    case QModbusRequest::GetCommEventCounter:
    case QModbusRequest::GetCommEventLog:
    case QModbusRequest::ReportServerId:
        resp = QModbusExceptionResponse(r.functionCode(), QModbusExceptionResponse::IllegalFunction);
        break;
    default:
        resp = ModbusServer::processRequest(r, serverAddress);
        break;
    }

    return resp;
}

///
/// \brief ModbusTcpServer::open
/// \return
///
bool ModbusTcpServer::open()
{
    if (state() == QModbusDevice::ConnectedState)
        return true;

    if (_server->isListening())
        return false;

    const QUrl url = QUrl::fromUserInput(_networkAddress + QStringLiteral(":") + QString::number(_networkPort));

    if (!url.isValid()) {
        setError(tr("Invalid connection settings for TCP communication specified."), QModbusDevice::ConnectionError);
        qCWarning(QT_MODBUS) << "(TCP server) Invalid host:" << url.host() << "or port:"
                             << url.port();
        return false;
    }

    if (_server->listen(QHostAddress(url.host()), quint16(url.port())))
        setState(QModbusDevice::ConnectedState);
    else
        setError(_server->errorString(), QModbusDevice::ConnectionError);

    return state() == QModbusDevice::ConnectedState;
}

///
/// \brief ModbusTcpServer::close
///
void ModbusTcpServer::close()
{
    if (state() == QModbusDevice::UnconnectedState)
        return;

    if (_server->isListening())
        _server->close();

    for (auto socket : std::as_const(_connections))
        socket->disconnectFromHost();

    setState(QModbusDevice::UnconnectedState);
}
