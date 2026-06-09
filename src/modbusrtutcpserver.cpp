// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modbusrtutcpserver.cpp
/// \brief Implements the Modbus RTU over TCP/IP server.
///

#include <limits>
#include <utility>
#include <QDataStream>
#include <QDateTime>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include "qmodbusadurtu.h"
#include "modbusrtutcpserver.h"

namespace {

constexpr int MaxRtuAduSize = 256;

///
/// \brief Returns the expected request ADU size, zero for an incomplete header,
/// and -1 for a function with no predefined request length.
/// \param data
/// \return
///
int expectedRequestSize(const QByteArray& data)
{
    if (data.size() < 2)
        return 0;

    const quint8 functionCode = quint8(data.at(1));
    switch (functionCode)
    {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x08:
            return 8;

        case 0x07:
        case 0x0b:
        case 0x0c:
        case 0x11:
            return 4;

        case 0x0f:
        case 0x10:
            if (data.size() < 7)
                return 0;
            return 9 + quint8(data.at(6));

        case 0x14:
        case 0x15:
            if (data.size() < 3)
                return 0;
            return 5 + quint8(data.at(2));

        case 0x16:
            return 10;

        case 0x17:
            if (data.size() < 11)
                return 0;
            return 13 + quint8(data.at(10));

        case 0x18:
            return 6;

        case 0x2b:
            return 7;

        default:
            return -1;
    }
}

///
/// \brief Finds a CRC-valid frame beginning at the supplied offset.
/// \param buffer
/// \param offset
/// \return
///
int validFrameSizeAt(const QByteArray& buffer, int offset)
{
    const QByteArray data = buffer.mid(offset);
    if (data.size() < QModbusAduRtu::MinRtuFrameSize || quint8(data.at(0)) > 247
        || quint8(data.at(1)) == 0) {
        return 0;
    }

    const int expectedSize = expectedRequestSize(data);
    if (expectedSize > 0)
    {
        if (expectedSize > MaxRtuAduSize || data.size() < expectedSize)
            return 0;

        const QModbusAduRtu adu(data.left(expectedSize));
        return adu.matchingChecksum() ? expectedSize : 0;
    }

    const int candidateLimit = qMin(data.size(), MaxRtuAduSize);
    for (int size = QModbusAduRtu::MinRtuFrameSize; size <= candidateLimit; ++size)
    {
        const QModbusAduRtu adu(data.left(size));
        if (adu.matchingChecksum())
            return size;
    }

    return 0;
}

///
/// \brief Finds the first CRC-valid frame at or after the supplied offset.
/// \param buffer
/// \param firstOffset
/// \param frameSize
/// \return
///
int findValidFrame(const QByteArray& buffer, int firstOffset, int& frameSize)
{
    for (int offset = firstOffset;
         offset + QModbusAduRtu::MinRtuFrameSize <= buffer.size();
         ++offset) {
        frameSize = validFrameSizeAt(buffer, offset);
        if (frameSize > 0)
            return offset;
    }

    frameSize = 0;
    return -1;
}

}

///
/// \brief ModbusRtuTcpServer::ModbusRtuTcpServer
/// \param parent
///
ModbusRtuTcpServer::ModbusRtuTcpServer(QObject* parent)
    : ModbusServer(parent)
    , _server(new QTcpServer(this))
    , _pendingResponseTimer(new QTimer(this))
{
    _pendingResponseTimer->setSingleShot(true);

    connect(_server, &QTcpServer::newConnection,
            this, &ModbusRtuTcpServer::on_newConnection);
    connect(_server, &QTcpServer::acceptError,
            this, &ModbusRtuTcpServer::on_acceptError);
    connect(_pendingResponseTimer, &QTimer::timeout,
            this, &ModbusRtuTcpServer::flushPendingResponses);
}

///
/// \brief ModbusRtuTcpServer::~ModbusRtuTcpServer
///
ModbusRtuTcpServer::~ModbusRtuTcpServer()
{
    close();
}

///
/// \brief ModbusRtuTcpServer::connectionParameter
/// \param parameter
/// \return
///
QVariant ModbusRtuTcpServer::connectionParameter(
    QModbusDevice::ConnectionParameter parameter) const
{
    switch (parameter)
    {
        case QModbusDevice::NetworkPortParameter:
            return _networkPort;
        case QModbusDevice::NetworkAddressParameter:
            return _networkAddress;
        default:
            return {};
    }
}

