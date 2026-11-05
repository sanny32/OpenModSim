// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificategenerator.cpp
/// \brief Generates X.509 server identities through libcrypto.
///

#include "certificategenerator.h"
#include <QCoreApplication>
#include <QHostAddress>
#include <QRegularExpression>
#include <QUrl>
#include <memory>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>

namespace {

///
/// \brief Encodes validated DNS names and IP addresses for the SAN extension.
///
QByteArray alternativeNames(const QStringList& hosts)
{
    if (hosts.isEmpty() || hosts.size() > 128)
        return {};
    const QRegularExpression label("^[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?$");
    QByteArray names;
    for (const QString& host : hosts) {
        const QString value = host.trimmed();
        QHostAddress address;
        QByteArray name;
        if (address.setAddress(value) && address.scopeId().isEmpty()) {
            name = "IP:" + address.toString().toLatin1();
        } else {
            const QByteArray dns = QUrl::toAce(value);
            if (dns.isEmpty() || dns.size() > 253)
                return {};
            for (const QByteArray& part : dns.split('.')) {
                if (!label.match(QString::fromLatin1(part)).hasMatch())
                    return {};
            }
            name = "DNS:" + dns;
        }
        if (!names.isEmpty())
            names += ',';
        names += name;
    }
    return names;
}

///
/// \brief Adds an X.509 extension using the self-signed certificate as issuer.
///
bool addExtension(X509* certificate, int nid, const QByteArray& value)
{
    X509V3_CTX context;
    X509V3_set_ctx(&context, certificate, certificate, nullptr, nullptr, 0);
    const std::unique_ptr<X509_EXTENSION, decltype(&X509_EXTENSION_free)> extension(
        X509V3_EXT_conf_nid(nullptr, &context, nid, value.constData()), X509_EXTENSION_free);
    return extension && X509_add_ext(certificate, extension.get(), -1) == 1;
}

///
/// \brief Copies a memory BIO without exposing its backing allocation.
///
QByteArray bioBytes(BIO* bio)
{
    char* data = nullptr;
    const long size = BIO_get_mem_data(bio, &data);
    return size > 0 ? QByteArray(data, int(size)) : QByteArray();
}

}

///
/// \brief Creates an RSA server identity with DNS and IP subject alternative names.
/// \param commonName Server name shown in certificate viewers.
/// \param hosts DNS names and IP addresses clients use to reach the server.
/// \param validDays Validity period from 1 to 3650 days.
/// \param error Receives a failure description.
/// \return PEM certificate and key, or empty data on failure.
///
GeneratedCertificate CertificateGenerator::generate(const QString& commonName, const QStringList& hosts,
                                                   int validDays, QString* error)
{
    if (error)
        error->clear();
    const QByteArray names = alternativeNames(hosts);
    if (commonName.trimmed().isEmpty() || commonName.toUtf8().size() > 64
        || commonName.contains(QChar::Null) || names.isEmpty() || validDays < 1 || validDays > 3650) {
        if (error)
            *error = QCoreApplication::translate("CertificateGenerator", "Enter a server name, valid DNS names or IP addresses, and a validity period from 1 to 3650 days.");
        return {};
    }
    if (error)
        *error = QCoreApplication::translate("CertificateGenerator", "Cannot generate the server certificate or private key.");

    const std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> context(
        EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), EVP_PKEY_CTX_free);
    EVP_PKEY* generatedKey = nullptr;
    if (!context || EVP_PKEY_keygen_init(context.get()) <= 0
        || EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(), 2048) <= 0
        || EVP_PKEY_keygen(context.get(), &generatedKey) <= 0)
        return {};
    const std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(generatedKey, EVP_PKEY_free);
    const std::unique_ptr<X509, decltype(&X509_free)> certificate(X509_new(), X509_free);
    unsigned char serialBytes[20];
    if (!certificate || RAND_bytes(serialBytes, sizeof(serialBytes)) != 1)
        return {};
    serialBytes[0] = (serialBytes[0] & 0x7f) | 1;
    const std::unique_ptr<BIGNUM, decltype(&BN_free)> serial(
        BN_bin2bn(serialBytes, sizeof(serialBytes), nullptr), BN_free);
    if (!serial || !BN_to_ASN1_INTEGER(serial.get(), X509_get_serialNumber(certificate.get()))
        || X509_set_version(certificate.get(), 2) != 1
        || !X509_gmtime_adj(X509_getm_notBefore(certificate.get()), -300)
        || !X509_gmtime_adj(X509_getm_notAfter(certificate.get()), long(validDays) * 86400)
        || X509_set_pubkey(certificate.get(), key.get()) != 1)
        return {};
    const QByteArray name = commonName.toUtf8();
    X509_NAME* subject = X509_get_subject_name(certificate.get());
    if (X509_NAME_add_entry_by_NID(subject, NID_commonName, MBSTRING_UTF8,
                                  reinterpret_cast<const unsigned char*>(name.constData()), name.size(), -1, 0) != 1
        || X509_set_issuer_name(certificate.get(), subject) != 1
        || !addExtension(certificate.get(), NID_basic_constraints, "critical,CA:FALSE")
        || !addExtension(certificate.get(), NID_key_usage, "critical,digitalSignature,keyEncipherment")
        || !addExtension(certificate.get(), NID_ext_key_usage, "serverAuth")
        || !addExtension(certificate.get(), NID_subject_alt_name, names)
        || !addExtension(certificate.get(), NID_subject_key_identifier, "hash")
        || X509_sign(certificate.get(), key.get(), EVP_sha256()) <= 0)
        return {};
    const std::unique_ptr<BIO, decltype(&BIO_free)> certBio(BIO_new(BIO_s_mem()), BIO_free);
    const std::unique_ptr<BIO, decltype(&BIO_free)> keyBio(BIO_new(BIO_s_mem()), BIO_free);
    if (!certBio || !keyBio || PEM_write_bio_X509(certBio.get(), certificate.get()) != 1
        || PEM_write_bio_PrivateKey(keyBio.get(), key.get(), nullptr, nullptr, 0, nullptr, nullptr) != 1)
        return {};
    if (error)
        error->clear();
    return {bioBytes(certBio.get()), bioBytes(keyBio.get())};
}
