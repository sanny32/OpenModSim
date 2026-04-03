#include <QtWidgets>
#include "mainwindow.h"
#include "apppreferences.h"
#include "formregistermapview.h"
#include "modbusmessages/modbusmessages.h"
#include "controls/numericlineedit.h"
#include "formatutils.h"
#include "ui_formregistermapview.h"

namespace {

// Role aliases so delegates compile without change
constexpr int RoleDeviceId  = RegisterMapRole::DeviceId;
constexpr int RoleType      = RegisterMapRole::Type;
constexpr int RoleAddress   = RegisterMapRole::Address;
constexpr int RoleTypeValue = RegisterMapRole::TypeValue;

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

    static bool isBitType(const QModelIndex& index)
    {
        const int regType = index.siblingAtColumn(ColType).data(RoleTypeValue).toInt();
        return regType == QModbusDataUnit::Coils ||
               regType == QModbusDataUnit::DiscreteInputs;
    }

    static QString tooltipFor(DataType type, bool bitType = false)
    {
        switch (type) {
            case DataType::Binary:  return bitType
                                        ? tr("1-bit value (Coils / Discrete Inputs)")
                                        : tr("16-bit register value shown as 16 binary digits");
            case DataType::UInt16:  return tr("Unsigned 16-bit integer  (0 … 65535)");
            case DataType::Int16:   return tr("Signed 16-bit integer  (−32768 … 32767)");
            case DataType::Hex:     return tr("16-bit value displayed as hexadecimal");
            case DataType::Ansi:    return tr("16-bit value displayed as ANSI character");
            case DataType::Float32: return tr("IEEE 754 single-precision float  (2 registers)");
            case DataType::Float64: return tr("IEEE 754 double-precision float  (4 registers)");
            case DataType::Int32:   return tr("Signed 32-bit integer  (2 registers)");
            case DataType::UInt32:  return tr("Unsigned 32-bit integer  (2 registers)");
            case DataType::Int64:   return tr("Signed 64-bit integer  (4 registers)");
            case DataType::UInt64:  return tr("Unsigned 64-bit integer  (4 registers)");
        }
        return {};
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
    {
        if (isBitType(index))
            return nullptr;

        auto* cb = new QComboBox(parent);
        for (auto it = EnumStrings<DataType>::mapping().cbegin();
             it != EnumStrings<DataType>::mapping().cend(); ++it) {
            cb->addItem(it.value(), static_cast<int>(it.key()));
            cb->setItemData(cb->count() - 1, tooltipFor(it.key()), Qt::ToolTipRole);
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
        const int idx = cb->findText(index.data(Qt::EditRole).toString());
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

    bool helpEvent(QHelpEvent* e, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index) override
    {
        if (e->type() == QEvent::ToolTip) {
            const bool bit = isBitType(index);
            const QString tip = tooltipFor(enumFromString<DataType>(index.data(Qt::DisplayRole).toString(), DataType::Int16), bit);
            QToolTip::showText(e->globalPos(), tip, view);
            return true;
        }
        return QStyledItemDelegate::helpEvent(e, view, option, index);
    }
};

///
/// \brief OrderItemDelegate — inline combo box for RegisterOrder column
///
class OrderItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    static QString tooltipFor(RegisterOrder order)
    {
        switch (order) {
            case RegisterOrder::MSRF: return tr("Most Significant Register First — big-endian word order");
            case RegisterOrder::LSRF: return tr("Least Significant Register First — little-endian word order");
        }
        return {};
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        auto* cb = new QComboBox(parent);
        for (auto it = EnumStrings<RegisterOrder>::mapping().cbegin();
             it != EnumStrings<RegisterOrder>::mapping().cend(); ++it) {
            cb->addItem(it.value(), static_cast<int>(it.key()));
            cb->setItemData(cb->count() - 1, tooltipFor(it.key()), Qt::ToolTipRole);
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
        const int idx = cb->findText(index.data(Qt::EditRole).toString());
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

    bool helpEvent(QHelpEvent* e, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index) override
    {
        if (e->type() == QEvent::ToolTip) {
            const auto order = enumFromString<RegisterOrder>(index.data(Qt::DisplayRole).toString(), RegisterOrder::MSRF);
            QToolTip::showText(e->globalPos(), tooltipFor(order), view);
            return true;
        }
        return QStyledItemDelegate::helpEvent(e, view, option, index);
    }
};

///
/// \brief UnitItemDelegate — NumericLineEdit editor for the Unit (device ID) column
///
class UnitItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    static bool hexViewFromIndex(const QModelIndex& index)
    {
        auto* proxy = qobject_cast<const QSortFilterProxyModel*>(index.model());
        auto* src = proxy ? qobject_cast<RegisterMapDataModel*>(proxy->sourceModel()) : nullptr;
        return src ? src->hexView() : false;
    }

    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        if (hexViewFromIndex(index)) {
            bool ok;
            const quint8 v = static_cast<quint8>(option->text.toUShort(&ok));
            if (ok) option->text = formatUInt8Value(DataType::Hex, false, v);
        }
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
    {
        auto* editor = new NumericLineEdit(parent);
        editor->setInputMode(NumericLineEdit::UInt32Mode);
        editor->setInputRange<quint32>(1, 255);
        editor->setHexView(hexViewFromIndex(index));
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

    static RegisterMapDataModel* sourceModel(const QModelIndex& index)
    {
        auto* proxy = qobject_cast<const QSortFilterProxyModel*>(index.model());
        return proxy ? qobject_cast<RegisterMapDataModel*>(proxy->sourceModel()) : nullptr;
    }

    static bool zeroBasedFromIndex(const QModelIndex& index)
    {
        auto* src = sourceModel(index);
        return src ? src->zeroBased() : false;
    }

    static bool hexViewFromIndex(const QModelIndex& index)
    {
        auto* src = sourceModel(index);
        return src ? src->hexView() : false;
    }

    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        if (hexViewFromIndex(index)) {
            bool ok;
            const quint16 v = option->text.toUShort(&ok);
            if (ok) option->text = formatUInt16Value(DataType::Hex, false, v);
        }
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
    {
        const bool zeroBased = zeroBasedFromIndex(index);
        auto* editor = new NumericLineEdit(parent);
        editor->setInputMode(NumericLineEdit::UInt32Mode);
        editor->setInputRange<quint32>(zeroBased ? 0 : 1, zeroBased ? 65535 : 65536);
        editor->setHexView(hexViewFromIndex(index));
        return editor;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto* le = qobject_cast<NumericLineEdit*>(editor);
        if (!le) return;
        const quint16 rawAddr = static_cast<quint16>(
            index.siblingAtColumn(ColUnit).data(RoleAddress).toUInt());
        const bool zeroBased = zeroBasedFromIndex(index);
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
                case DataType::Ansi:
                    editor->setInputMode(NumericLineEdit::AnsiMode);
                    editor->setInputRange<quint16>(0, 0xFFFF);
                    break;
                default: // Binary, Hex
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
                le->setValue<float>(index.data(Qt::EditRole).toFloat());
                break;
            case DataType::Float64:
                le->setValue<double>(index.data(Qt::EditRole).toDouble());
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
                text = QString::number(le->value<float>(), 'g', 8);
                break;
            case DataType::Float64:
                text = QString::number(le->value<double>(), 'g', QLocale::FloatingPointShortest);
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
                    case NumericLineEdit::AnsiMode:
                        newValue = le->value<quint16>();
                        text = QString::number(newValue);
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

    // Create model and filter proxy
    _model = new RegisterMapDataModel(_mbMultiServer, this);
    _displayDefinition.ZeroBasedAddress = AppPreferences::instance().dataViewDefinitions().ZeroBasedAddress;
    _model->setZeroBased(_displayDefinition.ZeroBasedAddress);
    _proxy = new RegisterMapFilterProxy(this);
    _proxy->setSourceModel(_model);
    ui->tableView->setModel(_proxy);

    // Header setup
    auto* hdr = ui->tableView->horizontalHeader();
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

    ui->tableView->verticalHeader()->setDefaultSectionSize(20);
    ui->tableView->verticalHeader()->hide();

    // Drag-and-drop row reordering
    ui->tableView->setDragEnabled(true);
    ui->tableView->setAcceptDrops(true);
    ui->tableView->setDragDropMode(QAbstractItemView::InternalMove);
    ui->tableView->setDragDropOverwriteMode(false);
    ui->tableView->setDropIndicatorShown(true);
    ui->tableView->setDefaultDropAction(Qt::MoveAction);

    // Item delegates (unchanged, work with QTableView via QModelIndex interface)
    ui->tableView->setItemDelegateForColumn(ColUnit,     new UnitItemDelegate(ui->tableView));
    ui->tableView->setItemDelegateForColumn(ColType,     new TypeItemDelegate(ui->tableView));
    ui->tableView->setItemDelegateForColumn(ColAddress,  new AddressItemDelegate(ui->tableView));
    ui->tableView->setItemDelegateForColumn(ColDataType, new DataTypeItemDelegate(ui->tableView));
    ui->tableView->setItemDelegateForColumn(ColOrder,    new OrderItemDelegate(ui->tableView));
    ui->tableView->setItemDelegateForColumn(ColValue,    new ValueItemDelegate(ui->tableView));

    setupToolBar();

    // Context menu on table
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, [this](const QPoint& pos) {
        const QModelIndex clickedIndex = ui->tableView->indexAt(pos);
        if (clickedIndex.isValid())
            ui->tableView->setCurrentIndex(clickedIndex);
        updateActionState();

        QMenu menu(this);
        menu.addAction(ui->actionAdd);
        menu.addAction(ui->actionInsert);
        menu.addAction(ui->actionDelete);
        menu.addSeparator();
        menu.addAction(ui->actionClear);
        menu.exec(ui->tableView->viewport()->mapToGlobal(pos));
    });

    // Selection and row count signals
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &FormRegisterMapView::updateActionState);
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &FormRegisterMapView::updateActionState);
    connect(_proxy, &QAbstractItemModel::rowsInserted,
            this, &FormRegisterMapView::updateActionState);
    connect(_proxy, &QAbstractItemModel::rowsRemoved,
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
    RegisterMapViewDefinitions dd = _displayDefinition;
    dd.FormName         = windowTitle();
    dd.ZeroBasedAddress = _model->zeroBased();
    dd.HexView          = ui->actionHexView->isChecked();
    return dd;
}

///
/// \brief FormRegisterMapView::setDisplayDefinition
///
void FormRegisterMapView::setDisplayDefinition(const RegisterMapViewDefinitions& dd)
{
    _displayDefinition = dd;
    if (!dd.FormName.isEmpty())
        setWindowTitle(dd.FormName);
    _model->setZeroBased(dd.ZeroBasedAddress);
    _model->setHexView(dd.HexView);
    if (_addrBaseCombo)
        _addrBaseCombo->setCurrentIndex(dd.ZeroBasedAddress ? 1 : 0);
    {
        QSignalBlocker b(ui->actionHexView);
        ui->actionHexView->setChecked(dd.HexView);
    }
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
    _model->applyMbDataChange(deviceId, data);
}

///
/// \brief FormRegisterMapView::addRowAndReturnSourceRow
///
int FormRegisterMapView::addRowAndReturnSourceRow(int referenceSourceRow)
{
    ItemMapKey key{ 1, QModbusDataUnit::HoldingRegisters, 0 };
    RegisterMapEntry entry;
    entry.type  = DataType::Int16;
    entry.order = RegisterOrder::MSRF;

    if (referenceSourceRow >= 0 && referenceSourceRow < _model->rowCount()) {
        const ItemMapKey referenceKey = _model->keyForRow(referenceSourceRow);
        const auto& referenceEntry = _model->entries()[referenceKey];
        key.DeviceId  = referenceKey.DeviceId;
        key.Type      = referenceKey.Type;
        key.Address   = referenceKey.Address < 0xFFFF ? referenceKey.Address + 1 : referenceKey.Address;
        entry.type    = referenceEntry.type;
        entry.order   = referenceEntry.order;
    } else if (!_model->isEmpty()) {
        const ItemMapKey last = _model->lastKey();
        const auto& lastEntry = _model->entries()[last];
        key.DeviceId  = last.DeviceId;
        key.Type      = last.Type;
        key.Address   = last.Address < 0xFFFF ? last.Address + 1 : last.Address;
        entry.type    = lastEntry.type;
        entry.order   = lastEntry.order;
    }

    // Skip duplicate keys
    while (_model->contains(key)) {
        if (key.Address < 0xFFFF) ++key.Address;
        else break;
    }

    entry.timestamp = _mbMultiServer.timestamp(key.DeviceId, key.Type, key.Address);
    const auto unit = _mbMultiServer.data(key.DeviceId, key.Type, key.Address, 1);
    entry.value = unit.isValid() ? static_cast<quint16>(unit.value(0)) : 0;

    const int sourceRow = _model->rowCount();
    _model->addEntry(key, entry);
    return sourceRow;
}

///
/// \brief FormRegisterMapView::editSourceRow
///
void FormRegisterMapView::editSourceRow(int sourceRow)
{
    if (sourceRow < 0 || sourceRow >= _model->rowCount()) return;

    const QModelIndex srcIdx   = _model->index(sourceRow, ColAddress);
    const QModelIndex proxyIdx = _proxy->mapFromSource(srcIdx);
    if (!proxyIdx.isValid()) return;

    ui->tableView->scrollTo(proxyIdx);
    ui->tableView->setCurrentIndex(proxyIdx);
    ui->tableView->edit(proxyIdx);
}

///
/// \brief FormRegisterMapView::on_actionAdd_triggered
///
void FormRegisterMapView::on_actionAdd_triggered()
{
    const int newSourceRow = addRowAndReturnSourceRow();
    editSourceRow(newSourceRow);
    updateActionState();
}

///
/// \brief FormRegisterMapView::on_actionInsert_triggered
///
void FormRegisterMapView::on_actionInsert_triggered()
{
    const QModelIndex currentProxyIdx = ui->tableView->currentIndex();
    if (!currentProxyIdx.isValid()) return;

    const QModelIndex currentSourceIdx = _proxy->mapToSource(currentProxyIdx.siblingAtColumn(ColUnit));
    if (!currentSourceIdx.isValid()) return;

    const int insertAfterSourceRow = currentSourceIdx.row();

    const int newSourceRow = addRowAndReturnSourceRow(insertAfterSourceRow);
    int targetSourceRow = newSourceRow;

    const int insertTargetRow = insertAfterSourceRow + 1;
    if (insertTargetRow >= 0 &&
        insertTargetRow < _model->rowCount() &&
        insertTargetRow != newSourceRow &&
        _model->moveRows({}, newSourceRow, 1, {}, insertTargetRow)) {
        targetSourceRow = insertTargetRow;
    }

    editSourceRow(targetSourceRow);
    updateActionState();
}

///
/// \brief FormRegisterMapView::on_actionDelete_triggered
///
void FormRegisterMapView::on_actionDelete_triggered()
{
    const auto selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QList<int> sourceRows;
    for (const auto& proxyIdx : selected)
        sourceRows.append(_proxy->mapToSource(proxyIdx).row());

    _model->removeEntries(sourceRows);
    updateActionState();
}

///
/// \brief FormRegisterMapView::on_actionClear_triggered
///
void FormRegisterMapView::on_actionClear_triggered()
{
    // Collect source rows of all currently visible (proxy) rows
    QList<int> sourceRows;
    for (int pr = 0; pr < _proxy->rowCount(); ++pr)
        sourceRows.append(_proxy->mapToSource(_proxy->index(pr, 0)).row());

    _model->removeEntries(sourceRows);
    updateActionState();
}

///
/// \brief FormRegisterMapView::on_actionHexView_toggled
///
void FormRegisterMapView::on_actionHexView_toggled(bool checked)
{
    _displayDefinition.HexView = checked;
    _model->setHexView(checked);
    emit definitionChanged();
}

///
/// \brief FormRegisterMapView::updateActionState
///
void FormRegisterMapView::updateActionState()
{
    const bool hasSelection = !ui->tableView->selectionModel()->selectedRows().isEmpty();
    ui->actionInsert->setEnabled(hasSelection);
    ui->actionDelete->setEnabled(hasSelection);
    ui->actionClear->setEnabled(_proxy->rowCount() > 0);
}

///
/// \brief FormRegisterMapView::processRequest
///
void FormRegisterMapView::processRequest(quint8 deviceId, QModbusDataUnit::RegisterType type,
                                         quint16 startAddress, quint16 count)
{
    if (count == 0 || count > 2000) return;

    const QModbusDataUnit unit = _mbMultiServer.data(deviceId, type, startAddress, count);

    for (quint16 i = 0; i < count; ++i) {
        const ItemMapKey key{ deviceId, type, static_cast<quint16>(startAddress + i) };
        const quint16 value = unit.isValid() ? static_cast<quint16>(unit.value(i)) : 0;

        if (_model->contains(key)) {
            const QModbusDataUnit singleUnit(type, startAddress + i, QVector<quint16>{value});
            _model->applyMbDataChange(deviceId, singleUnit);
        } else if (_autoAddOnRequest) {
            RegisterMapEntry entry;
            entry.value = value;
            entry.type  = (type == QModbusDataUnit::Coils ||
                           type == QModbusDataUnit::DiscreteInputs)
                          ? DataType::Binary : DataType::Int16;
            entry.order     = RegisterOrder::MSRF;
            entry.timestamp = QDateTime::currentDateTime();
            _model->addEntry(key, entry);
        }
    }
}

///
/// \brief FormRegisterMapView::setupToolBar
///
void FormRegisterMapView::setupToolBar()
{
    // Expanding spacer before filter area
    auto* spacerWidget = new QWidget(ui->toolBar);
    spacerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* spacerAction = ui->toolBar->insertWidget(ui->actionClear, spacerWidget);

    // Separator between Delete and address base widget
    ui->toolBar->insertSeparator(spacerAction);

    // Address base combobox wrapped in a labeled layout widget
    _addrBaseCombo = new QComboBox(ui->toolBar);
    _addrBaseCombo->addItem(tr("1-based")); // index 0
    _addrBaseCombo->addItem(tr("0-based")); // index 1
    _addrBaseCombo->setCurrentIndex(_model->zeroBased() ? 1 : 0);
    _addrBaseCombo->setFixedWidth(80);

    auto* addrBaseWidget = new QWidget(ui->toolBar);
    auto* addrBaseLayout = new QHBoxLayout(addrBaseWidget);
    addrBaseLayout->setContentsMargins(9, 0, 9, 0);
    addrBaseLayout->setSpacing(6);
    addrBaseLayout->addWidget(new QLabel(tr("Address Base:"), addrBaseWidget));
    addrBaseLayout->addWidget(_addrBaseCombo);
    ui->toolBar->insertWidget(spacerAction, addrBaseWidget);

    // Separator between address base widget and Hex View action
    ui->toolBar->insertSeparator(spacerAction);
    ui->toolBar->insertAction(spacerAction, ui->actionHexView);

    // Filter widgets
    _filterUnitSpin = new QSpinBox;
    _filterUnitSpin->setRange(0, 255);
    _filterUnitSpin->setSpecialValueText(tr("All"));
    _filterUnitSpin->setFixedWidth(60);

    _filterTypeCombo = new QComboBox;
    _filterTypeCombo->addItem(tr("All Types"));
    _filterTypeCombo->addItem(tr("Coils"));
    _filterTypeCombo->addItem(tr("Discrete Inputs"));
    _filterTypeCombo->addItem(tr("Input Registers"));
    _filterTypeCombo->addItem(tr("Holding Registers"));
    _filterTypeCombo->setFixedWidth(130);

    auto* filterWidget = new QWidget(ui->toolBar);
    auto* filterLayout = new QHBoxLayout(filterWidget);
    filterLayout->setContentsMargins(9, 0, 9, 0);
    filterLayout->setSpacing(6);
    filterLayout->addWidget(new QLabel(tr("Unit:"), filterWidget));
    filterLayout->addWidget(_filterUnitSpin);
    filterLayout->addWidget(new QLabel(tr("Data Type:"), filterWidget));
    filterLayout->addWidget(_filterTypeCombo);

    ui->toolBar->insertWidget(ui->actionClear, filterWidget);
    ui->toolBar->insertSeparator(ui->actionClear);

    connect(_addrBaseCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
        const bool zeroBased = (idx == 1);
        _displayDefinition.ZeroBasedAddress = zeroBased;
        _model->setZeroBased(zeroBased);
        emit definitionChanged();
    });
    connect(_filterTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int idx) { _proxy->setFilterTypeIndex(idx); updateActionState(); });
    connect(_filterUnitSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int val) { _proxy->setFilterUnit(val); updateActionState(); });
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
/// \brief FormRegisterMapView::columnWidths
///
QList<int> FormRegisterMapView::columnWidths() const
{
    const auto* hdr = ui->tableView->horizontalHeader();
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
    auto* hdr = ui->tableView->horizontalHeader();
    for (int i = 0; i < widths.size() && i < hdr->count(); ++i)
        if (widths[i] > 0) hdr->resizeSection(i, widths[i]);
}
