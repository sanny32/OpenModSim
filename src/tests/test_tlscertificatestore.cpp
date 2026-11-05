// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_tlscertificatestore.cpp
/// \brief Tests generated TLS server identities and atomic persistence.
///

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QSslKey>
#include <QSslSocket>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include "pki/certificategenerator.h"
#include "pki/tlscertificatestore.h"

class TestTlsCertificateStore : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void generatesServerIdentity();
    void rejectsInvalidGeneration_data();
    void rejectsInvalidGeneration();
    void persistsAndExportsIdentity();
    void ignoresLegacyTrustFiles();
    void preservesIdentityOnFailure();
    void rejectsMissingAndCorruptIdentity();
};

///
/// \brief Skips store integration when Qt cannot load a TLS backend.
///
void TestTlsCertificateStore::initTestCase()
{
    if (!QSslSocket::supportsSsl())
        QSKIP("Qt TLS backend is unavailable");
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
    QVERIFY(certificate.isSelfSigned());
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
    QVERIFY(!store.exportServerCertificate(store.root() + "/own/identity.pem").isEmpty());
#ifndef Q_OS_WIN
    const auto permissions = QFile::permissions(store.root() + "/own/identity.pem");
    QVERIFY(!(permissions & (QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ReadOther | QFileDevice::WriteOther)));
#endif
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
    QFile file(directory.filePath("own/identity.pem"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("invalid identity");
    file.close();
    QVERIFY(!store.configuration(configuration).isEmpty());
}

QTEST_GUILESS_MAIN(TestTlsCertificateStore)
#include "test_tlscertificatestore.moc"
