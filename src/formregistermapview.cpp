#include <QtWidgets>
#include "mainwindow.h"
#include "apppreferences.h"
#include "formregistermapview.h"
#include "formatutils.h"
#include "numericutils.h"
#include "modbusmessages/modbusmessages.h"
#include "controls/numericlineedit.h"
#include "ui_formregistermapview.h"

namespace {

constexpr int RoleDeviceId  = Qt::UserRole;
constexpr int RoleType      = Qt::UserRole + 1;
constexpr int RoleAddress   = Qt::UserRole + 2;
constexpr int RoleTypeValue = Qt::UserRole + 3;

enum Col { ColUnit = 0, ColType, ColAddress, ColDataType, ColOrder, ColComment, ColValue, ColTimestamp };

///
/// \brief TypeItemDelegate — inline combo box for register type column
///
class TypeItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        auto* cb = new QComboBox(parent);
        cb->addItem(tr("Coils"),            static_cast<int>(QModbusDataUnit::Coils));
        cb->addItem(tr("Discrete Inputs"),  static_cast<int>(QModbusDataUnit::DiscreteInputs));
        cb->addItem(tr("Input Registers"),  static_cast<int>(QModbusDataUnit::InputRegisters));
        cb->addItem(tr("Holding Registers"),static_cast<int>(QModbusDataUnit::HoldingRegisters));
        return cb;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        const QString current = index.data(Qt::DisplayRole).toString();
        const int idx = cb->findText(current);
        if (idx >= 0) cb->setCurrentIndex(idx);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        auto* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        model->setData(index, cb->currentText(), Qt::DisplayRole);
        model->setData(index, cb->currentData(),  RoleTypeValue);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
        editor->setGeometry(option.rect);
    }
};

///
/// \brief DataTypeItemDelegate — inline combo box for DataType column
///
class DataTypeItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        auto* cb = new QComboBox(parent);
        for (auto it = EnumStrings<DataType>::mapping().cbegin();
             it != EnumStrings<DataType>::mapping().cend(); ++it) {
            cb->addItem(it.value(), static_cast<int>(it.key()));
        }
        auto* self = const_cast<DataTypeItemDelegate*>(this);
        connect(cb, QOverload<int>::of(&QComboBox::activated), self, [self, cb](int) {
            emit self->commitData(cb);
            emit self->closeEditor(cb);
        });
        return cb;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        const int idx = cb->findText(index.data(Qt::DisplayRole).toString());
        if (idx >= 0) cb->setCurrentIndex(idx);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        auto* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        model->setData(index, cb->currentText(), Qt::DisplayRole);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
        editor->setGeometry(option.rect);
    }
};

///
/// \brief OrderItemDelegate — inline combo box for RegisterOrder column
///
class OrderItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        auto* cb = new QComboBox(parent);
        for (auto it = EnumStrings<RegisterOrder>::mapping().cbegin();
             it != EnumStrings<RegisterOrder>::mapping().cend(); ++it) {
            cb->addItem(it.value(), static_cast<int>(it.key()));
        }
        auto* self = const_cast<OrderItemDelegate*>(this);
        connect(cb, QOverload<int>::of(&QComboBox::activated), self, [self, cb](int) {
            emit self->commitData(cb);
            emit self->closeEditor(cb);
        });
        return cb;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        const int idx = cb->findText(index.data(Qt::DisplayRole).toString());
        if (idx >= 0) cb->setCurrentIndex(idx);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        auto* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        model->setData(index, cb->currentText(), Qt::DisplayRole);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
        editor->setGeometry(option.rect);
    }
};

///
/// \brief UnitItemDelegate — NumericLineEdit editor for the Unit (device ID) column
///
class UnitItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        auto* editor = new NumericLineEdit(parent);
        editor->setInputMode(NumericLineEdit::UInt32Mode);
        editor->setInputRange<quint32>(1, 255);
        return editor;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto* le = qobject_cast<NumericLineEdit*>(editor);
        if (!le) return;
        le->setValue<quint32>(index.data(RoleDeviceId).toUInt());
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        auto* le = qobject_cast<NumericLineEdit*>(editor);
        if (!le) return;
        model->setData(index, QString::number(le->value<quint32>()), Qt::EditRole);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
        editor->setGeometry(option.rect);
    }
};

