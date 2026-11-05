// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file tlscertificateswidget.cpp
/// \brief Implements TLS server identity generation and export.
///

#include "tlscertificateswidget.h"
#include "ui_tlscertificateswidget.h"
#include <QCryptographicHash>
#include <QFileDialog>
#include <QHostInfo>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QRegularExpression>

///
/// \brief Opens the shared certificate store in the Modbus settings page.
/// \param parent Parent widget.
/// \param certificateStoreRoot PKI directory; empty uses the application store.
///
TlsCertificatesWidget::TlsCertificatesWidget(QWidget* parent, const QString& certificateStoreRoot)
    : QWidget(parent), ui(new Ui::TlsCertificatesWidget), _store(certificateStoreRoot)
{
    ui->setupUi(this);
    ui->commonName->setText("OpenModSim@" + QHostInfo::localHostName());
    QStringList hosts = {QHostInfo::localHostName(), "localhost", "127.0.0.1", "::1"};
    for (const auto& address : QNetworkInterface::allAddresses()) {
        if (!address.isNull() && !address.isMulticast() && address.scopeId().isEmpty())
            hosts.append(address.toString());
    }
    hosts.removeDuplicates();
    ui->hostNames->setText(hosts.join(", "));
    connect(ui->generateButton, &QPushButton::clicked, this, &TlsCertificatesWidget::generateCertificate);
    connect(ui->exportButton, &QPushButton::clicked, this, &TlsCertificatesWidget::exportCertificate);
    refresh();
}

///
/// \brief Releases the certificate settings controls.
///
TlsCertificatesWidget::~TlsCertificatesWidget()
{
    delete ui;
}

///
/// \brief Retranslates certificate details when the application language changes.
///
void TlsCertificatesWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        refresh();
    }
    QWidget::changeEvent(event);
}

///
/// \brief Reloads certificate metadata without displaying the private key.
///
void TlsCertificatesWidget::refresh()
{
    const auto certificate = _store.serverCertificate();
    if (certificate.isNull()) {
        ui->serverDetails->setPlainText(tr("No server certificate. Generate one to enable TLS."));
    } else {
        const auto alternatives = certificate.subjectAlternativeNames();
        QStringList hosts = alternatives.values(QSsl::DnsEntry);
        hosts.append(alternatives.values(QSsl::IpAddressEntry));
        ui->serverDetails->setPlainText(tr("Subject: %1\nHost names / IP addresses: %2\nValid until: %3\nSHA-256: %4")
            .arg(certificate.subjectInfo(QSslCertificate::CommonName).join(", "), hosts.join(", "),
                 certificate.expiryDate().toLocalTime().toString(Qt::ISODate),
                 QString::fromLatin1(certificate.digest(QCryptographicHash::Sha256).toHex(':'))));
    }
    ui->exportButton->setEnabled(!certificate.isNull());

}

///
/// \brief Generates an identity after confirming replacement of an existing certificate.
///
void TlsCertificatesWidget::generateCertificate()
{
    if (!_store.serverCertificate().isNull()
        && QMessageBox::question(this, tr("Replace server certificate"),
                                 tr("Clients will need to trust the new certificate. Replace the existing server certificate?")) != QMessageBox::Yes)
        return;
    const auto hosts = ui->hostNames->text().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);
    const QString error = _store.generateServerCertificate(ui->commonName->text().trimmed(), hosts, ui->validDays->value());
    reportError(error);
    if (error.isEmpty())
        refresh();
}

///
/// \brief Exports the public certificate for installing trust in Modbus clients.
///
void TlsCertificatesWidget::exportCertificate()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Export server certificate"), "server.pem", tr("PEM certificate (*.pem)"));
    if (!path.isEmpty())
        reportError(_store.exportServerCertificate(path));
}

///
/// \brief Shows failures without treating successful operations as notifications.
///
void TlsCertificatesWidget::reportError(const QString& error)
{
    if (!error.isEmpty())
        QMessageBox::warning(this, tr("Certificates"), error);
}