///
/// \brief ModbusRtuTcpServer::setConnectionParameter
/// \param parameter
/// \param value
///
void ModbusRtuTcpServer::setConnectionParameter(
    QModbusDevice::ConnectionParameter parameter, const QVariant& value)
{
    switch (parameter)
    {
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
/// \brief ModbusRtuTcpServer::on_newConnection
///
void ModbusRtuTcpServer::on_newConnection()
{
    while (_server->hasPendingConnections())
    {
        QTcpSocket* socket = _server->nextPendingConnection();
        if (!socket)
            continue;

        _connections.append(socket);
        _requestBuffers.insert(socket, QByteArray());
        emit modbusClientConnected(socket->peerAddress().toString(), socket->peerPort());

        connect(socket, &QTcpSocket::readyRead, this, [this, socket]
        {
            const QByteArray received = socket->readAll();
            if (received.isEmpty())
                return;

            emit rawDataReceived(QDateTime::currentDateTime(), received);
            QByteArray& buffer = _requestBuffers[socket];
            buffer.append(received);

            QByteArray frame;
            while (takeNextFrame(buffer, frame))
                processFrame(socket, frame, QDateTime::currentDateTime());
        });

        connect(socket, &QTcpSocket::disconnected, this, [this, socket]
        {
            const QString address = socket->peerAddress().toString();
            const quint16 port = socket->peerPort();
            _connections.removeAll(socket);
            _requestBuffers.remove(socket);
            _pendingResponses.remove(socket);
            emit modbusClientDisconnected(address, port);
            socket->deleteLater();
            armPendingResponseTimer();
        });
    }
}

///
/// \brief ModbusRtuTcpServer::on_acceptError
/// \param error
///
void ModbusRtuTcpServer::on_acceptError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    setError(_server->errorString(), QModbusDevice::ConnectionError);
}

///
/// \brief ModbusRtuTcpServer::takeNextFrame
/// \param buffer
/// \param frame
/// \return
///
bool ModbusRtuTcpServer::takeNextFrame(QByteArray& buffer, QByteArray& frame)
{
    while (buffer.size() >= QModbusAduRtu::MinRtuFrameSize)
    {
        if (quint8(buffer.at(0)) > 247 || quint8(buffer.at(1)) == 0)
        {
            buffer.remove(0, 1);
            continue;
        }

        const int expectedSize = expectedRequestSize(buffer);
        if (expectedSize == 0)
            return false;

        if (expectedSize > MaxRtuAduSize)
        {
            buffer.remove(0, 1);
            continue;
        }

        if (expectedSize > 0)
        {
            if (buffer.size() < expectedSize)
                return false;

            const QByteArray candidate = buffer.left(expectedSize);
            if (QModbusAduRtu(candidate).matchingChecksum())
            {
                frame = candidate;
                buffer.remove(0, expectedSize);
                return true;
            }

            int recoveredSize = 0;
            const int recoveredOffset = findValidFrame(buffer, 1, recoveredSize);
            if (recoveredOffset >= 0)
            {
                buffer.remove(0, recoveredOffset);
                continue;
            }

            buffer.remove(0, expectedSize);
            continue;
        }

        const int unknownFrameSize = validFrameSizeAt(buffer, 0);
        if (unknownFrameSize > 0)
        {
            frame = buffer.left(unknownFrameSize);
            buffer.remove(0, unknownFrameSize);
            return true;
        }

        int recoveredSize = 0;
        const int recoveredOffset = findValidFrame(buffer, 1, recoveredSize);
        if (recoveredOffset >= 0)
        {
            buffer.remove(0, recoveredOffset);
            continue;
        }

        if (buffer.size() >= MaxRtuAduSize)
        {
            buffer.remove(0, 1);
            continue;
        }

        return false;
    }

    return false;
}

///
/// \brief ModbusRtuTcpServer::processFrame
/// \param socket
/// \param frame
/// \param receiveTime
///
void ModbusRtuTcpServer::processFrame(QTcpSocket* socket, const QByteArray& frame,
                                      const QDateTime& receiveTime)
{
    const QModbusAduRtu adu(frame);
    const quint8 serverAddress = adu.serverAddress();
    const bool broadcast = serverAddress == 0;
    if (!broadcast && !matchingServerAddress(serverAddress))
        return;

    const QModbusRequest request(adu.pdu());
    const auto requestMessage = ModbusMessage::create(
        request, ModbusMessage::Rtu, serverAddress, 0, receiveTime, true);
    emit modbusRequest(requestMessage);

    const ModbusDefinitions definitions = getDefinitions();
    if (definitions.ErrorSimulations.noResponse())
        return;

    if (broadcast)
    {
        for (int address : serverAddresses())
        {
            setCurrentRequestClient(socket->peerAddress().toString(), socket->peerPort());
            forwardProcessRequest(request, address);
            clearCurrentRequestClient();
        }
        return;
    }

    setCurrentRequestClient(socket->peerAddress().toString(), socket->peerPort());
    const QModbusResponse response = forwardProcessRequest(request, serverAddress);
    clearCurrentRequestClient();

    if (!response.isValid()
        || value(QModbusServer::ListenOnlyMode, serverAddress).toBool()) {
        incrementCounter(ModbusServer::Counter::ServerNoResponse, serverAddress);
        return;
    }

    const quint8 responseAddress = serverAddress
        + (definitions.ErrorSimulations.responseIncorrectId() ? 1 : 0);
    PendingResponse pending;
    pending.Socket = socket;
    pending.Data = createResponseFrame(
        responseAddress, response, definitions.ErrorSimulations.responseIncorrectCrc());
    pending.RequestMessage = requestMessage;

    int responseDelay = 0;
    if (definitions.ErrorSimulations.responseDelay()) {
        responseDelay = definitions.ErrorSimulations.responseDelayTime();
    } else if (definitions.ErrorSimulations.responseRandomDelay()
               && definitions.ErrorSimulations.responseRandomDelayUpToTime() > 0) {
        responseDelay = QRandomGenerator::global()->bounded(
            definitions.ErrorSimulations.responseRandomDelayUpToTime());
    }

    enqueueResponse(pending, responseDelay);
}

///
/// \brief ModbusRtuTcpServer::forwardProcessRequest
/// \param request
/// \param serverAddress
/// \return
///
QModbusResponse ModbusRtuTcpServer::forwardProcessRequest(
    const QModbusPdu& request, int serverAddress)
{
    const ModbusDefinitions definitions = getDefinitions();
    incrementCounter(ModbusServer::Counter::BusMessage, serverAddress);

    if (definitions.ErrorSimulations.responseIllegalFunction()) {
        incrementCounter(ModbusServer::Counter::ServerMessage, serverAddress);
        return QModbusExceptionResponse(
            request.functionCode(), QModbusExceptionResponse::IllegalFunction);
    }

    if (definitions.ErrorSimulations.responseDeviceBusy()
        || value(QModbusServer::DeviceBusy, serverAddress).value<quint16>() == 0xffff) {
        incrementCounter(ModbusServer::Counter::ServerBusy, serverAddress);
        return QModbusExceptionResponse(
            request.functionCode(), QModbusExceptionResponse::ServerDeviceBusy);
    }

    incrementCounter(ModbusServer::Counter::ServerMessage, serverAddress);

    if (request.functionCode() == QModbusRequest::EncapsulatedInterfaceTransport)
    {
        quint8 meiType = 0;
        request.decodeData(&meiType);
        if (meiType == EncapsulatedInterfaceTransport::CanOpenGeneralReference) {
            return QModbusExceptionResponse(
                request.functionCode(), QModbusExceptionResponse::IllegalFunction);
        }
    }

    return ModbusServer::processRequest(request, serverAddress);
}

///
/// \brief ModbusRtuTcpServer::createResponseFrame
/// \param serverAddress
/// \param response
/// \param incorrectCrc
/// \return
///
QByteArray ModbusRtuTcpServer::createResponseFrame(
    quint8 serverAddress, const QModbusResponse& response, bool incorrectCrc) const
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream << serverAddress << response;

    quint16 crc = QModbusAduRtu::calculateCRC(result.constData(), result.size());
    if (incorrectCrc)
        crc ^= 0xffff;
    stream << crc;
    return result;
}

