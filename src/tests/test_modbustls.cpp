// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbustls.cpp
/// \brief Integration tests for Modbus TCP with server-authenticated TLS.
///

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QSignalSpy>
#include <QSslKey>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <openssl/pem.h>
#include "modbustcpserver.h"
#include "connectiondetails.h"
#include "pki/tlscertificatestore.h"

class TestModbusTls : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void exchangesEncryptedRequests();
    void acceptsAnyTlsClient_data();
    void acceptsAnyTlsClient();
    void rejectsPlaintextClient();
    void rejectsUntrustedServer();
    void rejectsMissingCredentials();
    void closesIncompleteHandshakes();

private:
    bool openssl(const QStringList& arguments);
    bool createIdentity(const QString& name, const QString& usage);
    bool expireCertificate(const QString& name);
    quint16 startServer(ModbusTcpServer& server);
    void configureClient(QSslSocket& socket, const QString& identity);
    QTemporaryDir _directory;
    QString _openssl;
    ConnectionDetails _connection;
};

///
/// \brief Runs certificate provisioning without requiring manual test setup.
///
bool TestModbusTls::openssl(const QStringList& arguments)
{
    QProcess process;
    process.setWorkingDirectory(_directory.path());
    process.start(_openssl, arguments);
    return process.waitForFinished(10000) && process.exitCode() == 0;
}

///
/// \brief Creates a CA signed identity with the requested extended key usage.
///
bool TestModbusTls::createIdentity(const QString& name, const QString& usage)
{
    QFile extensions(_directory.filePath(name + ".ext"));
    if (!extensions.open(QIODevice::WriteOnly))
        return false;
    extensions.write(("basicConstraints=CA:FALSE\nkeyUsage=digitalSignature,keyEncipherment\nextendedKeyUsage="
                      + usage + "\nsubjectAltName=DNS:localhost,IP:127.0.0.1\n").toUtf8());
    extensions.close();
    return openssl({"req", "-new", "-newkey", "rsa:2048", "-nodes", "-keyout", name + ".key",
                    "-out", name + ".csr", "-subj", "/CN=" + name})
        && openssl({"x509", "-req", "-in", name + ".csr", "-CA", "ca.pem", "-CAkey", "ca.key",
                    "-CAcreateserial", "-out", name + ".pem", "-days", "2", "-extfile", name + ".ext"});
}

///
/// \brief Creates an expired fixture without depending on CLI support for negative validity periods.
///
bool TestModbusTls::expireCertificate(const QString& name)
{
    QFile file(_directory.filePath(name + ".pem"));
    QFile ca(_directory.filePath("ca.key"));
    if (!file.open(QIODevice::ReadOnly) || !ca.open(QIODevice::ReadOnly))
        return false;
    const QByteArray bytes = file.readAll();
    const QByteArray keyBytes = ca.readAll();
    file.close();
    const std::unique_ptr<BIO, decltype(&BIO_free)> input(BIO_new_mem_buf(bytes.constData(), bytes.size()), BIO_free);
    const std::unique_ptr<BIO, decltype(&BIO_free)> keyInput(BIO_new_mem_buf(keyBytes.constData(), keyBytes.size()), BIO_free);
    const std::unique_ptr<X509, decltype(&X509_free)> certificate(PEM_read_bio_X509(input.get(), nullptr, nullptr, nullptr), X509_free);
    const std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(PEM_read_bio_PrivateKey(keyInput.get(), nullptr, nullptr, nullptr), EVP_PKEY_free);
    const std::unique_ptr<BIO, decltype(&BIO_free)> output(BIO_new(BIO_s_mem()), BIO_free);
    if (!certificate || !key || !output
        || !X509_gmtime_adj(X509_getm_notBefore(certificate.get()), -2 * 86400)
        || !X509_gmtime_adj(X509_getm_notAfter(certificate.get()), -86400)
        || X509_sign(certificate.get(), key.get(), EVP_sha256()) <= 0
        || PEM_write_bio_X509(output.get(), certificate.get()) != 1
        || !file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    char* data = nullptr;
    const long size = BIO_get_mem_data(output.get(), &data);
    return file.write(data, size) == size;
}

///
/// \brief Provisions temporary certificates, skipping unavailable TLS dependencies.
///
void TestModbusTls::initTestCase()
{
    if (!QSslSocket::supportsSsl())
        QSKIP("Qt TLS backend is unavailable");
    _openssl = QStandardPaths::findExecutable("openssl");
    if (_openssl.isEmpty())
        QSKIP("openssl is unavailable for test certificate provisioning");
    QVERIFY(_directory.isValid());
    QVERIFY(openssl({"req", "-x509", "-newkey", "rsa:2048", "-nodes", "-keyout", "ca.key",
                     "-out", "ca.pem", "-days", "2", "-subj", "/CN=Modbus Test CA",
                     "-addext", "basicConstraints=critical,CA:TRUE"}));
    QVERIFY(createIdentity("server", "serverAuth"));
    QVERIFY(createIdentity("client", "clientAuth"));
    QVERIFY(createIdentity("expired", "clientAuth"));
    QVERIFY(expireCertificate("expired"));
    QVERIFY(openssl({"req", "-x509", "-newkey", "rsa:2048", "-nodes", "-keyout", "rogue.key",
                     "-out", "rogue.pem", "-days", "2", "-subj", "/CN=Untrusted Client"}));
    _connection.TcpParams.TlsEnabled = true;
    TlsCertificateStore store(_directory.filePath("pki"));
    QVERIFY(store.generateServerCertificate("Modbus test server", {"localhost", "127.0.0.1"}, 2).isEmpty());
}

///
/// \brief Starts a TLS transport with a holding register map on a free local port.
///
quint16 TestModbusTls::startServer(ModbusTcpServer& server)
{
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, 0))
        return 0;
    const quint16 port = probe.serverPort();
    probe.close();
    server.setProperty("ConnectionDetails", QVariant::fromValue(_connection));
    server.setConnectionParameter(QModbusDevice::NetworkAddressParameter, "127.0.0.1");
    server.setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
    ModbusDataUnitMap map;
    map.addUnitMap(QUuid::createUuid(), QModbusDataUnit::HoldingRegisters, 0, 8);
    server.addServerAddress(1);
    server.setMap(map, 1);
    return server.connectDevice() ? port : 0;
}

