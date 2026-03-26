#include <QtWidgets>
#include "mainwindow.h"
#include "formregistermapview.h"
#include "modbusmessages/modbusmessages.h"
#include "ui_formregistermapview.h"

///
/// \brief FormRegisterMapView::FormRegisterMapView
///
FormRegisterMapView::FormRegisterMapView(ModbusMultiServer& server, MainWindow* parent)
    : QWidget(static_cast<QWidget*>(parent))
    , ui(new Ui::FormRegisterMapView)
    , _mbMultiServer(server)
{
    ui->setupUi(this);

    setupToolbar();

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(20);
    ui->tableWidget->verticalHeader()->hide();

    connect(ui->tableWidget, &QTableWidget::cellChanged,
            this, &FormRegisterMapView::on_tableWidget_cellChanged);

    setupServerConnections();

    setWindowIcon(QIcon(":/res/actionShowData.png"));
}

///
/// \brief FormRegisterMapView::~FormRegisterMapView
///
FormRegisterMapView::~FormRegisterMapView()
{
    delete ui;
}

///
/// \brief FormRegisterMapView::displayDefinition
///
RegisterMapViewDefinitions FormRegisterMapView::displayDefinition() const
{
    return _displayDefinition;
}

///
/// \brief FormRegisterMapView::setDisplayDefinition
///
void FormRegisterMapView::setDisplayDefinition(const RegisterMapViewDefinitions& dd)
{
    _displayDefinition = dd;
    if (!dd.FormName.isEmpty())
        setWindowTitle(dd.FormName);
}

///
/// \brief FormRegisterMapView::backgroundColor
///
QColor FormRegisterMapView::backgroundColor() const
{
    return ui->tableWidget->palette().color(QPalette::Base);
}

///
/// \brief FormRegisterMapView::setBackgroundColor
///
void FormRegisterMapView::setBackgroundColor(const QColor& clr)
{
    if (!clr.isValid()) return;
    auto pal = ui->tableWidget->palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::AlternateBase, clr.lighter(108));
    ui->tableWidget->setPalette(pal);
}

///
/// \brief FormRegisterMapView::foregroundColor
///
QColor FormRegisterMapView::foregroundColor() const
{
    return ui->tableWidget->palette().color(QPalette::Text);
}

///
/// \brief FormRegisterMapView::setForegroundColor
///
void FormRegisterMapView::setForegroundColor(const QColor& clr)
{
    if (!clr.isValid()) return;
    auto pal = ui->tableWidget->palette();
    pal.setColor(QPalette::Text, clr);
    ui->tableWidget->setPalette(pal);
}

///
/// \brief FormRegisterMapView::font
///
QFont FormRegisterMapView::font() const
{
    return ui->tableWidget->font();
}

///
/// \brief FormRegisterMapView::setFont
///
void FormRegisterMapView::setFont(const QFont& font)
{
    ui->tableWidget->setFont(font);
}

///
/// \brief FormRegisterMapView::saveXml
///
void FormRegisterMapView::saveXml(QXmlStreamWriter& xml) const
{
    xml << const_cast<FormRegisterMapView*>(this);
}

///
/// \brief FormRegisterMapView::loadXml
///
void FormRegisterMapView::loadXml(QXmlStreamReader& xml)
{
    xml >> this;
}

///
/// \brief FormRegisterMapView::connectEditSlots
///
void FormRegisterMapView::connectEditSlots()
{
}

///
/// \brief FormRegisterMapView::disconnectEditSlots
///
void FormRegisterMapView::disconnectEditSlots()
{
}

///
/// \brief FormRegisterMapView::show
///
void FormRegisterMapView::show()
{
    QWidget::show();
    emit showed();
}

///
/// \brief FormRegisterMapView::changeEvent
///
void FormRegisterMapView::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        ui->tableWidget->horizontalHeaderItem(0)->setText(tr("DevId"));
        ui->tableWidget->horizontalHeaderItem(1)->setText(tr("Type"));
        ui->tableWidget->horizontalHeaderItem(2)->setText(tr("Address"));
        ui->tableWidget->horizontalHeaderItem(3)->setText(tr("Value"));
        ui->tableWidget->horizontalHeaderItem(4)->setText(tr("Description"));
    }
    QWidget::changeEvent(event);
}

///
/// \brief FormRegisterMapView::closeEvent
///
void FormRegisterMapView::closeEvent(QCloseEvent* event)
{
    emit closing();
    QWidget::closeEvent(event);
}