///
/// \brief AddressItemDelegate — NumericLineEdit editor for the Address column
///
class AddressItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        const bool zeroBased = AppPreferences::instance().dataViewDefinitions().ZeroBasedAddress;
        auto* editor = new NumericLineEdit(parent);
        editor->setInputMode(NumericLineEdit::UInt32Mode);
        editor->setInputRange<quint32>(zeroBased ? 0 : 1, zeroBased ? 65535 : 65536);
        return editor;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto* le = qobject_cast<NumericLineEdit*>(editor);
        if (!le) return;
        const quint16 rawAddr = static_cast<quint16>(
            index.siblingAtColumn(ColUnit).data(RoleAddress).toUInt());
        const bool zeroBased = AppPreferences::instance().dataViewDefinitions().ZeroBasedAddress;
        le->setValue<quint32>(zeroBased ? rawAddr : static_cast<quint32>(rawAddr) + 1);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        auto* le = qobject_cast<NumericLineEdit*>(editor);
        if (!le) return;
        model->setData(index, QString::number(le->value<quint32>()), Qt::EditRole);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
        editor->setGeometry(option.rect);
    }
};

///
/// \brief ValueItemDelegate — NumericLineEdit editor for the Value column
///
class ValueItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
    {
        const auto regType = static_cast<QModbusDataUnit::RegisterType>(
            index.siblingAtColumn(ColUnit).data(RoleType).toInt());

        const DataType type = enumFromString<DataType>(
            index.siblingAtColumn(ColDataType).data(Qt::DisplayRole).toString(), DataType::Int16);

        auto* editor = new NumericLineEdit(parent);

        if (regType == QModbusDataUnit::Coils || regType == QModbusDataUnit::DiscreteInputs) {
            editor->setInputMode(NumericLineEdit::UInt32Mode);
            editor->setInputRange<quint32>(0, 1);
        } else {
            switch (type) {
                case DataType::Int16:
                    editor->setInputMode(NumericLineEdit::Int32Mode);
                    editor->setInputRange<qint32>(-32768, 32767);
                    break;
                case DataType::UInt16:
                    editor->setInputMode(NumericLineEdit::UInt32Mode);
                    editor->setInputRange<quint32>(0, 65535);
                    break;
                case DataType::Float32:
                    editor->setInputMode(NumericLineEdit::FloatMode);
                    break;
                case DataType::Float64:
                    editor->setInputMode(NumericLineEdit::DoubleMode);
                    break;
                case DataType::Int32:
                    editor->setInputMode(NumericLineEdit::Int32Mode);
                    break;
                case DataType::UInt32:
                    editor->setInputMode(NumericLineEdit::UInt32Mode);
                    break;
                case DataType::Int64:
                    editor->setInputMode(NumericLineEdit::Int64Mode);
                    break;
                case DataType::UInt64:
                    editor->setInputMode(NumericLineEdit::UInt64Mode);
                    break;
                default: // Binary, Hex, Ansi
                    editor->setInputMode(NumericLineEdit::HexMode);
                    editor->setInputRange<quint16>(0, 0xFFFF);
                    break;
            }
        }
        return editor;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto* le = qobject_cast<NumericLineEdit*>(editor);
        if (!le) return;

        const DataType type = enumFromString<DataType>(
            index.siblingAtColumn(ColDataType).data(Qt::DisplayRole).toString(), DataType::Int16);
        const QString text = index.data(Qt::DisplayRole).toString();

        switch (type) {
            case DataType::Float32:
                le->setValue<float>(text.toFloat());
                break;
            case DataType::Float64:
                le->setValue<double>(text.toDouble());
                break;
            case DataType::Int32:
                le->setValue<qint32>(static_cast<qint32>(text.toLong()));
                break;
            case DataType::UInt32:
                le->setValue<quint32>(static_cast<quint32>(text.toULong()));
                break;
            case DataType::Int64:
                le->setValue<qint64>(text.toLongLong());
                break;
            case DataType::UInt64:
                le->setValue<quint64>(text.toULongLong());
                break;
            default: {
                const quint16 rawValue = static_cast<quint16>(index.data(Qt::UserRole).toUInt());
                switch (le->inputMode()) {
                    case NumericLineEdit::Int32Mode:
                        le->setValue<qint32>(static_cast<qint16>(rawValue));
                        break;
                    case NumericLineEdit::HexMode:
                        le->setValue<quint16>(rawValue);
                        break;
                    default:
                        le->setValue<quint32>(rawValue);
                        break;
                }
                break;
            }
        }
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        auto* le = qobject_cast<NumericLineEdit*>(editor);
        if (!le) return;

        const DataType type = enumFromString<DataType>(
            index.siblingAtColumn(ColDataType).data(Qt::DisplayRole).toString(), DataType::Int16);

        QString text;
        switch (type) {
            case DataType::Float32:
                text = QString::number(le->value<float>());
                break;
            case DataType::Float64:
                text = QString::number(le->value<double>());
                break;
            case DataType::Int32:
                text = QString::number(le->value<qint32>());
                break;
            case DataType::UInt32:
                text = QString::number(le->value<quint32>());
                break;
            case DataType::Int64:
                text = QString::number(le->value<qint64>());
                break;
            case DataType::UInt64:
                text = QString::number(le->value<quint64>());
                break;
            default: {
                quint16 newValue;
                switch (le->inputMode()) {
                    case NumericLineEdit::Int32Mode:
                        newValue = static_cast<quint16>(le->value<qint32>());
                        text = QString::number(static_cast<qint16>(newValue));
                        break;
                    case NumericLineEdit::HexMode:
                        newValue = le->value<quint16>();
                        text = QStringLiteral("0x%1").arg(newValue, 4, 16, QChar('0')).toUpper();
                        break;
                    default:
                        newValue = static_cast<quint16>(le->value<quint32>());
                        text = QString::number(newValue);
                        break;
                }
                break;
            }
        }
        model->setData(index, text, Qt::EditRole);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
        editor->setGeometry(option.rect);
    }
};

} // anonymous namespace

