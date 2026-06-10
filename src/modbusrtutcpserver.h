// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modbusrtutcpserver.h
/// \brief Declares the Modbus RTU over TCP/IP server.
///

#ifndef MODBUSRTUTCPSERVER_H
#define MODBUSRTUTCPSERVER_H

#include <QHash>
#include <QPointer>
#include <QQueue>
#include <QTcpServer>
#include "modbusserver.h"

class QTimer;

///
/// \brief Serves CRC-protected Modbus RTU frames over TCP/IP.
///
class ModbusRtuTcpServer : public ModbusServer
{
    Q_OBJECT

public:
    explicit ModbusRtuTcpServer(QObject* parent = nullptr);
    ~ModbusRtuTcpServer() override;

    QVariant connectionParameter(QModbusDevice::ConnectionParameter parameter) const override;
    void setConnectionParameter(QModbusDevice::ConnectionParameter parameter,
                                const QVariant& value) override;
    int connectedClientCount() const { return _connections.size(); }

    QIODevice* device() const override { return nullptr; }

signals:
    void modbusClientConnected(const QString& clientAddress, quint16 clientPort);
    void modbusClientDisconnected(const QString& clientAddress, quint16 clientPort);

private slots:
    void on_newConnection();
    void on_acceptError(QAbstractSocket::SocketError error);
    void flushPendingResponses();

protected:
    bool open() override;
    void close() override;

private:
    struct PendingResponse
    {
        QPointer<QTcpSocket> Socket;
        QByteArray Data;
        QSharedPointer<const ModbusMessage> RequestMessage;
        qint64 DueAt = 0;
    };

    bool takeNextFrame(QByteArray& buffer, QByteArray& frame);
    void processFrame(QTcpSocket* socket, const QByteArray& frame, const QDateTime& receiveTime);
    QModbusResponse forwardProcessRequest(const QModbusPdu& request, int serverAddress);
    QByteArray createResponseFrame(quint8 serverAddress, const QModbusResponse& response,
                                   bool incorrectCrc) const;
    void enqueueResponse(const PendingResponse& response, int delayMs);
    void sendResponse(const PendingResponse& response);
    void armPendingResponseTimer();

private:
    int _networkPort = 502;
    QString _networkAddress = QStringLiteral("0.0.0.0");

    QTcpServer* _server = nullptr;
    QVector<QTcpSocket*> _connections;
    QHash<QTcpSocket*, QByteArray> _requestBuffers;
    QHash<QTcpSocket*, QQueue<PendingResponse>> _pendingResponses;
    QTimer* _pendingResponseTimer = nullptr;
};

#endif // MODBUSRTUTCPSERVER_H
