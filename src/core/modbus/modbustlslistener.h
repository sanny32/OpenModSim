// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modbustlslistener.h
/// \brief Accepts encrypted TLS sockets for the Modbus TCP transport.
///

#ifndef MODBUSTLSLISTENER_H
#define MODBUSTLSLISTENER_H

#include <QTcpServer>
#include <QSslConfiguration>
#include "pki/tlscertificatestore.h"

class QSslSocket;

class ModbusTlsListener : public QTcpServer
{
    Q_OBJECT
public:
    ///
    /// \brief Creates a listener owned by the TCP transport.
    /// \param parent Transport owning this listener.
    /// \param certificateStoreRoot Shared PKI directory; empty uses the application store.
    ///
    explicit ModbusTlsListener(QObject* parent, const QString& certificateStoreRoot);

    ///
    /// \brief Loads the server identity for encrypted client connections.
    /// \param enabled Whether to use TLS for incoming connections.
    /// \return An error description, or an empty string when configuration succeeds.
    ///
    QString configure(bool enabled);

    ///
    /// \brief Closes the listener and aborts incomplete handshakes.
    ///
    void stop();

signals:
    void handshakeError(const QString& description);

protected:
    void incomingConnection(qintptr descriptor) override;

private:
    void finishHandshake();
    void reportSocketError();
    TlsCertificateStore _store;
    QSslConfiguration _configuration;
    bool _tlsEnabled = false;
    QList<QSslSocket*> _handshakes;
};

#endif