///
/// \brief FormRegisterMapView::on_mbRequest
///
void FormRegisterMapView::on_mbRequest(const ConnectionDetails& /*cd*/, QSharedPointer<const ModbusMessage> msg)
{
    if (!msg || !msg->isRequest()) return;

    const quint8 deviceId = static_cast<quint8>(msg->deviceId());
    const auto pduData = msg->adu()->pdu().data();

    auto readU16 = [&](int idx) -> quint16 {
        if (pduData.size() < idx + 2) return 0;
        return (static_cast<quint8>(pduData[idx]) << 8) | static_cast<quint8>(pduData[idx + 1]);
    };

    switch (msg->functionCode())
    {
        case QModbusPdu::ReadCoils:
            processRequest(deviceId, QModbusDataUnit::Coils, readU16(0), readU16(2));
        break;
        case QModbusPdu::ReadDiscreteInputs:
            processRequest(deviceId, QModbusDataUnit::DiscreteInputs, readU16(0), readU16(2));
        break;
        case QModbusPdu::ReadHoldingRegisters:
            processRequest(deviceId, QModbusDataUnit::HoldingRegisters, readU16(0), readU16(2));
        break;
        case QModbusPdu::ReadInputRegisters:
            processRequest(deviceId, QModbusDataUnit::InputRegisters, readU16(0), readU16(2));
        break;
        case QModbusPdu::WriteSingleCoil:
            processRequest(deviceId, QModbusDataUnit::Coils, readU16(0), 1);
        break;
        case QModbusPdu::WriteSingleRegister:
            processRequest(deviceId, QModbusDataUnit::HoldingRegisters, readU16(0), 1);
        break;
        case QModbusPdu::WriteMultipleCoils:
            processRequest(deviceId, QModbusDataUnit::Coils, readU16(0), readU16(2));
        break;
        case QModbusPdu::WriteMultipleRegisters:
            processRequest(deviceId, QModbusDataUnit::HoldingRegisters, readU16(0), readU16(2));
        break;
        case QModbusPdu::MaskWriteRegister:
            processRequest(deviceId, QModbusDataUnit::HoldingRegisters, readU16(0), 1);
        break;
        case QModbusPdu::ReadWriteMultipleRegisters:
            processRequest(deviceId, QModbusDataUnit::HoldingRegisters, readU16(0), readU16(2));
            processRequest(deviceId, QModbusDataUnit::HoldingRegisters, readU16(4), readU16(6));
        break;
        default:
        break;
    }
}

///
/// \brief FormRegisterMapView::on_mbDataChanged
///
void FormRegisterMapView::on_mbDataChanged(quint8 deviceId, const QModbusDataUnit& data)
{
    if (!data.isValid()) return;

    for (quint32 i = 0; i < data.valueCount(); ++i) {
        const ItemMapKey key{ deviceId, data.registerType(), static_cast<quint16>(data.startAddress() + i) };
        auto it = _registerMap.find(key);
        if (it != _registerMap.end()) {
            const quint16 value = static_cast<quint16>(data.value(i));
            it->value = value;
            const int row = findRow(key);
            if (row >= 0) updateValue(row, value);
        }
    }
}

///
/// \brief FormRegisterMapView::on_actionClearMap_triggered
///
void FormRegisterMapView::on_actionClearMap_triggered()
{
    _registerMap.clear();
    _updatingTable = true;
    ui->tableWidget->setRowCount(0);
    _updatingTable = false;
}

///
/// \brief FormRegisterMapView::on_tableWidget_cellChanged
///
void FormRegisterMapView::on_tableWidget_cellChanged(int row, int col)
{
    if (_updatingTable || col != 4) return;

    const auto* keyItem = ui->tableWidget->item(row, 0);
    if (!keyItem) return;

    ItemMapKey key;
    key.DeviceId = static_cast<quint8>(keyItem->data(Qt::UserRole).toInt());
    key.Type     = static_cast<QModbusDataUnit::RegisterType>(keyItem->data(Qt::UserRole + 1).toInt());
    key.Address  = static_cast<quint16>(keyItem->data(Qt::UserRole + 2).toInt());

    auto it = _registerMap.find(key);
    if (it != _registerMap.end()) {
        const auto* descItem = ui->tableWidget->item(row, 4);
        it->description = descItem ? descItem->text() : QString();
    }
}

///
/// \brief FormRegisterMapView::processRequest
///
void FormRegisterMapView::processRequest(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 count)
{
    if (count == 0 || count > 2000) return;

    const QModbusDataUnit unit = _mbMultiServer.data(deviceId, type, startAddress, count);

    for (quint16 i = 0; i < count; ++i) {
        const ItemMapKey key{ deviceId, type, static_cast<quint16>(startAddress + i) };
        const quint16 value = unit.isValid() ? static_cast<quint16>(unit.value(i)) : 0;

        auto it = _registerMap.find(key);
        if (it != _registerMap.end()) {
            if (it->value != value) {
                it->value = value;
                const int row = findRow(key);
                if (row >= 0) updateValue(row, value);
            }
        } else {
            Entry entry;
            entry.value = value;
            _registerMap[key] = entry;
            insertEntry(key, value, QString());
        }
    }
}

