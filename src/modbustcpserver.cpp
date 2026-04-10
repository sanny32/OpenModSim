#include <QUrl>
#include <QTimer>
#include <QPointer>
#include <QSharedPointer>
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
    _delayedResponseTimer = new QTimer(this);
    _delayedResponseTimer->setSingleShot(true);

    QObject::connect(_delayedResponseTimer, &QTimer::timeout, this, &ModbusTcpServer::flushDelayedResponses);
    QObject::connect(_server, &QTcpServer::newConnection, this, &ModbusTcpServer::on_newConnection);
    QObject::connect(_server, &QTcpServer::acceptError, this, &ModbusTcpServer::on_acceptError);
    QObject::connect(this, &ModbusServer::rawDataReceived, this, &ModbusTcpServer::on_rawDataReceived);
}

///
/// \brief ModbusTcpServer::~ModbusTcpServer
///
ModbusTcpServer::~ModbusTcpServer()
{
    ModbusTcpServer::close();
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
    emit modbusClientConnected(socket->peerAddress().toString(), socket->peerPort());

    auto buffer = new QByteArray();

///
/// \brief QObject::connect
///
    QObject::connect(socket, &QObject::destroyed, socket, [buffer]() {
        // cleanup buffer
        delete buffer;
    });
///
/// \brief QObject::connect
///
    QObject::connect(socket, &QTcpSocket::disconnected, this, [socket, this]() {
        _connections.removeAll(socket);
        emit modbusClientDisconnected(socket->peerAddress().toString(), socket->peerPort());
        socket->deleteLater();
    });
///
/// \brief QObject::connect
///
    QObject::connect(socket, &QTcpSocket::readyRead, this, [buffer, socket, this]() {
        if (!socket)
            return;

        const ModbusDefinitions mbDef = getDefinitions();
        buffer->append(socket->readAll());

        while (!buffer->isEmpty()) {

            const QDateTime time = QDateTime::currentDateTime();
            emit rawDataReceived(time, *buffer);

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

            const auto msgReq = ModbusMessage::create(request, ModbusMessage::Tcp, unitId, transactionId, time, true);
            emit modbusRequest(msgReq);

            if(mbDef.ErrorSimulations.noResponse())
                return;

            setCurrentRequestClient(socket->peerAddress().toString(), socket->peerPort());
            const QModbusResponse response = forwardProcessRequest(request, unitId);
            clearCurrentRequestClient();

            if(mbDef.ErrorSimulations.responseIncorrectId())
                unitId++;

            int responseDelay = 0;
            if(mbDef.ErrorSimulations.responseDelay()) {
                responseDelay = mbDef.ErrorSimulations.responseDelayTime();
            }
            else if(mbDef.ErrorSimulations.responseRandomDelay()) {
                responseDelay = QRandomGenerator::global()->bounded(mbDef.ErrorSimulations.responseRandomDelayUpToTime());
            }

            PendingTcpResponse pending;
            pending.Socket = QPointer<QTcpSocket>(socket);
            pending.TransactionId = transactionId;
            pending.ProtocolId = protocolId;
            pending.UnitId = unitId;
            pending.Response = response;
            pending.RequestMessage = msgReq;

            if (responseDelay <= 0) {
                sendTcpResponse(pending);
                continue;
            }

            if (_delayedResponseCount >= _maxDelayedResponses) {
                if (!_delayedResponseOverflowLogged) {
                    qCWarning(QT_MODBUS) << "(TCP server) Delayed response queue overflow:"
                                         << _delayedResponseCount
                                         << "limit =" << _maxDelayedResponses
                                         << ". Returning ServerDeviceBusy.";
                    _delayedResponseOverflowLogged = true;
                }

                PendingTcpResponse busy = pending;
                busy.Response = QModbusExceptionResponse(request.functionCode(),
                                                         QModbusExceptionResponse::ServerDeviceBusy);
                sendTcpResponse(busy);
                continue;
            }

            _delayedResponseOverflowLogged = false;
            enqueueDelayedResponse(pending, responseDelay);
        }
    });
}

void ModbusTcpServer::sendTcpResponse(const PendingTcpResponse& pending)
{
    if (!pending.Socket)
        return;

    qCDebug(QT_MODBUS) << "(TCP server) Response PDU:" << pending.Response;

    QByteArray result;
    QDataStream output(&result, QIODevice::WriteOnly);
    // The length field is the byte count of the following fields, including the Unit
    // Identifier and PDU fields, so we add one byte to the response size.
    output << pending.TransactionId << pending.ProtocolId << quint16(pending.Response.size() + 1)
           << pending.UnitId << pending.Response;

    if (!pending.Socket->isOpen()) {
        qCDebug(QT_MODBUS) << "(TCP server) Requesting socket has closed.";
        setError(QModbusTcpServer::tr("Requesting socket is closed"), QModbusDevice::WriteError);
        return;
    }

    const qint64 writtenBytes = pending.Socket->write(result);
    if (writtenBytes == -1 || writtenBytes < result.size()) {
        qCDebug(QT_MODBUS) << "(TCP server) Cannot write requested response to socket.";
        setError(QModbusTcpServer::tr("Could not write response to client"), QModbusDevice::WriteError);
        return;
    }

    const QDateTime sndTime = QDateTime::currentDateTime();
    emit rawDataSended(sndTime, result);

    const auto msgResp = ModbusMessage::create(result, ModbusMessage::Tcp, sndTime, false);
    emit modbusResponse(pending.RequestMessage, msgResp);
}

void ModbusTcpServer::enqueueDelayedResponse(const PendingTcpResponse& pending, int delayMs)
{
    const qint64 dueAt = QDateTime::currentMSecsSinceEpoch() + qMax(0, delayMs);
    _delayedResponses[dueAt].append(pending);
    ++_delayedResponseCount;
    armDelayedResponseTimer();
}

void ModbusTcpServer::flushDelayedResponses()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto it = _delayedResponses.begin();
    while (it != _delayedResponses.end() && it.key() <= now) {
        const QList<PendingTcpResponse> batch = it.value();
        _delayedResponseCount -= batch.size();
        if (_delayedResponseCount < 0)
            _delayedResponseCount = 0;

        for (const auto& pending : batch)
            sendTcpResponse(pending);

        it = _delayedResponses.erase(it);
    }

    armDelayedResponseTimer();
}

void ModbusTcpServer::armDelayedResponseTimer()
{
    if (_delayedResponses.isEmpty()) {
        _delayedResponseTimer->stop();
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 nextDue = _delayedResponses.firstKey();
    const int timeoutMs = int(qMax<qint64>(0, nextDue - now));

    if (!_delayedResponseTimer->isActive()
        || _delayedResponseTimer->remainingTime() > timeoutMs) {
        _delayedResponseTimer->start(timeoutMs);
    }
}

///
/// \brief ModbusTcpServer::on_rawDataReceived
/// \param time
/// \param data
///
void ModbusTcpServer::on_rawDataReceived(const QDateTime& time, const QByteArray& data)
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

    return ModbusServer::processRequest(r, serverAddress);
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

    _delayedResponses.clear();
    _delayedResponseCount = 0;
    _delayedResponseOverflowLogged = false;
    if (_delayedResponseTimer->isActive())
        _delayedResponseTimer->stop();

    for (auto socket : std::as_const(_connections))
        socket->disconnectFromHost();

    setState(QModbusDevice::UnconnectedState);
}

