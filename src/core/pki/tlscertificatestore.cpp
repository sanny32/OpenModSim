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
#include <QHostInfo>
#include <QNetworkInterface>
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
/// \brief Returns the generated identity filename using the local hostname.
///
QString TlsCertificateStore::generatedFileName()
{
    return QStringLiteral("OpenModSim@%1.pem").arg(QHostInfo::localHostName());
}

///
/// \brief Returns the default certificate subject for this computer.
///
QString TlsCertificateStore::defaultServerName()
{
    return (QStringLiteral("OpenModSim@") + QHostInfo::localHostName()).left(64);
}

///
/// \brief Returns local DNS names and IP addresses for the default certificate.
///
QStringList TlsCertificateStore::defaultHostNames()
{
    QStringList hosts = {QHostInfo::localHostName(), "localhost", "127.0.0.1", "::1"};
    for (const auto& address : QNetworkInterface::allAddresses()) {
        if (!address.isNull() && !address.isMulticast() && address.scopeId().isEmpty())
            hosts.append(address.toString());
    }
    hosts.removeDuplicates();
    return hosts;
}

///
/// \brief Generates the default identity only when no named identity file exists.
/// \return A generation or validation error, or an empty string on success.
///
QString TlsCertificateStore::ensureServerCertificate()
{
    const auto files = QDir(_root + "/own").entryList({QStringLiteral("OpenModSim@*.pem")}, QDir::Files, QDir::Name);
    if (!files.isEmpty()) {
        QSslConfiguration configuration;
        return this->configuration(configuration);
    }
    return generateServerCertificate(defaultServerName(), defaultHostNames(), 3650);
}

///
/// \brief Locates the identity by application name, including identities from another host.
///
QString TlsCertificateStore::identityPath() const
{
    const QDir own(_root + "/own");
    const QString target = own.filePath(generatedFileName());
    if (QFileInfo::exists(target))
        return target;
    const auto files = own.entryInfoList({QStringLiteral("OpenModSim@*.pem")}, QDir::Files, QDir::Name);
    for (const auto& file : files) {
        if (!QSslCertificate::fromPath(file.absoluteFilePath()).isEmpty())
            return file.absoluteFilePath();
    }
    return target;
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
    return saveFile(_root + "/own/" + generatedFileName(), generated.Certificate + generated.PrivateKey, true);
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
