// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file tlscertificateswidget.cpp
/// \brief Implements TLS server identity generation and export.
///

#include "tlscertificateswidget.h"
#include "ui_tlscertificateswidget.h"
#include "themedicons.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QPainter>

namespace {
///
/// \brief Tints metadata glyphs with the current text or accent color.
///
void setLabelIcon(QLabel* label, const QString& name, int size, const QColor& color)
{
    QPixmap pixmap = themedIcon(name).pixmap(size, size);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    painter.end();
    label->setPixmap(pixmap);
}
}

///
/// \brief Opens the shared certificate store in the Modbus settings page.
/// \param parent Parent widget.
/// \param certificateStoreRoot PKI directory; empty uses the application store.
///
TlsCertificatesWidget::TlsCertificatesWidget(QWidget* parent, const QString& certificateStoreRoot)
    : QWidget(parent), ui(new Ui::TlsCertificatesWidget), _store(certificateStoreRoot)
{
    ui->setupUi(this);
    const auto certificate = _store.serverCertificate();
    QStringList hosts = TlsCertificateStore::defaultHostNames();
    QString name = TlsCertificateStore::defaultServerName();
    if (!certificate.isNull()) {
        name = certificate.subjectInfo(QSslCertificate::CommonName).join(", ");
        const auto alternatives = certificate.subjectAlternativeNames();
        hosts = alternatives.values(QSsl::DnsEntry);
        hosts.append(alternatives.values(QSsl::IpAddressEntry));
        ui->validDays->setValue(qBound(1, int(certificate.effectiveDate().secsTo(certificate.expiryDate()) / (24 * 60 * 60)), 3650));
    }
    ui->commonName->setText(name);
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
    } else if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        refresh();
    }
    QWidget::changeEvent(event);
}

///
/// \brief Updates certificate actions and icons for the current palette.
///
void TlsCertificatesWidget::refresh()
{
    setLabelIcon(ui->nameIcon, "omodsim/person", 18, palette().color(QPalette::WindowText));
    setLabelIcon(ui->hostsIcon, "omodsim/hosts", 18, palette().color(QPalette::WindowText));
    setLabelIcon(ui->validIcon, "omodsim/calendar", 18, palette().color(QPalette::WindowText));
    ui->generateButton->setIcon(themedIcon("omodsim/add"));
    ui->exportButton->setIcon(themedIcon("omodsim/export"));
    ui->exportButton->setEnabled(!_store.serverCertificate().isNull());
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
    const QString path = QFileDialog::getSaveFileName(this, tr("Export server certificate"), TlsCertificateStore::generatedFileName(), tr("PEM certificate (*.pem)"));
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
