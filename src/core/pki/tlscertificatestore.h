// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file tlscertificatestore.h
/// \brief Manages the application-wide TLS server identity.
///

#ifndef TLSCERTIFICATESTORE_H
#define TLSCERTIFICATESTORE_H

#include <QSslCertificate>
#include <QSslConfiguration>

class TlsCertificateStore
{
public:
    ///
    /// \brief Opens the application store or an explicitly selected test store.
    /// \param root Store directory; empty uses the application data directory.
    ///
    explicit TlsCertificateStore(const QString& root = {});

    ///
    /// \brief Returns the application PKI directory.
    ///
    static QString defaultRoot();

    ///
    /// \brief Returns the directory containing this store.
    ///
    QString root() const;

    ///
    /// \brief Returns the generated identity filename using the local hostname.
    ///
    static QString generatedFileName();

    ///
    /// \brief Returns the default certificate subject for this computer.
    ///
    static QString defaultServerName();

    ///
    /// \brief Returns local DNS names and IP addresses for the default certificate.
    ///
    static QStringList defaultHostNames();

    ///
    /// \brief Generates the default identity only when no named identity file exists.
    /// \return A generation or validation error, or an empty string on success.
    ///
    QString ensureServerCertificate();

    ///
    /// \brief Reads the public certificate of the installed server identity.
    ///
    QSslCertificate serverCertificate() const;

    ///
    /// \brief Atomically replaces the server identity with a self-signed certificate.
    /// \param commonName Server display name.
    /// \param hosts DNS names and IP addresses placed in the SAN extension.
    /// \param validDays Validity period from 1 to 3650 days.
    /// \return A failure description, or an empty string on success.
    ///
    QString generateServerCertificate(const QString& commonName, const QStringList& hosts, int validDays);

    ///
    /// \brief Exports only the public server certificate for clients to trust.
    /// \param fileName Destination PEM file.
    /// \return A failure description, or an empty string on success.
    ///
    QString exportServerCertificate(const QString& fileName) const;

    ///
    /// \brief Loads the server identity without requiring client certificates.
    /// \param configuration Receives TLS 1.2 or later with client verification disabled.
    /// \return A failure description, or an empty string on success.
    ///
    QString configuration(QSslConfiguration& configuration) const;

private:
    QString identityPath() const;
    QString _root;
};

#endif
