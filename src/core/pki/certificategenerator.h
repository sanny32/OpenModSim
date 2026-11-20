// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file certificategenerator.h
/// \brief Creates self-signed identities for the Modbus TLS server.
///

#ifndef CERTIFICATEGENERATOR_H
#define CERTIFICATEGENERATOR_H

#include <QByteArray>
#include <QStringList>

struct GeneratedCertificate
{
    QByteArray Certificate;
    QByteArray PrivateKey;
};

namespace CertificateGenerator {
///
/// \brief Creates an RSA server identity with DNS and IP subject alternative names.
/// \param commonName Server name shown in certificate viewers.
/// \param hosts DNS names and IP addresses clients use to reach the server.
/// \param validDays Validity period from 1 to 3650 days.
/// \param error Receives a failure description.
/// \return PEM certificate and key, or empty data on failure.
///
GeneratedCertificate generate(const QString& commonName, const QStringList& hosts,
                              int validDays, QString* error);
}

#endif