///
/// \brief Configures server verification and an optional client identity.
///
void TestModbusTls::configureClient(QSslSocket& socket, const QString& identity)
{
    auto configuration = QSslConfiguration::defaultConfiguration();
    configuration.setProtocol(QSsl::TlsV1_2OrLater);
    configuration.setCaCertificates({TlsCertificateStore(_directory.filePath("pki")).serverCertificate()});
    configuration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    if (!identity.isEmpty()) {
        configuration.setLocalCertificateChain(QSslCertificate::fromPath(_directory.filePath(identity + ".pem")));
        QFile key(_directory.filePath(identity + ".key"));
        QVERIFY(key.open(QIODevice::ReadOnly));
        configuration.setPrivateKey(QSslKey(key.readAll(), QSsl::Rsa));
    }
    socket.setSslConfiguration(configuration);
    socket.setPeerVerifyName("localhost");
}

///
/// \brief Checks encrypted Modbus requests without a client certificate.
///
void TestModbusTls::exchangesEncryptedRequests()
{
    ModbusTcpServer server(nullptr, _directory.filePath("pki"));
    const auto port = startServer(server);
    QVERIFY2(port, qPrintable(server.errorString(0)));
    QSslSocket socket;
    configureClient(socket, {});
    socket.connectToHostEncrypted("127.0.0.1", port);
    QTRY_VERIFY(socket.isEncrypted());
    QTRY_COMPARE(server.connectedClientCount(), 1);
    QSignalSpy requests(&server, &ModbusServer::modbusRequest);
    const QByteArray request = QByteArray::fromHex("123400000006010300000002");
    QCOMPARE(socket.write(request.left(3)), qint64(3));
    QTest::qWait(20);
    QCOMPARE(requests.count(), 0);
    socket.write(request.mid(3) + request);
    QTRY_COMPARE(requests.count(), 2);
    QTRY_VERIFY(socket.bytesAvailable() >= 26);
    const QByteArray response = QByteArray::fromHex("12340000000701030400000000");
    QCOMPARE(socket.readAll(), response + response);
    socket.disconnectFromHost();
    QTRY_COMPARE(server.connectedClientCount(), 0);
    server.disconnectDevice();
    QVERIFY(startServer(server));
    server.disconnectDevice();
}

///
/// \brief Covers clients without an identity and clients with arbitrary certificate configurations.
///
void TestModbusTls::acceptsAnyTlsClient_data()
{
    QTest::addColumn<QString>("identity");
    QTest::newRow("no-certificate") << QString();
    QTest::newRow("ca-signed") << QString("client");
    QTest::newRow("self-signed") << QString("rogue");
    QTest::newRow("expired") << QString("expired");
    QTest::newRow("server-purpose") << QString("server");
}

