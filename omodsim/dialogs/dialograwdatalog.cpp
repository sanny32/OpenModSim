#include "dialograwdatalog.h"
#include "ui_dialograwdatalog.h"

///
/// \brief RawDataLogModel::RawDataLogModel
/// \param parent
///
RawDataLogModel::RawDataLogModel(QObject* parent)
    : BufferingListModel(parent)
{
}

///
/// \brief RawDataLogModel::data
/// \param index
/// \param role
/// \return
///
QVariant RawDataLogModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() >= rowCount())
        return QVariant();

    const auto item = itemAt(index.row());
    switch(role)
    {
        case Qt::DisplayRole:
            return item.toHex(' ').toUpper();

        case Qt::UserRole:
            return QVariant::fromValue(item);
    }

    return QVariant();
}

///
/// \brief DialogRawDataLog::DialogRawDataLog
/// \param server
/// \param parent
///
DialogRawDataLog::DialogRawDataLog(const ModbusMultiServer& server, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogRawDataLog)
{
    ui->setupUi(this);
    ui->listViewLog->setModel(new RawDataLogModel(this));

    for(auto&& cd : server.connections())
    {
        switch(cd.Type)
        {
            case ConnectionType::Tcp:
            {
                const auto port = QString("%1:%2").arg(cd.TcpParams.IPAddress, QString::number(cd.TcpParams.ServicePort));
                ui->comboBoxServers->addItem(tr("Modbus/TCP Srv %1").arg(port), QVariant::fromValue(cd));
            }
            break;

            case ConnectionType::Serial:
                ui->comboBoxServers->addItem(tr("Port %1").arg(cd.SerialParams.PortName), QVariant::fromValue(cd));
            break;
        }
    }

    connect(&server, &ModbusMultiServer::rawDataReceived, this, &DialogRawDataLog::on_rawDataReceived);
}

///
/// \brief DialogRawDataLog::~DialogRawDataLog
///
DialogRawDataLog::~DialogRawDataLog()
{
    delete ui;
}

///
/// \brief DialogRawDataLog::on_rawDataReceived
/// \param cd
/// \param data
///
void DialogRawDataLog::on_rawDataReceived(const ConnectionDetails& cd, const QByteArray& data)
{
    if(cd == ui->comboBoxServers->currentData().value<ConnectionDetails>()) {
        ((RawDataLogModel*)ui->listViewLog->model())->append(data);
    }
}