///
/// \brief FormRegisterMapView::FormRegisterMapView
///
FormRegisterMapView::FormRegisterMapView(ModbusMultiServer& server, MainWindow* parent)
    : QWidget(static_cast<QWidget*>(parent))
    , ui(new Ui::FormRegisterMapView)
    , _mbMultiServer(server)
{
    ui->setupUi(this);

    auto* hdr = ui->tableWidget->horizontalHeader();
    auto hdrFont = hdr->font();
    hdrFont.setBold(true);
    hdr->setFont(hdrFont);

    hdr->setSectionResizeMode(ColUnit,      QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColType,      QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColAddress,   QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColDataType,  QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColOrder,     QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColComment,   QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColValue,     QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColTimestamp, QHeaderView::Interactive);

    hdr->resizeSection(ColUnit,      40);
    hdr->resizeSection(ColType,      120);
    hdr->resizeSection(ColAddress,   70);
    hdr->resizeSection(ColDataType,  70);
    hdr->resizeSection(ColOrder,     65);
    hdr->resizeSection(ColComment,   200);
    hdr->resizeSection(ColValue,     160);
    hdr->resizeSection(ColTimestamp, 160);

    ui->tableWidget->verticalHeader()->setDefaultSectionSize(20);
    ui->tableWidget->verticalHeader()->hide();

    ui->tableWidget->setItemDelegateForColumn(ColUnit,     new UnitItemDelegate(ui->tableWidget));
    ui->tableWidget->setItemDelegateForColumn(ColType,     new TypeItemDelegate(ui->tableWidget));
    ui->tableWidget->setItemDelegateForColumn(ColAddress,  new AddressItemDelegate(ui->tableWidget));
    ui->tableWidget->setItemDelegateForColumn(ColDataType, new DataTypeItemDelegate(ui->tableWidget));
    ui->tableWidget->setItemDelegateForColumn(ColOrder,    new OrderItemDelegate(ui->tableWidget));
    ui->tableWidget->setItemDelegateForColumn(ColValue,    new ValueItemDelegate(ui->tableWidget));

    auto* spacer = new QWidget(ui->toolBar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar->insertWidget(ui->actionClear, spacer);

    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        menu.addAction(ui->actionAdd);
        menu.addAction(ui->actionDelete);
        menu.addSeparator();
        menu.addAction(ui->actionClear);
        menu.exec(ui->tableWidget->viewport()->mapToGlobal(pos));
    });

    connect(ui->tableWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &FormRegisterMapView::updateActionState);
    connect(ui->tableWidget->model(), &QAbstractItemModel::rowsInserted,
            this, &FormRegisterMapView::updateActionState);
    connect(ui->tableWidget->model(), &QAbstractItemModel::rowsRemoved,
            this, &FormRegisterMapView::updateActionState);

    updateActionState();
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
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
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

    const QDateTime now = QDateTime::currentDateTime();
    for (quint32 i = 0; i < data.valueCount(); ++i) {
        const ItemMapKey key{ deviceId, data.registerType(), static_cast<quint16>(data.startAddress() + i) };
        const quint16 value = static_cast<quint16>(data.value(i));

        auto it = _registerMap.find(key);
        if (it != _registerMap.end()) {
            if (it->value != value) {
                it->value     = value;
                it->timestamp = now;
                const int row = findRow(key);
                if (row >= 0) updateValue(row, key, value);
            }
        }
    }
}

