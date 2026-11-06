// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_tlscertificatestore.cpp
/// \brief Tests generated TLS server identities and atomic persistence.
///

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QHostInfo>
#include <QSslKey>
#include <QSslSocket>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <openssl/pem.h>
#include <openssl/crypto.h>
#include <openssl/x509v3.h>
#include "pki/certificategenerator.h"
#include "pki/tlscertificatestore.h"

class TestTlsCertificateStore : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void generatesServerIdentity();
    void usesSingleOpenSslVersion();
    void createsDefaultIdentityOnce();
    void reportsStartupGenerationFailures();
    void rejectsInvalidGeneration_data();
    void rejectsInvalidGeneration();
    void persistsAndExportsIdentity();
    void ignoresLegacyTrustFiles();
    void loadsNamedIdentitiesOnly();
    void preservesIdentityOnFailure();
    void rejectsMissingAndCorruptIdentity();
    void rejectsMismatchedIdentity();
};

///
/// \brief Skips store integration when Qt cannot load a TLS backend.
///
void TestTlsCertificateStore::initTestCase()
{
    if (qEnvironmentVariableIsSet("OMODSIM_REQUIRE_TLS"))
        QVERIFY2(QSslSocket::supportsSsl(), "The configured TLS runtime could not be loaded");
    if (!QSslSocket::supportsSsl())
        QSKIP("Qt TLS backend is unavailable");
}

///
/// \brief Keeps certificate generation and the OpenSSL Qt backend on the same ABI family.
///
void TestTlsCertificateStore::usesSingleOpenSslVersion()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    constexpr unsigned long minimum = 0x10101000UL;
    constexpr unsigned long maximum = 0x30000000UL;
#else
    constexpr unsigned long minimum = 0x30000000UL;
    constexpr unsigned long maximum = 0x40000000UL;
#endif
    QVERIFY(OpenSSL_version_num() >= minimum && OpenSSL_version_num() < maximum);
#ifndef Q_OS_MAC
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (QSslSocket::activeBackend() != "openssl")
        return;
#endif
    const auto tlsVersion = QSslSocket::sslLibraryVersionNumber();
    QVERIFY2(tlsVersion >= minimum && tlsVersion < maximum,
             qPrintable(QSslSocket::sslLibraryVersionString()));
#endif
}

///
/// \brief Creates the first-run identity and preserves its bytes on later launches.
///
void TestTlsCertificateStore::createsDefaultIdentityOnce()
{
    QTemporaryDir directory;
    TlsCertificateStore store(directory.filePath("pki"));
    QVERIFY2(store.ensureServerCertificate().isEmpty(), "First-run generation failed");
    const auto certificate = store.serverCertificate();
    QVERIFY(!certificate.isNull());
    QCOMPARE(certificate.subjectInfo(QSslCertificate::CommonName), QStringList{TlsCertificateStore::defaultServerName()});
    QVERIFY(certificate.subjectAlternativeNames().values(QSsl::DnsEntry).contains("localhost"));
    QVERIFY(certificate.expiryDate() > QDateTime::currentDateTimeUtc().addDays(3649));
    const QString path = store.root() + "/own/" + TlsCertificateStore::generatedFileName();
    QFile identity(path);
    QVERIFY(identity.open(QIODevice::ReadOnly));
    const QByteArray original = identity.readAll();
    identity.close();
    QVERIFY(TlsCertificateStore(store.root()).ensureServerCertificate().isEmpty());
    QVERIFY(identity.open(QIODevice::ReadOnly));
    QCOMPARE(identity.readAll(), original);
    identity.close();
    QVERIFY(QFile::rename(path, store.root() + "/own/OpenModSim@previous-host.pem"));
    QVERIFY(store.ensureServerCertificate().isEmpty());
    QCOMPARE(store.serverCertificate(), certificate);
    QVERIFY(!QFileInfo::exists(path));
}

