#include <QFileDialog>
#include <QMessageBox>
#include "fontutils.h"
#include "htmldelegate.h"
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
            return QString(R"(
                    <span style="color:#444444">%1</span>
                    <b style="color:%2">%3</b>
                    <span>%4</span>
                )").arg(item.Time.toString(Qt::ISODateWithMs),
                        item.Direction == RawData::Tx ? "#0066cc" : "#009933",
                        item.Direction == RawData::Tx ? "[Tx]" : "[Rx]",
                        item.Data.toHex(' ').toUpper());

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

    setWindowFlags(
        Qt::Window
        | Qt::WindowMinimizeButtonHint
        | Qt::WindowMaximizeButtonHint
        | Qt::WindowCloseButtonHint
    );

    auto model = new RawDataLogModel(this);
    ui->listViewLog->setFont(defaultMonospaceFont());
    ui->listViewLog->setModel(model);
    ui->listViewLog->setItemDelegate(new HtmlDelegate(this));
    ui->comboBoxRowLimit->setCurrentText(QString::number(model->rowLimit()));

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

    connect(model, &RawDataLogModel::rowsInserted,
            this, [this]{
                if(ui->checkBoxAutoScroll->isChecked()) {
                    ui->listViewLog->scrollToBottom();
                }
            });

    connect(&server, &ModbusMultiServer::rawDataSended, this, &DialogRawDataLog::on_rawDataSended);
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
/// \param time
/// \param data
///
void DialogRawDataLog::on_rawDataReceived(const ConnectionDetails& cd, const QDateTime& time, const QByteArray& data)
{
    if(cd == ui->comboBoxServers->currentData().value<ConnectionDetails>()) {
        RawData raw { RawData::Tx, time, data };
        ((RawDataLogModel*)ui->listViewLog->model())->append(raw);
    }
}

///
/// \brief DialogRawDataLog::on_rawDataSended
/// \param cd
/// \param time
/// \param data
///
void DialogRawDataLog::on_rawDataSended(const ConnectionDetails& cd, const QDateTime& time, const QByteArray& data)
{
    if(cd == ui->comboBoxServers->currentData().value<ConnectionDetails>()) {
        RawData raw { RawData::Rx, time, data };
        ((RawDataLogModel*)ui->listViewLog->model())->append(raw);
    }
}

///
/// \brief DialogRawDataLog::on_pushButtonPause_clicked
///
void DialogRawDataLog::on_pushButtonPause_clicked()
{
    auto model = ((RawDataLogModel*)ui->listViewLog->model());
    model->setBufferingMode(!model->isBufferingMode());
    ui->pushButtonPause->setText(model->isBufferingMode() ? tr("Resume") : tr("Pause"));
}

///
/// \brief DialogRawDataLog::on_pushButtonClear_clicked
///
void DialogRawDataLog::on_pushButtonClear_clicked()
{
    ((RawDataLogModel*)ui->listViewLog->model())->clear();
}

///
/// \brief DialogRawDataLog::on_pushButtonExport_clicked
///
void DialogRawDataLog::on_pushButtonExport_clicked()
{
    const auto filename = QFileDialog::getSaveFileName(this, QString(), QString(), "Text files (*.txt)");
    if(!filename.isEmpty()) {
        const bool result = ((RawDataLogModel*)ui->listViewLog->model())->exportToTextFile(filename, [](const RawData& r){
            return QString("%1 %2 %3").arg(
                    r.Time.toString(Qt::ISODateWithMs),
                    r.Direction == RawData::Tx ? "[Tx]" : "[Rx]",
                    r.Data.toHex(' ').toUpper()
                    );
        });
        if(result) {
            QMessageBox::information(this, windowTitle(), tr("Log exported successfully to file %1").arg(filename));
        }
        else {
            QMessageBox::critical(this, windowTitle(), tr("Export log error!"));
        }
    }
}

///
/// \brief DialogRawDataLog::on_comboBoxServers_currentIndexChanged
///
void DialogRawDataLog::on_comboBoxServers_currentIndexChanged(int)
{
    ((RawDataLogModel*)ui->listViewLog->model())->clear();
}

///
/// \brief DialogRawDataLog::on_comboBoxRowLimit_currentTextChanged
/// \param text
///
void DialogRawDataLog::on_comboBoxRowLimit_currentTextChanged(const QString& text)
{
    auto model = ((RawDataLogModel*)ui->listViewLog->model());

    bool ok;
    const int limit = text.toInt(&ok);
    if(ok) {
        model->setRowLimit(qBound(10, limit, 10000));
    }

    ui->comboBoxRowLimit->setCurrentText(QString::number(model->rowLimit()));
}
