// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogselectserviceport.h
/// \brief Declares the dialogselectserviceport interfaces.
///

#ifndef DIALOGSELECTSERVICEPORT_H
#define DIALOGSELECTSERVICEPORT_H

#include "qfixedsizedialog.h"
#include "connectiondetails.h"

namespace Ui {
class DialogSelectServicePort;
}

class DialogSelectServicePort : public QFixedSizeDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Edits TCP endpoint settings and the TLS selection.
    /// \param params Connection parameters updated on acceptance.
    /// \param parent Parent widget.
    /// \param allowTls Whether the transport supports TLS.
    ///
    explicit DialogSelectServicePort(TcpConnectionParams& params, QWidget *parent = nullptr, bool allowTls = true);
    ///
    /// \brief DialogSelectServicePort::~DialogSelectServicePort
    ///
    ~DialogSelectServicePort();

    ///
    /// \brief Applies the endpoint settings and the TLS selection.
    ///
    void accept() override;

protected:
    void changeEvent(QEvent* event) override;

private:
    void updateTlsPort(bool enabled);
    Ui::DialogSelectServicePort *ui;
    TcpConnectionParams& _params;
};

#endif // DIALOGSELECTSERVICEPORT_H

