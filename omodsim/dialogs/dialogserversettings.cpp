#include "serialportutils.h"
#include "dialogserversettings.h"
#include "ui_dialogserversettings.h"

///
/// \brief DialogServerSettings::DialogServerSettings
/// \param srv
/// \param parent
///
DialogServerSettings::DialogServerSettings(ModbusMultiServer& srv, QWidget *parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogServerSettings)
    ,_mbMultiServer(srv)
{
    ui->setupUi(this);

    for(auto&& cd : _mbMultiServer.connections()) {
        QListWidgetItem* item = new QListWidgetItem(ui->listServers);
        item->setData(Qt::UserRole, QVariant::fromValue(cd));

        switch(cd.Type) {
        case ConnectionType::Tcp:
            item->setText(tr("Modbus/TCP Srv %1:%2").arg(cd.TcpParams.IPAddress, QString::number(cd.TcpParams.ServicePort)));
            break;

        case ConnectionType::Serial:
             item->setText(tr("Port %1").arg(cd.SerialParams.PortName));
            break;
        }

        ui->listServers->addItem(item);
    }
    ui->listServers->setCurrentRow(0);
}

///
/// \brief DialogServerSettings::~DialogServerSettings
///
DialogServerSettings::~DialogServerSettings()
{
    delete ui;
}

///
/// \brief DialogServerSettings::on_listServers_currentRowChanged
/// \param row
///
void DialogServerSettings::on_listServers_currentRowChanged(int row)
{
    const auto item = ui->listServers->item(row);
    if(item == nullptr) {
        return;
    }

    const auto cd = item->data(Qt::UserRole).value<ConnectionDetails>();

    switch(cd.Type) {
    case ConnectionType::Tcp:
        ui->checkBoxCrcErr->setEnabled(false);
        ui->labelSrvName->setText(tr("<b>Modbus/TCP Srv %1:%2</b>").arg(cd.TcpParams.IPAddress, QString::number(cd.TcpParams.ServicePort)));
        break;

    case ConnectionType::Serial:
        ui->checkBoxCrcErr->setEnabled(true);
        ui->labelSrvName->setText(tr("<b>Port %1:%2:%3:%4:%5</b>").arg(cd.SerialParams.PortName,
                                                                    QString::number(cd.SerialParams.BaudRate),
                                                                    QString::number(cd.SerialParams.WordLength),
                                                                    Parity_toString(cd.SerialParams.Parity),
                                                                    QString::number(cd.SerialParams.StopBits)));
        break;
    }
}