///
/// \brief FormRegisterMapView::on_actionAdd_triggered
///
void FormRegisterMapView::on_actionAdd_triggered()
{
    // Default: Unit=1, HoldingRegisters, address=0 (internal 0-based), Int16 format
    ItemMapKey key{ 1, QModbusDataUnit::HoldingRegisters, 0 };

    // If the key already exists, shift the address to avoid collision
    while (_registerMap.contains(key)) {
        if (key.Address < 0xFFFF) ++key.Address;
        else break;
    }

    Entry entry;
    entry.type      = DataType::Int16;
    entry.order     = RegisterOrder::MSRF;
    entry.timestamp = _mbMultiServer.timestamp(key.DeviceId, key.Type, key.Address);
    const auto unit = _mbMultiServer.data(key.DeviceId, key.Type, key.Address, 1);
    entry.value = unit.isValid() ? static_cast<quint16>(unit.value(0)) : 0;

    _registerMap[key] = entry;
    insertEntry(key, entry);

    // Start editing the Unit cell of the new row
    const int newRow = ui->tableWidget->rowCount() - 1;
    ui->tableWidget->setCurrentCell(newRow, ColUnit);
    ui->tableWidget->editItem(ui->tableWidget->item(newRow, ColUnit));
    updateActionState();
}