///
/// \brief ModbusRtuTcpServer::enqueueResponse
/// \param response
/// \param delayMs
///
void ModbusRtuTcpServer::enqueueResponse(const PendingResponse& response, int delayMs)
{
    if (!response.Socket)
        return;

    PendingResponse pending = response;
    pending.DueAt = QDateTime::currentMSecsSinceEpoch() + qMax(0, delayMs);

    QQueue<PendingResponse>& queue = _pendingResponses[pending.Socket];
    if (!queue.isEmpty())
        pending.DueAt = qMax(pending.DueAt, queue.back().DueAt);
    queue.enqueue(pending);

    flushPendingResponses();
}

///
/// \brief ModbusRtuTcpServer::sendResponse
/// \param response
///
void ModbusRtuTcpServer::sendResponse(const PendingResponse& response)
{
    if (!response.Socket || !response.Socket->isOpen())
        return;

    const qint64 written = response.Socket->write(response.Data);
    if (written < response.Data.size())
    {
        setError(tr("Could not write response to client"), QModbusDevice::WriteError);
        return;
    }

    const QDateTime sendTime = QDateTime::currentDateTime();
    emit rawDataSended(sendTime, response.Data);
    const auto responseMessage = ModbusMessage::create(
        response.Data, ModbusMessage::Rtu, sendTime, false);
    emit modbusResponse(response.RequestMessage, responseMessage);
}