///
/// \brief Reports unwritable stores and preserves corrupt named files for explicit replacement.
///
void TestTlsCertificateStore::reportsStartupGenerationFailures()
{
    QTemporaryDir directory;
    const QString root = directory.filePath("pki");
    TlsCertificateStore store(root);
    QVERIFY(QDir().mkpath(root + "/own"));
    const QString path = root + "/own/" + TlsCertificateStore::generatedFileName();
    QFile identity(path);
    QVERIFY(identity.open(QIODevice::WriteOnly));
    identity.write("corrupt identity");
    identity.close();
    QVERIFY(!store.ensureServerCertificate().isEmpty());
    QVERIFY(identity.open(QIODevice::ReadOnly));
    QCOMPARE(identity.readAll(), QByteArray("corrupt identity"));
    identity.close();
    QFile blocked(directory.filePath("blocked"));
    QVERIFY(blocked.open(QIODevice::WriteOnly));
    blocked.close();
    QVERIFY(!TlsCertificateStore(blocked.fileName()).ensureServerCertificate().isEmpty());
}

///
/// \brief Checks signatures, key matching, certificate purpose and host identities.
///
void TestTlsCertificateStore::generatesServerIdentity()
{
    QString error;
    const auto generated = CertificateGenerator::generate("Modbus simulator", {"localhost", "127.0.0.1", "::1"}, 2, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    const QSslCertificate certificate(generated.Certificate);
    QVERIFY(!certificate.isNull());
    QCOMPARE(certificate.subjectInfo(QSslCertificate::CommonName), QStringList{"Modbus simulator"});
    QCOMPARE(certificate.subjectAlternativeNames().values(QSsl::DnsEntry), QStringList{"localhost"});
    QVERIFY(certificate.subjectAlternativeNames().values(QSsl::IpAddressEntry).contains("127.0.0.1"));
    QVERIFY(certificate.subjectAlternativeNames().values(QSsl::IpAddressEntry).contains("0:0:0:0:0:0:0:1")
            || certificate.subjectAlternativeNames().values(QSsl::IpAddressEntry).contains("::1"));
    QVERIFY(certificate.expiryDate() > QDateTime::currentDateTimeUtc().addDays(1));
    QVERIFY(certificate.expiryDate() <= QDateTime::currentDateTimeUtc().addDays(2));
    const std::unique_ptr<BIO, decltype(&BIO_free)> certBio(
        BIO_new_mem_buf(generated.Certificate.constData(), generated.Certificate.size()), BIO_free);
    const std::unique_ptr<BIO, decltype(&BIO_free)> keyBio(
        BIO_new_mem_buf(generated.PrivateKey.constData(), generated.PrivateKey.size()), BIO_free);
    const std::unique_ptr<X509, decltype(&X509_free)> parsed(PEM_read_bio_X509(certBio.get(), nullptr, nullptr, nullptr), X509_free);
    const std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(PEM_read_bio_PrivateKey(keyBio.get(), nullptr, nullptr, nullptr), EVP_PKEY_free);
    QVERIFY(parsed && key);
    QCOMPARE(EVP_PKEY_bits(key.get()), 2048);
    QCOMPARE(X509_check_private_key(parsed.get(), key.get()), 1);
    QCOMPARE(X509_NAME_cmp(X509_get_subject_name(parsed.get()), X509_get_issuer_name(parsed.get())), 0);
    QCOMPARE(X509_verify(parsed.get(), key.get()), 1);
    QCOMPARE(X509_check_ca(parsed.get()), 0);
    QCOMPARE(X509_check_purpose(parsed.get(), X509_PURPOSE_SSL_SERVER, 0), 1);
    QCOMPARE(X509_check_purpose(parsed.get(), X509_PURPOSE_SSL_CLIENT, 0), 0);
    const auto second = CertificateGenerator::generate("Modbus simulator", {"localhost"}, 2, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(QSslCertificate(second.Certificate).serialNumber() != certificate.serialNumber());
}

///
/// \brief Covers invalid subjects, host lists, SAN injection and validity periods.
///
void TestTlsCertificateStore::rejectsInvalidGeneration_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QStringList>("hosts");
    QTest::addColumn<int>("days");
    QTest::newRow("empty-name") << QString() << QStringList{"localhost"} << 2;
    QTest::newRow("long-name") << QString(65, 'x') << QStringList{"localhost"} << 2;
    QTest::newRow("empty-hosts") << QString("server") << QStringList() << 2;
    QTest::newRow("empty-host") << QString("server") << QStringList{QString()} << 2;
    QTest::newRow("invalid-host") << QString("server") << QStringList{"invalid host"} << 2;
    QTest::newRow("san-injection") << QString("server") << QStringList{"localhost,URI:injected"} << 2;
    QTest::newRow("zero-days") << QString("server") << QStringList{"localhost"} << 0;
    QTest::newRow("too-many-days") << QString("server") << QStringList{"localhost"} << 3651;
}

///
/// \brief Leaves no key material when certificate inputs are rejected.
///
void TestTlsCertificateStore::rejectsInvalidGeneration()
{
    QFETCH(QString, name);
    QFETCH(QStringList, hosts);
    QFETCH(int, days);
    QString error;
    const auto generated = CertificateGenerator::generate(name, hosts, days, &error);
    QVERIFY(!error.isEmpty());
    QVERIFY(generated.Certificate.isEmpty());
    QVERIFY(generated.PrivateKey.isEmpty());
}

///
/// \brief Persists the identity with private permissions and exports no key material.
///
void TestTlsCertificateStore::persistsAndExportsIdentity()
{
    QTemporaryDir directory;
    TlsCertificateStore store(directory.filePath("pki"));
    QVERIFY(store.generateServerCertificate("server", {"localhost"}, 2).isEmpty());
    const QString identity = store.root() + "/own/" + TlsCertificateStore::generatedFileName();
    QCOMPARE(QFileInfo(identity).fileName(), QString("OpenModSim@%1.pem").arg(QHostInfo::localHostName()));
    QVERIFY(QFileInfo::exists(identity));
    QVERIFY(!QFileInfo::exists(store.root() + "/own/identity.pem"));
    TlsCertificateStore reopened(store.root());
    QCOMPARE(reopened.serverCertificate(), store.serverCertificate());
    QSslConfiguration configuration;
    QVERIFY(reopened.configuration(configuration).isEmpty());
    QVERIFY(!configuration.privateKey().isNull());
    QCOMPARE(configuration.localCertificate(), store.serverCertificate());
    QCOMPARE(configuration.protocol(), QSsl::TlsV1_2OrLater);
    QCOMPARE(configuration.peerVerifyMode(), QSslSocket::VerifyNone);
    QVERIFY(configuration.caCertificates().isEmpty());
    const QString exported = directory.filePath("public.pem");
    QVERIFY(store.exportServerCertificate(exported).isEmpty());
    QFile file(exported);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray bytes = file.readAll();
    QCOMPARE(bytes, store.serverCertificate().toPem());
    QVERIFY(!bytes.contains("PRIVATE KEY"));
    QVERIFY(!store.exportServerCertificate(identity).isEmpty());
    QVERIFY(!store.exportServerCertificate(store.root() + "/own/export.pem").isEmpty());
    QVERIFY(!store.exportServerCertificate(store.root() + "/own/../own/export.pem").isEmpty());
    QCOMPARE(store.serverCertificate(), configuration.localCertificate());
#ifndef Q_OS_WIN
    const auto permissions = QFile::permissions(identity);
    QVERIFY(!(permissions & (QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ReadOther | QFileDevice::WriteOther)));
#endif
}

///
/// \brief Ignores the old generic filename and loads application identities after host renaming.
///
void TestTlsCertificateStore::loadsNamedIdentitiesOnly()
{
    QTemporaryDir directory;
    TlsCertificateStore store(directory.path());
    QVERIFY(store.generateServerCertificate("original", {"localhost"}, 2).isEmpty());
    const auto original = store.serverCertificate();
    const QString current = directory.filePath("own/" + TlsCertificateStore::generatedFileName());
    const QString legacy = directory.filePath("own/identity.pem");
    QVERIFY(QFile::rename(current, legacy));
    QSslConfiguration configuration;
    QVERIFY(!store.configuration(configuration).isEmpty());
    QVERIFY(store.serverCertificate().isNull());
    const QString renamed = directory.filePath("own/OpenModSim@previous-host.pem");
    QVERIFY(QFile::rename(legacy, renamed));
    QVERIFY(store.configuration(configuration).isEmpty());
    QCOMPARE(configuration.localCertificate(), original);
    QFile invalid(directory.filePath("own/OpenModSim@000-invalid.pem"));
    QVERIFY(invalid.open(QIODevice::WriteOnly));
    invalid.write("invalid identity");
    invalid.close();
    QCOMPARE(store.serverCertificate(), original);
    QVERIFY(store.generateServerCertificate("replacement", {"localhost"}, 2).isEmpty());
    QCOMPARE(store.serverCertificate().subjectInfo(QSslCertificate::CommonName), QStringList{"replacement"});
    QVERIFY(QFileInfo::exists(renamed));
    QFile corrupt(current);
    QVERIFY(corrupt.open(QIODevice::WriteOnly | QIODevice::Truncate));
    corrupt.write("invalid current identity");
    corrupt.close();
    QVERIFY(!store.configuration(configuration).isEmpty());
}

///
/// \brief Ignores legacy trust files when configuring server-only TLS authentication.
///
void TestTlsCertificateStore::ignoresLegacyTrustFiles()
{
    QTemporaryDir directory;
    TlsCertificateStore store(directory.path());
    QVERIFY(store.generateServerCertificate("server", {"localhost"}, 2).isEmpty());
    QVERIFY(QDir().mkpath(directory.filePath("trusted")));
    QFile file(directory.filePath("trusted/legacy.pem"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("invalid legacy certificate");
    file.close();
    QSslConfiguration configuration;
    QVERIFY(store.configuration(configuration).isEmpty());
    QCOMPARE(configuration.peerVerifyMode(), QSslSocket::VerifyNone);
    QVERIFY(configuration.caCertificates().isEmpty());
    QVERIFY(file.exists());
}

///
/// \brief Keeps the installed identity when validation or persistence fails.
///
void TestTlsCertificateStore::preservesIdentityOnFailure()
{
    QTemporaryDir directory;
    TlsCertificateStore store(directory.filePath("pki"));
    QVERIFY(store.generateServerCertificate("original", {"localhost"}, 2).isEmpty());
    const auto certificate = store.serverCertificate();
    QVERIFY(!store.generateServerCertificate("", {"localhost"}, 2).isEmpty());
    QCOMPARE(store.serverCertificate(), certificate);
    const QString blockedPath = directory.filePath("file");
    QFile file(blockedPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    TlsCertificateStore blocked(blockedPath);
    QVERIFY(!blocked.generateServerCertificate("server", {"localhost"}, 2).isEmpty());
    QVERIFY(blocked.serverCertificate().isNull());
}

///
/// \brief Reports missing and corrupted identities before starting a TLS listener.
///
void TestTlsCertificateStore::rejectsMissingAndCorruptIdentity()
{
    QTemporaryDir directory;
    TlsCertificateStore store(directory.path());
    QSslConfiguration configuration;
    QVERIFY(!store.configuration(configuration).isEmpty());
    QVERIFY(!store.exportServerCertificate(directory.filePath("export.pem")).isEmpty());
    QVERIFY(QDir().mkpath(directory.filePath("own")));
    QFile file(directory.filePath("own/" + TlsCertificateStore::generatedFileName()));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("invalid identity");
    file.close();
    QVERIFY(!store.configuration(configuration).isEmpty());
}

///
/// \brief Rejects a readable certificate paired with a different private key.
///
void TestTlsCertificateStore::rejectsMismatchedIdentity()
{
    QTemporaryDir directory;
    TlsCertificateStore store(directory.path());
    QString error;
    const auto first = CertificateGenerator::generate("first", {"localhost"}, 2, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    const auto second = CertificateGenerator::generate("second", {"localhost"}, 2, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    const QByteArray bytes = first.Certificate + second.PrivateKey;
    QVERIFY(!QSslCertificate::fromData(bytes).isEmpty());
    QVERIFY(!QSslKey(bytes, QSsl::Rsa).isNull());
    QVERIFY(QDir().mkpath(directory.filePath("own")));
    QFile file(directory.filePath("own/" + TlsCertificateStore::generatedFileName()));
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(bytes), qint64(bytes.size()));
    file.close();
    QSslConfiguration configuration;
    QVERIFY(!store.configuration(configuration).isEmpty());
}

QTEST_GUILESS_MAIN(TestTlsCertificateStore)
#include "test_tlscertificatestore.moc"
