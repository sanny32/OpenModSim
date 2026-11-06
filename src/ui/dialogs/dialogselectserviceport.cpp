// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogselectserviceport.cpp
/// \brief Implements the dialogselectserviceport functionality.
///

#include <QNetworkInterface>
#include "dialogselectserviceport.h"
#include "ui_dialogselectserviceport.h"

///
/// \brief Edits TCP endpoint settings and the TLS selection.
/// \param params Connection parameters updated on acceptance.
/// \param parent Parent widget.
/// \param allowTls Whether the transport supports TLS.
///
DialogSelectServicePort::DialogSelectServicePort(TcpConnectionParams& params, QWidget *parent, bool allowTls)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogSelectServicePort)
    ,_params(params)
{
    ui->setupUi(this);
    ui->tlsGroup->setVisible(allowTls);
    ui->tlsGroup->setChecked(allowTls && params.TlsEnabled);
    connect(ui->tlsGroup, &QCheckBox::toggled, this, &DialogSelectServicePort::updateTlsPort);
    ui->lineEditPort->setInputRange(1, 65535);
    ui->lineEditPort->setValue(params.ServicePort);

    ui->comboBoxIp->addItem("0.0.0.0");
    for(auto&& nic : QNetworkInterface::allInterfaces())
    {
        if(!(nic.flags() & QNetworkInterface::IsRunning))
        {
            continue;
        }

        for(auto&& entry : nic.addressEntries())
        {
            const auto addr = entry.ip();

            if(addr.isNull() || addr.isMulticast() || addr.protocol() != QAbstractSocket::IPv4Protocol)
            {
                continue;
            }

            ui->comboBoxIp->addItem(addr.toString());
        }
    }

    ui->comboBoxIp->setCurrentText(_params.IPAddress);
}

///
/// \brief DialogSelectServicePort::~DialogSelectServicePort
///
DialogSelectServicePort::~DialogSelectServicePort()
{
    delete ui;
}

///
/// \brief DialogSelectServicePort::changeEvent
///
void DialogSelectServicePort::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    QDialog::changeEvent(event);
}

///
/// \brief Applies the endpoint settings and the TLS selection.
///
void DialogSelectServicePort::accept()
{
    _params.TlsEnabled = ui->tlsGroup->isChecked();
    _params.ServicePort = ui->lineEditPort->value<int>();
    _params.IPAddress = ui->comboBoxIp->currentText();
    QFixedSizeDialog::accept();
}

///
/// \brief Uses the standard port when switching between plain TCP and TLS.
///
void DialogSelectServicePort::updateTlsPort(bool enabled)
{
    if (ui->lineEditPort->value<int>() == (enabled ? 502 : 802))
        ui->lineEditPort->setValue(enabled ? 802 : 502);
}