///
/// \brief Exchanges Modbus data with every TLS client without importing any trust anchors.
///
void TestModbusTls::acceptsAnyTlsClient()
{
    QFETCH(QString, identity);
    ModbusTcpServer server(nullptr, _directory.filePath("pki"));
    const auto port = startServer(server);
    QVERIFY(port);
    QSignalSpy errors(&server, &ModbusServer::errorOccurred);
    QSslSocket socket;
    configureClient(socket, identity);
    socket.connectToHostEncrypted("127.0.0.1", port);
    QTRY_VERIFY(socket.isEncrypted());
    QTRY_COMPARE(server.connectedClientCount(), 1);
    socket.write(QByteArray::fromHex("123400000006010300000002"));
    QTRY_VERIFY(socket.bytesAvailable() >= 13);
    QCOMPARE(socket.readAll(), QByteArray::fromHex("12340000000701030400000000"));
    QVERIFY(errors.isEmpty());
}

///
/// \brief Rejects plaintext connections before any Modbus data is processed.
///
void TestModbusTls::rejectsPlaintextClient()
{
    ModbusTcpServer server(nullptr, _directory.filePath("pki"));
    const auto port = startServer(server);
    QVERIFY(port);
    QSignalSpy requests(&server, &ModbusServer::modbusRequest);
    QSignalSpy connected(&server, &ModbusTcpServer::modbusClientConnected);
    QSignalSpy errors(&server, &ModbusServer::errorOccurred);
    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", port);
    socket.write(QByteArray::fromHex("123400000006010300000002"));
    QTRY_VERIFY(!errors.isEmpty());
    QTRY_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(requests.count(), 0);
    QCOMPARE(connected.count(), 0);
    QCOMPARE(server.state(), QModbusDevice::ConnectedState);
}

///
/// \brief Verifies that clients reject a server signed by an untrusted CA.
///
void TestModbusTls::rejectsUntrustedServer()
{
    ModbusTcpServer server(nullptr, _directory.filePath("pki"));
    const auto port = startServer(server);
    QVERIFY(port);
    QSignalSpy requests(&server, &ModbusServer::modbusRequest);
    QSslSocket socket;
    configureClient(socket, "client");
    auto configuration = socket.sslConfiguration();
    configuration.setCaCertificates(QSslCertificate::fromPath(_directory.filePath("rogue.pem")));
    socket.setSslConfiguration(configuration);
    QSignalSpy errors(&socket, &QSslSocket::errorOccurred);
    socket.connectToHostEncrypted("127.0.0.1", port);
    QTRY_VERIFY(!errors.isEmpty());
    QTRY_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(requests.count(), 0);
    QTRY_COMPARE(server.connectedClientCount(), 0);
}

///
/// \brief Fails closed for unreadable or invalid credential files.
///
void TestModbusTls::rejectsMissingCredentials()
{
    const QString root = _directory.filePath("invalid-pki");
    ModbusTcpServer missing(nullptr, root);
    missing.setProperty("ConnectionDetails", QVariant::fromValue(_connection));
    QVERIFY(!missing.connectDevice());
    QVERIFY(!missing.errorString(0).isEmpty());
    QVERIFY(QDir().mkpath(root + "/own"));
    QFile malformed(root + "/own/identity.pem");
    QVERIFY(malformed.open(QIODevice::WriteOnly));
    malformed.write("not a PEM credential");
    malformed.close();
    ModbusTcpServer invalid(nullptr, root);
    invalid.setProperty("ConnectionDetails", QVariant::fromValue(_connection));
    QVERIFY(!invalid.connectDevice());
    QVERIFY(!invalid.errorString(0).isEmpty());
    QCOMPARE(invalid.state(), QModbusDevice::UnconnectedState);
}

///
/// \brief Aborts clients that have connected without completing a handshake.
///
void TestModbusTls::closesIncompleteHandshakes()
{
    ModbusTcpServer server(nullptr, _directory.filePath("pki"));
    const auto port = startServer(server);
    QVERIFY(port);
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    QTRY_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QTest::qWait(20);
    QCOMPARE(server.connectedClientCount(), 0);
    server.disconnectDevice();
    QTRY_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);
}

QTEST_GUILESS_MAIN(TestModbusTls)
#include "test_modbustls.moc"
