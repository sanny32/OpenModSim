// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modbustlslistener.cpp
/// \brief Accepts encrypted Modbus TCP connections.
///

#include "modbustlslistener.h"
#include <QSslSocket>
#include <QTimer>

///
/// \brief Creates a listener owned by the TCP transport.
/// \param parent Transport owning this listener.
/// \param certificateStoreRoot Shared PKI directory; empty uses the application store.
///
ModbusTlsListener::ModbusTlsListener(QObject* parent, const QString& certificateStoreRoot)
    : QTcpServer(parent), _store(certificateStoreRoot)
{
}

///
/// \brief Loads the server identity for encrypted client connections.
/// \param enabled Whether to use TLS for incoming connections.
/// \return An error description, or an empty string when configuration succeeds.
///
QString ModbusTlsListener::configure(bool enabled)
{
    _tlsEnabled = enabled;
    if (!_tlsEnabled)
        return {};
    if (!QSslSocket::supportsSsl())
        return tr("TLS is unavailable in this Qt installation.");

    return _store.configuration(_configuration);
}

///
/// \brief Starts TLS before exposing an accepted socket to Modbus processing.
///
void ModbusTlsListener::incomingConnection(qintptr descriptor)
{
    if (!_tlsEnabled) {
        QTcpServer::incomingConnection(descriptor);
        return;
    }
    auto* socket = new QSslSocket(this);
    if (!socket->setSocketDescriptor(descriptor)) {
        socket->deleteLater();
        return;
    }
    socket->setSslConfiguration(_configuration);
    _handshakes.append(socket);
    connect(socket, &QSslSocket::encrypted, this, &ModbusTlsListener::finishHandshake);
    connect(socket, &QSslSocket::errorOccurred, this, &ModbusTlsListener::reportSocketError);
    connect(socket, &QSslSocket::disconnected, socket, &QObject::deleteLater);
    connect(socket, &QObject::destroyed, this, [this, socket] { _handshakes.removeAll(socket); });
    QTimer::singleShot(10000, socket, [socket] { if (!socket->isEncrypted()) socket->abort(); });
    socket->startServerEncryption();
}

///
/// \brief Makes completed TLS connections available to the transport.
///
void ModbusTlsListener::finishHandshake()
{
    auto* socket = qobject_cast<QSslSocket*>(sender());
    _handshakes.removeAll(socket);
    addPendingConnection(socket);
    emit newConnection();
}

///
/// \brief Reports failed TLS connections without accepting their application data.
///
void ModbusTlsListener::reportSocketError()
{
    auto* socket = qobject_cast<QSslSocket*>(sender());
    if (_handshakes.removeAll(socket)) {
        emit handshakeError(socket->errorString());
        socket->abort();
        socket->deleteLater();
    }
}

///
/// \brief Closes the listener and aborts incomplete handshakes.
///
void ModbusTlsListener::stop()
{
    close();
    const auto sockets = _handshakes;
    for (auto* socket : sockets) {
        socket->abort();
        socket->deleteLater();
    }
    _handshakes.clear();
}
