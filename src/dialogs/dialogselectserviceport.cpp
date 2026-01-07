#include <QNetworkInterface>
#include "dialogselectserviceport.h"
#include "ui_dialogselectserviceport.h"

///
/// \brief DialogSelectServicePort::DialogSelectServicePort
/// \param port
/// \param parent
///
DialogSelectServicePort::DialogSelectServicePort(TcpConnectionParams& params, QWidget *parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogSelectServicePort)
    ,_params(params)
{
    ui->setupUi(this);
    ui->lineEditPort->setInputRange(0, 65535);
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
/// \brief DialogSelectServicePort::accept
///
void DialogSelectServicePort::accept()
{
    _params.ServicePort = ui->lineEditPort->value<int>();
    _params.IPAddress = ui->comboBoxIp->currentText();
    QFixedSizeDialog::accept();
}