///
/// \brief ModbusRtuTcpServer::flushPendingResponses
///
void ModbusRtuTcpServer::flushPendingResponses()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QMutableHashIterator<QTcpSocket*, QQueue<PendingResponse>> it(_pendingResponses);
    while (it.hasNext())
    {
        it.next();
        QQueue<PendingResponse>& queue = it.value();
        while (!queue.isEmpty() && queue.head().DueAt <= now)
            sendResponse(queue.dequeue());
        if (queue.isEmpty())
            it.remove();
    }

    armPendingResponseTimer();
}

///
/// \brief ModbusRtuTcpServer::armPendingResponseTimer
///
void ModbusRtuTcpServer::armPendingResponseTimer()
{
    qint64 nextDue = std::numeric_limits<qint64>::max();
    for (const QQueue<PendingResponse>& queue : std::as_const(_pendingResponses))
    {
        if (!queue.isEmpty())
            nextDue = qMin(nextDue, queue.head().DueAt);
    }

    if (nextDue == std::numeric_limits<qint64>::max())
    {
        _pendingResponseTimer->stop();
        return;
    }

    const qint64 remaining = qMax<qint64>(
        0, nextDue - QDateTime::currentMSecsSinceEpoch());
    _pendingResponseTimer->start(
        int(qMin<qint64>(remaining, std::numeric_limits<int>::max())));
}

///
/// \brief ModbusRtuTcpServer::open
/// \return
///
bool ModbusRtuTcpServer::open()
{
    if (state() == QModbusDevice::ConnectedState)
        return true;
    if (_server->isListening())
        return false;

    const QUrl url = QUrl::fromUserInput(
        _networkAddress + QStringLiteral(":") + QString::number(_networkPort));
    if (!url.isValid())
    {
        setError(tr("Invalid connection settings for RTU over TCP/IP communication specified."),
                 QModbusDevice::ConnectionError);
        return false;
    }

    if (_server->listen(QHostAddress(url.host()), quint16(url.port())))
        setState(QModbusDevice::ConnectedState);
    else
        setError(_server->errorString(), QModbusDevice::ConnectionError);

    return state() == QModbusDevice::ConnectedState;
}

///
/// \brief ModbusRtuTcpServer::close
///
void ModbusRtuTcpServer::close()
{
    if (state() == QModbusDevice::UnconnectedState)
        return;

    _server->close();
    _pendingResponseTimer->stop();
    _pendingResponses.clear();
    _requestBuffers.clear();

    const QVector<QTcpSocket*> connections = _connections;
    for (QTcpSocket* socket : connections)
        socket->disconnectFromHost();
    _connections.clear();

    setState(QModbusDevice::UnconnectedState);
}
