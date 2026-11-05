// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file tlscertificatestore.cpp
/// \brief Persists the shared TLS server identity.
///

#include "tlscertificatestore.h"
#include "certificategenerator.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSslKey>
#include <QSslSocket>
#include <QStandardPaths>

namespace {

///
/// \brief Translates store failures without a QObject dependency.
///
QString tr(const char* message)
{
    return QCoreApplication::translate("TlsCertificateStore", message);
}

///
/// \brief Writes a complete file while preserving the previous version on failure.
///
QString saveFile(const QString& path, const QByteArray& bytes, bool privateFile = false)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return QCoreApplication::translate("TlsCertificateStore", "Cannot create the certificate store directory.");
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return file.errorString();
    if (privateFile && !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner))
        return QCoreApplication::translate("TlsCertificateStore", "Cannot restrict access to the server private key.");
    if (file.write(bytes) != bytes.size() || !file.commit())
        return file.errorString();
    return {};
}

}

///
/// \brief Opens the application store or an explicitly selected test store.
/// \param root Store directory; empty uses the application data directory.
///
TlsCertificateStore::TlsCertificateStore(const QString& root)
    : _root(root.isEmpty() ? defaultRoot() : root)
{
}

///
/// \brief Returns the application PKI directory.
///
QString TlsCertificateStore::defaultRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/pki";
}

///
/// \brief Returns the directory containing this store.
///
QString TlsCertificateStore::root() const
{
    return _root;
}

///
/// \brief Keeps the certificate and key in one file so identity replacement is atomic.
///
QString TlsCertificateStore::identityPath() const
{
    return _root + "/own/identity.pem";
}

///
/// \brief Reads the public certificate of the installed server identity.
///
QSslCertificate TlsCertificateStore::serverCertificate() const
{
    const auto certificates = QSslCertificate::fromPath(identityPath());
    return certificates.isEmpty() ? QSslCertificate() : certificates.first();
}

///
/// \brief Atomically replaces the server identity with a self-signed certificate.
/// \param commonName Server display name.
/// \param hosts DNS names and IP addresses placed in the SAN extension.
/// \param validDays Validity period from 1 to 3650 days.
/// \return A failure description, or an empty string on success.
///
QString TlsCertificateStore::generateServerCertificate(const QString& commonName, const QStringList& hosts, int validDays)
{
    QString error;
    const auto generated = CertificateGenerator::generate(commonName, hosts, validDays, &error);
    if (generated.Certificate.isEmpty() || generated.PrivateKey.isEmpty())
        return error;
    return saveFile(identityPath(), generated.Certificate + generated.PrivateKey, true);
}

///
/// \brief Exports only the public server certificate for clients to trust.
/// \param fileName Destination PEM file.
/// \return A failure description, or an empty string on success.
///
QString TlsCertificateStore::exportServerCertificate(const QString& fileName) const
{
    const auto certificate = serverCertificate();
    if (certificate.isNull())
        return tr("Generate a server certificate first.");
    const QString target = QFileInfo(fileName).absoluteFilePath();
    const QString storeRoot = QFileInfo(_root).absoluteFilePath();
    if (target == storeRoot || target.startsWith(storeRoot + QDir::separator()))
        return tr("Export the certificate outside the certificate store.");
    return saveFile(fileName, certificate.toPem());
}

///
/// \brief Loads the server identity without requiring client certificates.
/// \param configuration Receives TLS 1.2 or later with client verification disabled.
/// \return A failure description, or an empty string on success.
///
QString TlsCertificateStore::configuration(QSslConfiguration& configuration) const
{
    QFile identity(identityPath());
    if (!identity.open(QIODevice::ReadOnly))
        return tr("Generate a server certificate in Modbus Definitions → Certificates before enabling TLS.");
    const auto bytes = identity.readAll();
    const auto certificates = QSslCertificate::fromData(bytes);
    const QSslKey key(bytes, QSsl::Rsa);
    if (certificates.isEmpty() || key.isNull())
        return tr("The stored TLS server identity is invalid. Generate a new server certificate.");
    configuration = QSslConfiguration::defaultConfiguration();
    configuration.setProtocol(QSsl::TlsV1_2OrLater);
    configuration.setLocalCertificateChain(certificates);
    configuration.setPrivateKey(key);
    configuration.setCaCertificates({});
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    return {};
}