///
/// \brief FormRegisterMapView::on_actionDelete_triggered
///
void FormRegisterMapView::on_actionDelete_triggered()
{
    const auto selectedRanges = ui->tableWidget->selectedRanges();
    if (selectedRanges.isEmpty()) return;

    // Collect rows in descending order to remove without shifting indices
    QList<int> rows;
    for (const auto& range : selectedRanges) {
        for (int r = range.topRow(); r <= range.bottomRow(); ++r)
            if (!rows.contains(r)) rows.append(r);
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    _updatingTable = true;
    for (int row : rows) {
        const ItemMapKey key = keyFromRow(row);
        _registerMap.remove(key);
        ui->tableWidget->removeRow(row);
    }
    _updatingTable = false;
    updateActionState();
}

///
/// \brief FormRegisterMapView::on_actionClear_triggered
///
void FormRegisterMapView::on_actionClear_triggered()
{
    _registerMap.clear();
    _updatingTable = true;
    ui->tableWidget->setRowCount(0);
    _updatingTable = false;
    updateActionState();
}

///
/// \brief FormRegisterMapView::updateActionState
///
void FormRegisterMapView::updateActionState()
{
    ui->actionDelete->setEnabled(!ui->tableWidget->selectedRanges().isEmpty());
    ui->actionClear->setEnabled(ui->tableWidget->rowCount() > 0);
}

///
/// \brief FormRegisterMapView::on_tableWidget_cellChanged
///
void FormRegisterMapView::on_tableWidget_cellChanged(int row, int col)
{
    if (_updatingTable) return;

    // Handle Value column
    if (col == ColValue) {
        const ItemMapKey key = keyFromRow(row);
        auto it = _registerMap.find(key);
        if (it == _registerMap.end()) return;

        auto* valItem = ui->tableWidget->item(row, ColValue);
        if (!valItem) return;

        bool ok = false;
        const QString text = valItem->text().trimmed();
        const int regCount = registersCount(it->type);
        const bool lsrf = (it->order == RegisterOrder::LSRF);
        QVector<quint16> regs(regCount, 0);

        switch (it->type) {
            case DataType::Float32: {
                const float f = text.toFloat(&ok);
                if (ok) {
                    if (lsrf) breakFloat(f, regs[0], regs[1], ByteOrder::Direct);
                    else       breakFloat(f, regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            case DataType::Float64: {
                const double d = text.toDouble(&ok);
                if (ok) {
                    if (lsrf) breakDouble(d, regs[0], regs[1], regs[2], regs[3], ByteOrder::Direct);
                    else       breakDouble(d, regs[3], regs[2], regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            case DataType::Int32: {
                const qint32 v = static_cast<qint32>(text.toLong(&ok));
                if (ok) {
                    if (lsrf) breakInt32(v, regs[0], regs[1], ByteOrder::Direct);
                    else       breakInt32(v, regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            case DataType::UInt32: {
                const quint32 v = static_cast<quint32>(text.toULong(&ok));
                if (ok) {
                    if (lsrf) breakUInt32(v, regs[0], regs[1], ByteOrder::Direct);
                    else       breakUInt32(v, regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            case DataType::Int64: {
                const qint64 v = text.toLongLong(&ok);
                if (ok) {
                    if (lsrf) breakInt64(v, regs[0], regs[1], regs[2], regs[3], ByteOrder::Direct);
                    else       breakInt64(v, regs[3], regs[2], regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            case DataType::UInt64: {
                const quint64 v = text.toULongLong(&ok);
                if (ok) {
                    if (lsrf) breakUInt64(v, regs[0], regs[1], regs[2], regs[3], ByteOrder::Direct);
                    else       breakUInt64(v, regs[3], regs[2], regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            default: {
                quint16 newValue = 0;
                if (text.startsWith("0x") || text.startsWith("0X"))
                    newValue = static_cast<quint16>(text.toUInt(&ok, 16));
                else if (it->type == DataType::Int16)
                    newValue = static_cast<quint16>(text.toShort(&ok));
                else
                    newValue = text.toUShort(&ok);
                if (ok) regs[0] = newValue;
                break;
            }
        }

        if (!ok) {
            // Restore previous value on invalid input
            _updatingTable = true;
            valItem->setText(formatValue(key.Type, it->type, it->order, regsForKey(key, it->type)));
            _updatingTable = false;
            return;
        }

        it->value = regs[0];
        it->timestamp = QDateTime::currentDateTime();

        QModbusDataUnit unit(key.Type, key.Address, regCount);
        for (int i = 0; i < regCount; ++i) unit.setValue(i, regs[i]);
        _mbMultiServer.setData(key.DeviceId, unit);

        _updatingTable = true;
        valItem->setData(Qt::UserRole, static_cast<quint32>(regs[0]));
        valItem->setText(formatValue(key.Type, it->type, it->order, regsForKey(key, it->type)));
        auto* tsItem = ui->tableWidget->item(row, ColTimestamp);
        if (tsItem) tsItem->setText(it->timestamp.toString(Qt::ISODate));
        _updatingTable = false;
        return;
    }

    // Handle Comment column
    if (col == ColComment) {
        const ItemMapKey key = keyFromRow(row);
        auto it = _registerMap.find(key);
        if (it != _registerMap.end()) {
            const auto* item = ui->tableWidget->item(row, ColComment);
            it->comment = item ? item->text() : QString();
        }
        return;
    }

    // Handle DataType column
    if (col == ColDataType) {
        const ItemMapKey key = keyFromRow(row);
        auto it = _registerMap.find(key);
        if (it == _registerMap.end()) return;

        const auto* typeItem = ui->tableWidget->item(row, ColDataType);
        if (!typeItem) return;
        it->type = enumFromString<DataType>(typeItem->text(), DataType::Int16);

        _updatingTable = true;
        auto* orderItem = ui->tableWidget->item(row, ColOrder);
        if (orderItem) {
            if (isMultiRegisterType(it->type)) {
                // Enable editing and show current order for multi-register types
                orderItem->setFlags(orderItem->flags() | Qt::ItemIsEditable);
                orderItem->setText(enumToString(it->order));
            } else {
                // Disable editing and clear cell for single-register types
                it->order = RegisterOrder::MSRF;
                orderItem->setFlags(orderItem->flags() & ~Qt::ItemIsEditable);
                orderItem->setText(QString());
            }
        }
        _updatingTable = false;

        // Update displayed value with new type (read all required registers from server)
        _updatingTable = true;
        auto* valItem = ui->tableWidget->item(row, ColValue);
        if (valItem) valItem->setText(formatValue(key.Type, it->type, it->order, regsForKey(key, it->type)));
        _updatingTable = false;
        return;
    }

    // Handle Order column
    if (col == ColOrder) {
        const ItemMapKey key = keyFromRow(row);
        auto it = _registerMap.find(key);
        if (it == _registerMap.end()) return;

        const auto* orderItem = ui->tableWidget->item(row, ColOrder);
        if (!orderItem) return;
        it->order = enumFromString<RegisterOrder>(orderItem->text(), RegisterOrder::MSRF);

        _updatingTable = true;
        auto* valItem = ui->tableWidget->item(row, ColValue);
        if (valItem) valItem->setText(formatValue(key.Type, it->type, it->order, regsForKey(key, it->type)));
        _updatingTable = false;
        return;
    }

    // Handle Unit (col 0), Type (col 1), Address (col 2) — key change
    if (col == ColUnit || col == ColType || col == ColAddress) {
        // Read old key stored in UserRole
        auto* unitItem = ui->tableWidget->item(row, ColUnit);
        if (!unitItem) return;

        const ItemMapKey oldKey = keyFromRow(row);

        // Build new key from current cell texts
        const auto* typeItem    = ui->tableWidget->item(row, ColType);
        const auto* addrItem    = ui->tableWidget->item(row, ColAddress);

        const quint8  newDevId  = static_cast<quint8>(unitItem->text().toUShort());
        const auto    newType   = stringToRegisterType(typeItem ? typeItem->text() : QString());
        const quint16 newAddr   = addressFromDisplay(addrItem ? addrItem->text() : QStringLiteral("1"));

        const ItemMapKey newKey{ newDevId, newType, newAddr };

        if (newKey.DeviceId == oldKey.DeviceId &&
            newKey.Type    == oldKey.Type    &&
            newKey.Address == oldKey.Address) return;

        // Preserve existing entry data
        Entry entry = _registerMap.value(oldKey);
        _registerMap.remove(oldKey);

        // Read fresh value from server
        const auto unit = _mbMultiServer.data(newDevId, newType, newAddr, 1);
        entry.value = unit.isValid() ? static_cast<quint16>(unit.value(0)) : 0;
        entry.timestamp = unit.isValid() ? QDateTime::currentDateTime() : QDateTime();
        _registerMap[newKey] = entry;

        // Update UserRole data and dependent cells
        _updatingTable = true;
        unitItem->setData(RoleDeviceId, static_cast<int>(newKey.DeviceId));
        unitItem->setData(RoleType,     static_cast<int>(newKey.Type));
        unitItem->setData(RoleAddress,  static_cast<int>(newKey.Address));

        auto* valItem = ui->tableWidget->item(row, ColValue);
        if (valItem) valItem->setText(formatValue(newType, entry.type, entry.order, regsForKey(newKey, entry.type)));

        auto* tsItem = ui->tableWidget->item(row, ColTimestamp);
        if (tsItem) tsItem->setText(entry.timestamp.isValid()
                                        ? entry.timestamp.toString(Qt::ISODate)
                                        : QString());
        _updatingTable = false;
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
                it->timestamp = QDateTime::currentDateTime();
                const int row = findRow(key);
                if (row >= 0) updateValue(row, key, value);
            }
        } else {
            Entry entry;
            entry.value = value;
            entry.type  = DataType::Int16;
            entry.order = RegisterOrder::MSRF;
            entry.timestamp = QDateTime::currentDateTime();
            _registerMap[key] = entry;
            insertEntry(key, entry);
        }
    }
}

///
/// \brief FormRegisterMapView::findRow
///
int FormRegisterMapView::findRow(const ItemMapKey& key) const
{
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        const auto* item = ui->tableWidget->item(i, ColUnit);
        if (!item) continue;
        if (item->data(RoleDeviceId).toInt() == static_cast<int>(key.DeviceId) &&
            item->data(RoleType).toInt()     == static_cast<int>(key.Type)     &&
            item->data(RoleAddress).toInt()  == static_cast<int>(key.Address))
            return i;
    }
    return -1;
}

///
/// \brief FormRegisterMapView::insertEntry
///
void FormRegisterMapView::insertEntry(const ItemMapKey& key, const Entry& entry)
{
    _updatingTable = true;

    const int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    // Col 0: Unit (editable) — stores key in UserRole
    auto* unitItem = new QTableWidgetItem(QString::number(key.DeviceId));
    unitItem->setData(RoleDeviceId, static_cast<int>(key.DeviceId));
    unitItem->setData(RoleType,     static_cast<int>(key.Type));
    unitItem->setData(RoleAddress,  static_cast<int>(key.Address));
    unitItem->setTextAlignment(Qt::AlignCenter);

    // Col 1: Type (editable via delegate)
    auto* typeItem = new QTableWidgetItem(registerTypeToString(key.Type));
    typeItem->setTextAlignment(Qt::AlignCenter);

    // Col 2: Address (editable)
    auto* addrItem = new QTableWidgetItem(addressToDisplay(key.Address));
    addrItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Col 3: DataType (editable via delegate)
    auto* dataTypeItem = new QTableWidgetItem(enumToString(entry.type));
    dataTypeItem->setTextAlignment(Qt::AlignCenter);

    // Col 4: Order (editable only for multi-register types)
    const bool multiReg = isMultiRegisterType(entry.type);
    auto* orderItem = new QTableWidgetItem(multiReg ? enumToString(entry.order) : QString());
    orderItem->setTextAlignment(Qt::AlignCenter);
    if (!multiReg)
        orderItem->setFlags(orderItem->flags() & ~Qt::ItemIsEditable);

    // Col 5: Comment (editable)
    auto* commentItem = new QTableWidgetItem(entry.comment);

    // Col 6: Value (editable)
    auto* valItem = new QTableWidgetItem(formatValue(key.Type, entry.type, entry.order, regsForKey(key, entry.type)));
    valItem->setData(Qt::UserRole, static_cast<quint32>(entry.value));
    valItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Col 7: Timestamp (read-only)
    auto* tsItem = new QTableWidgetItem(entry.timestamp.isValid()
                                            ? entry.timestamp.toString(Qt::ISODate)
                                            : QString());
    tsItem->setFlags(tsItem->flags() & ~Qt::ItemIsEditable);
    tsItem->setTextAlignment(Qt::AlignCenter);

    ui->tableWidget->setItem(row, ColUnit,     unitItem);
    ui->tableWidget->setItem(row, ColType,     typeItem);
    ui->tableWidget->setItem(row, ColAddress,  addrItem);
    ui->tableWidget->setItem(row, ColDataType, dataTypeItem);
    ui->tableWidget->setItem(row, ColOrder,    orderItem);
    ui->tableWidget->setItem(row, ColComment,  commentItem);
    ui->tableWidget->setItem(row, ColValue,    valItem);
    ui->tableWidget->setItem(row, ColTimestamp, tsItem);

    _updatingTable = false;
}

///
/// \brief FormRegisterMapView::updateValue
///
void FormRegisterMapView::updateValue(int row, const ItemMapKey& key, quint16 value)
{
    auto it = _registerMap.find(key);
    const DataType      type  = (it != _registerMap.end()) ? it->type  : DataType::Int16;
    const RegisterOrder order = (it != _registerMap.end()) ? it->order : RegisterOrder::MSRF;
    const QDateTime ts        = (it != _registerMap.end()) ? it->timestamp : QDateTime::currentDateTime();

    _updatingTable = true;
    auto* valItem = ui->tableWidget->item(row, ColValue);
    if (valItem) {
        valItem->setData(Qt::UserRole, static_cast<quint32>(value));
        valItem->setText(formatValue(key.Type, type, order, regsForKey(key, type)));
    }

    auto* tsItem = ui->tableWidget->item(row, ColTimestamp);
    if (tsItem) tsItem->setText(ts.toString(Qt::ISODate));
    _updatingTable = false;
}

///
/// \brief FormRegisterMapView::updateAddressCells
/// Refreshes all address cells when global address base setting changes.
///
void FormRegisterMapView::updateAddressCells()
{
    _updatingTable = true;
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        const auto* unitItem = ui->tableWidget->item(row, ColUnit);
        if (!unitItem) continue;
        const quint16 rawAddr = static_cast<quint16>(unitItem->data(RoleAddress).toInt());
        auto* addrItem = ui->tableWidget->item(row, ColAddress);
        if (addrItem) addrItem->setText(addressToDisplay(rawAddr));
    }
    _updatingTable = false;
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
/// \brief FormRegisterMapView::stringToRegisterType
///
QModbusDataUnit::RegisterType FormRegisterMapView::stringToRegisterType(const QString& str) const
{
    if (str == tr("Coils"))            return QModbusDataUnit::Coils;
    if (str == tr("Discrete Inputs"))  return QModbusDataUnit::DiscreteInputs;
    if (str == tr("Input Registers"))  return QModbusDataUnit::InputRegisters;
    return QModbusDataUnit::HoldingRegisters;
}

///
/// \brief FormRegisterMapView::regsForKey
///
QVector<quint16> FormRegisterMapView::regsForKey(const ItemMapKey& key, DataType type) const
{
    const int count = registersCount(type);
    const QModbusDataUnit unit = _mbMultiServer.data(key.DeviceId, key.Type, key.Address, count);
    QVector<quint16> regs;
    for (int i = 0; i < count; ++i)
        regs << (unit.isValid() ? static_cast<quint16>(unit.value(i)) : 0);
    return regs;
}

///
/// \brief FormRegisterMapView::formatValue
///
QString FormRegisterMapView::formatValue(QModbusDataUnit::RegisterType regType,
                                         DataType type, RegisterOrder order, const QVector<quint16>& regs) const
{
    if (regs.isEmpty()) return QString();
    QVariant outValue;
    switch (type) {
        case DataType::Binary:
            return formatBinaryValue(regType, regs[0], ByteOrder::Direct, outValue, false);
        case DataType::UInt16:
            return formatUInt16Value(regType, regs[0], ByteOrder::Direct, false, outValue, false);
        case DataType::Int16:
            return formatInt16Value(regType, static_cast<qint16>(regs[0]), ByteOrder::Direct, outValue, false);
        default:
            if (isMultiRegisterType(type) && regs.size() >= registersCount(type)) {
                const QVariant val = makeValue(regs, type, order, ByteOrder::Direct);
                if (val.isValid()) return val.toString();
            }
            return formatHexValue(regType, regs[0], ByteOrder::Direct, outValue, false);
    }
}

///
/// \brief FormRegisterMapView::addressToDisplay
///
QString FormRegisterMapView::addressToDisplay(quint16 addr) const
{
    const bool zeroBased = AppPreferences::instance().dataViewDefinitions().ZeroBasedAddress;
    return QString::number(zeroBased ? addr : static_cast<quint32>(addr) + 1);
}

///
/// \brief FormRegisterMapView::addressFromDisplay
///
quint16 FormRegisterMapView::addressFromDisplay(const QString& text, bool* ok) const
{
    bool localOk = false;
    const quint32 v = text.toUInt(&localOk);
    if (ok) *ok = localOk;
    if (!localOk) return 0;

    const bool zeroBased = AppPreferences::instance().dataViewDefinitions().ZeroBasedAddress;
    if (zeroBased)
        return static_cast<quint16>(qMin(v, static_cast<quint32>(0xFFFF)));

    // Base-1: subtract 1, clamp to 0
    return static_cast<quint16>(v > 0 ? qMin(v - 1, static_cast<quint32>(0xFFFF)) : 0);
}

///
/// \brief FormRegisterMapView::keyFromRow
/// Reads the ItemMapKey stored in UserRole data of the Unit cell.
///
ItemMapKey FormRegisterMapView::keyFromRow(int row) const
{
    const auto* item = ui->tableWidget->item(row, ColUnit);
    if (!item) return { 0, QModbusDataUnit::HoldingRegisters, 0 };
    ItemMapKey key;
    key.DeviceId = static_cast<quint8>(item->data(RoleDeviceId).toInt());
    key.Type     = static_cast<QModbusDataUnit::RegisterType>(item->data(RoleType).toInt());
    key.Address  = static_cast<quint16>(item->data(RoleAddress).toInt());
    return key;
}

///
/// \brief FormRegisterMapView::columnWidths
///
QList<int> FormRegisterMapView::columnWidths() const
{
    const auto* hdr = ui->tableWidget->horizontalHeader();
    QList<int> widths;
    for (int i = 0; i < hdr->count(); ++i)
        widths.append(hdr->sectionSize(i));
    return widths;
}

///
/// \brief FormRegisterMapView::setColumnWidths
///
void FormRegisterMapView::setColumnWidths(const QList<int>& widths)
{
    auto* hdr = ui->tableWidget->horizontalHeader();
    for (int i = 0; i < widths.size() && i < hdr->count(); ++i)
        if (widths[i] > 0) hdr->resizeSection(i, widths[i]);
}

