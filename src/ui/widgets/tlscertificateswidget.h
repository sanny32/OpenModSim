// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file tlscertificateswidget.h
/// \brief Declares the Modbus certificate settings page.
///

#ifndef TLSCERTIFICATESWIDGET_H
#define TLSCERTIFICATESWIDGET_H

#include <QWidget>
#include "pki/tlscertificatestore.h"

namespace Ui { class TlsCertificatesWidget; }

class TlsCertificatesWidget : public QWidget
{
    Q_OBJECT
public:
    ///
    /// \brief Opens the shared certificate store in the Modbus settings page.
    /// \param parent Parent widget.
    /// \param certificateStoreRoot PKI directory; empty uses the application store.
    ///
    explicit TlsCertificatesWidget(QWidget* parent = nullptr, const QString& certificateStoreRoot = {});

    ///
    /// \brief Releases the certificate settings controls.
    ///
    ~TlsCertificatesWidget() override;

protected:
    void changeEvent(QEvent* event) override;

private:
    void refresh();
    void generateCertificate();
    void exportCertificate();
    void reportError(const QString& error);

    Ui::TlsCertificatesWidget* ui;
    TlsCertificateStore _store;
};

#endif