///
/// \brief FormRegisterMapView::findRow
///
int FormRegisterMapView::findRow(const ItemMapKey& key) const
{
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        const auto* item = ui->tableWidget->item(i, 0);
        if (!item) continue;
        if (item->data(Qt::UserRole).toInt()     == static_cast<int>(key.DeviceId) &&
            item->data(Qt::UserRole + 1).toInt() == static_cast<int>(key.Type)     &&
            item->data(Qt::UserRole + 2).toInt() == static_cast<int>(key.Address))
            return i;
    }
    return -1;
}

///
/// \brief FormRegisterMapView::insertEntry
///
void FormRegisterMapView::insertEntry(const ItemMapKey& key, quint16 value, const QString& description)
{
    _updatingTable = true;

    const int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    auto* devItem = new QTableWidgetItem(QString::number(key.DeviceId));
    devItem->setFlags(devItem->flags() & ~Qt::ItemIsEditable);
    devItem->setData(Qt::UserRole,     static_cast<int>(key.DeviceId));
    devItem->setData(Qt::UserRole + 1, static_cast<int>(key.Type));
    devItem->setData(Qt::UserRole + 2, static_cast<int>(key.Address));

    auto* typeItem = new QTableWidgetItem(registerTypeToString(key.Type));
    typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);

    auto* addrItem = new QTableWidgetItem(QString::number(key.Address + 1)); // 1-based display
    addrItem->setFlags(addrItem->flags() & ~Qt::ItemIsEditable);
    addrItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* valItem = new QTableWidgetItem(formatValue(key.Type, value));
    valItem->setFlags(valItem->flags() & ~Qt::ItemIsEditable);
    valItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* descItem = new QTableWidgetItem(description);

    ui->tableWidget->setItem(row, 0, devItem);
    ui->tableWidget->setItem(row, 1, typeItem);
    ui->tableWidget->setItem(row, 2, addrItem);
    ui->tableWidget->setItem(row, 3, valItem);
    ui->tableWidget->setItem(row, 4, descItem);

    _updatingTable = false;
}

///
/// \brief FormRegisterMapView::updateValue
///
void FormRegisterMapView::updateValue(int row, quint16 value)
{
    auto* keyItem = ui->tableWidget->item(row, 0);
    if (!keyItem) return;

    const auto type = static_cast<QModbusDataUnit::RegisterType>(keyItem->data(Qt::UserRole + 1).toInt());

    _updatingTable = true;
    auto* valItem = ui->tableWidget->item(row, 3);
    if (valItem) valItem->setText(formatValue(type, value));
    _updatingTable = false;
}

///
/// \brief FormRegisterMapView::setupToolbar
///
void FormRegisterMapView::setupToolbar()
{
    _toolBar = new QToolBar(this);
    _toolBar->setIconSize(QSize(16, 16));
    _toolBar->setMovable(false);

    auto* clearAction = _toolBar->addAction(QIcon(":/res/icon-clear.png"), tr("Clear Register Map"));
    connect(clearAction, &QAction::triggered, this, &FormRegisterMapView::on_actionClearMap_triggered);

    qobject_cast<QVBoxLayout*>(layout())->insertWidget(0, _toolBar);
}

///
/// \brief FormRegisterMapView::setupServerConnections
///
void FormRegisterMapView::setupServerConnections()
{
    connect(&_mbMultiServer, &ModbusMultiServer::requestOnConnection,
            this, &FormRegisterMapView::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::dataChanged,
            this, &FormRegisterMapView::on_mbDataChanged);
}

///
/// \brief FormRegisterMapView::registerTypeToString
///
QString FormRegisterMapView::registerTypeToString(QModbusDataUnit::RegisterType type) const
{
    switch (type)
    {
        case QModbusDataUnit::Coils:            return tr("Coils");
        case QModbusDataUnit::DiscreteInputs:   return tr("Discrete Inputs");
        case QModbusDataUnit::InputRegisters:   return tr("Input Registers");
        case QModbusDataUnit::HoldingRegisters: return tr("Holding Registers");
        default:                                return QString();
    }
}

///
/// \brief FormRegisterMapView::formatValue
///
QString FormRegisterMapView::formatValue(QModbusDataUnit::RegisterType type, quint16 value) const
{
    if (type == QModbusDataUnit::Coils || type == QModbusDataUnit::DiscreteInputs)
        return value ? QStringLiteral("1") : QStringLiteral("0");
    return QStringLiteral("0x%1").arg(value, 4, 16, QChar('0')).toUpper();
}
