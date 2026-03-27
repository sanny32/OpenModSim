#include <QtWidgets>
#include "mainwindow.h"
#include "apppreferences.h"
#include "formregistermapview.h"
#include "formatutils.h"
#include "modbusmessages/modbusmessages.h"
#include "controls/numericlineedit.h"
#include "ui_formregistermapview.h"

namespace {

constexpr int RoleDeviceId  = Qt::UserRole;
constexpr int RoleType      = Qt::UserRole + 1;
constexpr int RoleAddress   = Qt::UserRole + 2;
constexpr int RoleTypeValue = Qt::UserRole + 3;

enum Col { ColUnit = 0, ColType, ColAddress, ColFormat, ColComment, ColValue, ColTimestamp };

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
/// \brief FormatItemDelegate — inline combo box for DataDisplayMode column
///
class FormatItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        auto* cb = new QComboBox(parent);
        for (auto it = EnumStrings<DataDisplayMode>::mapping().cbegin();
             it != EnumStrings<DataDisplayMode>::mapping().cend(); ++it) {
            cb->addItem(it.value(), static_cast<int>(it.key()));
        }
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

        const QString fmtStr = index.siblingAtColumn(ColFormat).data(Qt::DisplayRole).toString();
        DataDisplayMode fmt = DataDisplayMode::Int16;
        for (auto it = EnumStrings<DataDisplayMode>::mapping().cbegin();
             it != EnumStrings<DataDisplayMode>::mapping().cend(); ++it) {
            if (it.value() == fmtStr) { fmt = it.key(); break; }
        }

        auto* editor = new NumericLineEdit(parent);

