#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include "fontutils.h"
#include "htmldelegate.h"
#include "modbusmessage.h"
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
                    <span style="color:%4">%5</span>
                )").arg(item.Time.toString(Qt::ISODateWithMs),
                        item.Direction == RawData::Tx ? "#0066cc" : "#009933",
                        item.Direction == RawData::Tx ? "[Tx]" : "[Rx]",
                        item.Valid ? "#000000" : "#cc0000",
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

    for(auto&& cd : server.connections()) {
        comboBoxServers_addConnection(cd);
    }

    createCopyActions();

    connect(model, &RawDataLogModel::rowsInserted,
            this, [this]{
                if(ui->checkBoxAutoScroll->isChecked()) {
                    ui->listViewLog->scrollToBottom();
                }
            });

    connect(&server, &ModbusMultiServer::connected, this, &DialogRawDataLog::on_connected);
    connect(&server, &ModbusMultiServer::rawDataSended, this, &DialogRawDataLog::on_rawDataSended);
    connect(&server, &ModbusMultiServer::rawDataReceived, this, &DialogRawDataLog::on_rawDataReceived);
    connect(ui->listViewLog, &QListView::customContextMenuRequested, this, &DialogRawDataLog::on_customContextMenuRequested);
}

///
/// \brief DialogRawDataLog::~DialogRawDataLog
///
DialogRawDataLog::~DialogRawDataLog()
{
    delete ui;
}

///
/// \brief DialogRawDataLog::changeEvent
/// \param event
///
void DialogRawDataLog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        _copyAct->setText(tr("Copy Text"));
        _copyBytesAct->setText(tr("Copy Bytes"));
    }
    QDialog::changeEvent(event);
}

///
/// \brief DialogRawDataLog::createCopyActions
///
void DialogRawDataLog::createCopyActions()
{
    QIcon copyIcon = QIcon::fromTheme("edit-copy");
    if (copyIcon.isNull()) {
        copyIcon = style()->standardIcon(QStyle::SP_FileIcon);
    }

    _copyAct = new QAction(copyIcon, tr("Copy Text"), this);
    _copyAct->setShortcut(QKeySequence::Copy);
    _copyAct->setShortcutContext(Qt::WidgetShortcut);
    _copyAct->setShortcutVisibleInContextMenu(true);
    ui->listViewLog->addAction(_copyAct);

    connect(_copyAct, &QAction::triggered, this, [this]() {
        const auto index = ui->listViewLog->currentIndex();
        if (index.isValid()) {
            QTextDocument doc;
            doc.setHtml(index.data(Qt::DisplayRole).toString());
            QApplication::clipboard()->setText(doc.toPlainText());
        }
    });

    _copyBytesAct = new QAction(tr("Copy Bytes"), this);
    _copyBytesAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    _copyBytesAct->setShortcutContext(Qt::WidgetShortcut);
    _copyBytesAct->setShortcutVisibleInContextMenu(true);
    ui->listViewLog->addAction(_copyBytesAct);

    connect(_copyBytesAct, &QAction::triggered, this, [this]() {
        const auto index = ui->listViewLog->currentIndex();
        if (index.isValid()) {
            const auto r = index.data(Qt::UserRole).value<RawData>();
            QApplication::clipboard()->setText(r.Data.toHex(' ').toUpper());
        }
    });
}

///
/// \brief DialogRawDataLog::on_customContextMenuRequested
/// \param pos
///
void DialogRawDataLog::on_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(_copyAct);
    menu.addAction(_copyBytesAct);
    menu.exec(ui->listViewLog->viewport()->mapToGlobal(pos));
}

///
/// \brief DialogRawDataLog::on_connected
/// \param cd
///
void DialogRawDataLog::on_connected(const ConnectionDetails& cd)
{
    const auto index = ui->comboBoxServers->findData(QVariant::fromValue(cd));
    if(index == -1) {
        comboBoxServers_addConnection(cd);
    }
}

///
/// \brief DialogRawDataLog::comboBoxServers_addConnection
/// \param cd
///
void DialogRawDataLog::comboBoxServers_addConnection(const ConnectionDetails& cd)
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

///
/// \brief DialogRawDataLog::on_rawDataReceived
/// \param cd
/// \param time
/// \param data
///
void DialogRawDataLog::on_rawDataReceived(const ConnectionDetails& cd, const QDateTime& time, const QByteArray& data)
{
    if(cd == ui->comboBoxServers->currentData().value<ConnectionDetails>()) {
        const auto protocol = cd.Type == ConnectionType::Serial ? ModbusMessage::Rtu : ModbusMessage::Tcp;
        const bool valid = ModbusMessage::create(data, protocol, time, true)->isValid();

        RawData raw { RawData::Tx, time, data, valid };
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
        const auto protocol = cd.Type == ConnectionType::Serial ? ModbusMessage::Rtu : ModbusMessage::Tcp;
        const bool valid = ModbusMessage::create(data, protocol, time, false)->isValid();

        RawData raw { RawData::Rx, time, data, valid };
        ((RawDataLogModel*)ui->listViewLog->model())->append(raw);
    }
}

///
/// \brief DialogRawDataLog::on_pushButtonPause_clicked
///
void DialogRawDataLog::on_pushButtonPause_clicked()
{
    if(logViewState() == LogViewState::Paused) {
        setLogViewState(LogViewState::Running);
    }
    else {
        setLogViewState(LogViewState::Paused);
    }
}

///
/// \brief DialogRawDataLog::logViewState
/// \return
///
LogViewState DialogRawDataLog::logViewState() const
{
    auto model = ((RawDataLogModel*)ui->listViewLog->model());
    if(model->isBufferingMode()) {
        return LogViewState::Paused;
    }
    else {
        return LogViewState::Running;
    }
}

///
/// \brief DialogRawDataLog::setLogViewState
/// \param state
///
void DialogRawDataLog::setLogViewState(LogViewState state)
{
    auto model = ((RawDataLogModel*)ui->listViewLog->model());
    switch(state)
    {
        case LogViewState::Paused:
            model->setBufferingMode(true);
            ui->pushButtonPause->setText(tr("Resume"));
        break;

        case LogViewState::Running:
            model->setBufferingMode(false);
            ui->pushButtonPause->setText(tr("Pause"));
        break;

        default:
        break;
    }
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

    if(logViewState() == LogViewState::Paused) {
        setLogViewState(LogViewState::Running);
    }
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