        if (regType == QModbusDataUnit::Coils || regType == QModbusDataUnit::DiscreteInputs) {
            editor->setInputMode(NumericLineEdit::UInt32Mode);
            editor->setInputRange<quint32>(0, 1);
        } else {
            switch (fmt) {
                case DataDisplayMode::Int16:
                    editor->setInputMode(NumericLineEdit::Int32Mode);
                    editor->setInputRange<qint32>(-32768, 32767);
                    break;
                case DataDisplayMode::UInt16:
                    editor->setInputMode(NumericLineEdit::UInt32Mode);
                    editor->setInputRange<quint32>(0, 65535);
                    break;
                default: // Binary, Hex, and multi-register formats
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
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        auto* le = qobject_cast<NumericLineEdit*>(editor);
        if (!le) return;

        quint16 newValue;
        QString text;
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
    hdr->setSectionResizeMode(ColUnit,      QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColType,      QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColAddress,   QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColFormat,    QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColComment,   QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColValue,     QHeaderView::Interactive);
    hdr->setSectionResizeMode(ColTimestamp, QHeaderView::Interactive);

    hdr->resizeSection(ColUnit,      40);
    hdr->resizeSection(ColType,      120);
    hdr->resizeSection(ColAddress,   70);
    hdr->resizeSection(ColFormat,    80);
    hdr->resizeSection(ColComment,   200);
    hdr->resizeSection(ColValue,     140);
    hdr->resizeSection(ColTimestamp, 160);

    ui->tableWidget->verticalHeader()->setDefaultSectionSize(20);
    ui->tableWidget->verticalHeader()->hide();

    ui->tableWidget->setItemDelegateForColumn(ColUnit,    new UnitItemDelegate(ui->tableWidget));
    ui->tableWidget->setItemDelegateForColumn(ColType,   new TypeItemDelegate(ui->tableWidget));
    ui->tableWidget->setItemDelegateForColumn(ColAddress, new AddressItemDelegate(ui->tableWidget));
    ui->tableWidget->setItemDelegateForColumn(ColFormat, new FormatItemDelegate(ui->tableWidget));
    ui->tableWidget->setItemDelegateForColumn(ColValue,  new ValueItemDelegate(ui->tableWidget));

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
    entry.format    = DataDisplayMode::Int16;
    entry.timestamp = _mbMultiServer.timestamp(key.DeviceId, key.Type, key.Address);
    const auto unit = _mbMultiServer.data(key.DeviceId, key.Type, key.Address, 1);
    entry.value = unit.isValid() ? static_cast<quint16>(unit.value(0)) : 0;

    _registerMap[key] = entry;
    insertEntry(key, entry);

    // Start editing the Unit cell of the new row
    const int newRow = ui->tableWidget->rowCount() - 1;
    ui->tableWidget->setCurrentCell(newRow, ColUnit);
    ui->tableWidget->editItem(ui->tableWidget->item(newRow, ColUnit));
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
        quint16 newValue = 0;
        if (text.startsWith("0x") || text.startsWith("0X"))
            newValue = static_cast<quint16>(text.toUInt(&ok, 16));
        else if (it->format == DataDisplayMode::Int16)
            newValue = static_cast<quint16>(text.toShort(&ok));
        else
            newValue = text.toUShort(&ok);

        if (!ok) {
            // Restore previous value on invalid input
            _updatingTable = true;
            valItem->setText(formatValue(key.Type, it->format, it->value));
            _updatingTable = false;
            return;
        }

        it->value = newValue;
        it->timestamp = QDateTime::currentDateTime();

        QModbusDataUnit unit(key.Type, key.Address, 1);
        unit.setValue(0, newValue);
        _mbMultiServer.setData(key.DeviceId, unit);

        _updatingTable = true;
        valItem->setData(Qt::UserRole, static_cast<quint32>(newValue));
        valItem->setText(formatValue(key.Type, it->format, newValue));
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

    // Handle Format column
    if (col == ColFormat) {
        const ItemMapKey key = keyFromRow(row);
        auto it = _registerMap.find(key);
        if (it == _registerMap.end()) return;

        const auto* fmtItem = ui->tableWidget->item(row, ColFormat);
        if (!fmtItem) return;
        const QString fmtStr = fmtItem->text();
        for (auto mit = EnumStrings<DataDisplayMode>::mapping().cbegin();
             mit != EnumStrings<DataDisplayMode>::mapping().cend(); ++mit) {
            if (mit.value() == fmtStr) {
                it->format = mit.key();
                break;
            }
        }

        // Update displayed value with new format
        _updatingTable = true;
        auto* valItem = ui->tableWidget->item(row, ColValue);
        if (valItem) valItem->setText(formatValue(key.Type, it->format, it->value));
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
        if (valItem) valItem->setText(formatValue(newType, entry.format, entry.value));

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
            entry.format = DataDisplayMode::Int16;
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

    // Col 3: Format (editable via delegate)
    auto* fmtItem = new QTableWidgetItem(EnumStrings<DataDisplayMode>::mapping().value(entry.format));
    fmtItem->setTextAlignment(Qt::AlignCenter);

    // Col 4: Comment (editable)
    auto* commentItem = new QTableWidgetItem(entry.comment);

    // Col 5: Value (editable)
    auto* valItem = new QTableWidgetItem(formatValue(key.Type, entry.format, entry.value));
    valItem->setData(Qt::UserRole, static_cast<quint32>(entry.value));
    valItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Col 6: Timestamp (read-only)
    auto* tsItem = new QTableWidgetItem(entry.timestamp.isValid()
                                            ? entry.timestamp.toString(Qt::ISODate)
                                            : QString());
    tsItem->setFlags(tsItem->flags() & ~Qt::ItemIsEditable);
    tsItem->setTextAlignment(Qt::AlignCenter);

    ui->tableWidget->setItem(row, ColUnit,      unitItem);
    ui->tableWidget->setItem(row, ColType,      typeItem);
    ui->tableWidget->setItem(row, ColAddress,   addrItem);
    ui->tableWidget->setItem(row, ColFormat,    fmtItem);
    ui->tableWidget->setItem(row, ColComment,   commentItem);
    ui->tableWidget->setItem(row, ColValue,     valItem);
    ui->tableWidget->setItem(row, ColTimestamp, tsItem);

    _updatingTable = false;
}

///
/// \brief FormRegisterMapView::updateValue
///
void FormRegisterMapView::updateValue(int row, const ItemMapKey& key, quint16 value)
{
    auto it = _registerMap.find(key);
    const DataDisplayMode fmt = (it != _registerMap.end()) ? it->format : DataDisplayMode::Int16;
    const QDateTime ts = (it != _registerMap.end()) ? it->timestamp : QDateTime::currentDateTime();

    _updatingTable = true;
    auto* valItem = ui->tableWidget->item(row, ColValue);
    if (valItem) {
        valItem->setData(Qt::UserRole, static_cast<quint32>(value));
        valItem->setText(formatValue(key.Type, fmt, value));
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
/// \brief FormRegisterMapView::formatValue
///
QString FormRegisterMapView::formatValue(QModbusDataUnit::RegisterType type,
                                         DataDisplayMode fmt, quint16 value) const
{
    QVariant outValue;
    switch (fmt) {
        case DataDisplayMode::Binary:
            return formatBinaryValue(type, value, ByteOrder::Direct, outValue, false);
        case DataDisplayMode::UInt16:
            return formatUInt16Value(type, value, ByteOrder::Direct, false, outValue, false);
        case DataDisplayMode::Int16:
            return formatInt16Value(type, static_cast<qint16>(value), ByteOrder::Direct, outValue, false);
        default:
            return formatHexValue(type, value, ByteOrder::Direct, outValue, false);
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

