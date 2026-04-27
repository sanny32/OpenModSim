#include <QtMath>
#include <QRandomGenerator>
#include <QComboBox>
#include <QSignalBlocker>
#include "modbuslimits.h"
#include "modbusmultiserver.h"
#include "uiutils.h"
#include "formatutils.h"
#include "numericutils.h"
#include "numericlineedit.h"
#include "dialogforcemultipleregisters.h"
#include "ui_dialogforcemultipleregisters.h"

namespace {
quint16 parseBinary16(const QString& text, quint16 fallback = 0)
{
    bool ok = false;
    const auto value = text.trimmed().toUShort(&ok, 2);
    return ok ? value : fallback;
}

AddressBase addressBase(const ModbusWriteParams& params)
{
    return params.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1;
}
}

///
/// \brief DialogForceMultipleRegisters::DialogForceMultipleRegisters
/// \param params
/// \param type
/// \param length
/// \param dd
/// \param parent
///
DialogForceMultipleRegisters::DialogForceMultipleRegisters(ModbusWriteParams& params, QModbusDataUnit::RegisterType type, int length, bool displayHexAddresses, QWidget *parent) :
      QAdjustedSizeDialog(parent)
    , ui(new Ui::DialogForceMultipleRegisters)
    ,_writeParams(params)
    ,_type(type)
    ,_hexAddress(displayHexAddresses)
{
    ui->setupUi(this);

    ui->lineEditStep->setValue(1);
    recolorPushButtonIcon(ui->pushButtonExport, Qt::red);
    recolorPushButtonIcon(ui->pushButtonImport, Qt::darkGreen);

    switch(type)
    {
        case QModbusDataUnit::InputRegisters:
            setWindowTitle(tr("PRESET INPUT REGISTERS"));
        break;
        case QModbusDataUnit::HoldingRegisters:
            setWindowTitle(tr("PRESET HOLDING REGISTERS"));
        break;
        default:
            break;
    }

    _data = params.Value.value<QVector<quint16>>();
    if(_data.length() != length) _data.resize(length);
    if(_writeParams.DataMode == DataType::Ansi) _writeParams.DataMode = DataType::Hex;

    setupAddressControls(length);
    setupDisplayControls();
    setupPresetData();
    setupEditorInputs();

    updateAddressSummary();
    updateTableWidget();
}

///
/// \brief DialogForceMultipleRegisters::~DialogForceMultipleRegisters
///
DialogForceMultipleRegisters::~DialogForceMultipleRegisters()
{
    delete ui;
}

///
/// rief DialogForceMultipleRegisters::changeEvent
///
///
/// \brief DialogForceMultipleRegisters::changeEvent
///
void DialogForceMultipleRegisters::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        updateAddressSummary();
    }

    QDialog::changeEvent(event);
}

///
/// \brief DialogForceMultipleRegisters::setupAddressControls
///
void DialogForceMultipleRegisters::setupAddressControls(int length)
{
    ui->lineEditDeviceId->setLeadingZeroes(_writeParams.LeadingZeros);
    ui->lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditDeviceId->setValue(_writeParams.DeviceId);
    ui->lineEditDeviceId->setHexView(_hexAddress);
    ui->lineEditDeviceId->setHexButtonVisible(false);

    ui->lineEditAddress->setLeadingZeroes(_writeParams.LeadingZeros);
    ui->lineEditAddress->setInputMode(_hexAddress ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(_writeParams.AddrSpace, _writeParams.ZeroBasedAddress));
    ui->lineEditAddress->setValue(_writeParams.Address);
    ui->lineEditAddress->setHexView(_hexAddress);
    ui->lineEditAddress->setHexButtonVisible(false);

    const auto initialLenRange = ModbusLimits::lengthRange(_writeParams.Address, _writeParams.ZeroBasedAddress, _writeParams.AddrSpace);
    const int initialLength = qBound(initialLenRange.from(), length, initialLenRange.to());
    if (_data.size() != initialLength)
        _data.resize(initialLength);

    ui->lineEditLength->setLeadingZeroes(_writeParams.LeadingZeros);
    ui->lineEditLength->setInputMode(_hexAddress ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditLength->setInputRange(initialLenRange);
    ui->lineEditLength->setValue(initialLength);
    ui->lineEditLength->setHexView(_hexAddress);
    ui->lineEditLength->setHexButtonVisible(false);

    connect(ui->lineEditDeviceId,
            static_cast<void (NumericLineEdit::*)(const QVariant&)>(&NumericLineEdit::valueChanged),
            this,
            [this](const QVariant&) {
        _writeParams.DeviceId = ui->lineEditDeviceId->value<quint32>();
        reloadDataFromServer();
        updateTableWidget();
    });

    connect(ui->lineEditAddress,
            static_cast<void (NumericLineEdit::*)(const QVariant&)>(&NumericLineEdit::valueChanged),
            this,
            [this](const QVariant&) {
        const int address = ui->lineEditAddress->value<int>();
        _writeParams.Address = static_cast<quint16>(address);

        const auto lenRange = ModbusLimits::lengthRange(address, _writeParams.ZeroBasedAddress, _writeParams.AddrSpace);
        QSignalBlocker lengthBlocker(ui->lineEditLength);
        ui->lineEditLength->setInputRange(lenRange);
        if(ui->lineEditLength->value<int>() > lenRange.to()) {
            ui->lineEditLength->setValue(lenRange.to());
            ui->lineEditLength->update();
        }

        reloadDataFromServer();
        updateAddressSummary();
        updateTableWidget();
    });

    connect(ui->lineEditLength,
            static_cast<void (NumericLineEdit::*)(const QVariant&)>(&NumericLineEdit::valueChanged),
            this,
            [this](const QVariant&) {
        reloadDataFromServer();
        updateAddressSummary();
        updateTableWidget();
    });
}

///
/// \brief DialogForceMultipleRegisters::setupDisplayControls
///
void DialogForceMultipleRegisters::setupDisplayControls()
{
    ui->comboBoxDisplayFormat->clear();
    auto addFormat = [this](DataType type, const QString& text)
    {
        ui->comboBoxDisplayFormat->addItem(text, static_cast<int>(type));
    };

    addFormat(DataType::Binary,  QStringLiteral("Binary"));
    addFormat(DataType::Hex,     QStringLiteral("Hex"));
    addFormat(DataType::Int16,   QStringLiteral("Int16"));
    addFormat(DataType::UInt16,  QStringLiteral("UInt16"));
    addFormat(DataType::Int32,   QStringLiteral("Int32"));
    addFormat(DataType::UInt32,  QStringLiteral("UInt32"));
    addFormat(DataType::Int64,   QStringLiteral("Int64"));
    addFormat(DataType::UInt64,  QStringLiteral("UInt64"));
    addFormat(DataType::Float32, QStringLiteral("Float32"));
    addFormat(DataType::Float64, QStringLiteral("Float64"));

    ui->comboBoxRegisterOrder->setItemData(0, static_cast<int>(RegisterOrder::MSRF));
    ui->comboBoxRegisterOrder->setItemData(1, static_cast<int>(RegisterOrder::LSRF));
    ui->comboBoxByteOrder->setItemData(0, static_cast<int>(ByteOrder::Direct));
    ui->comboBoxByteOrder->setItemData(1, static_cast<int>(ByteOrder::Swapped));

    auto applyDisplayMode = [this](DataType type, RegisterOrder order)
    {
        _writeParams.DataMode = type;
        _writeParams.RegOrder = isMultiRegisterType(type) ? order : RegisterOrder::MSRF;
        setupEditorInputs();
        updateTableWidget();
        updateDisplayBar();
    };

    connect(ui->comboBoxDisplayFormat, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, applyDisplayMode](int index) {
        if (index < 0)
            return;
        const auto type = static_cast<DataType>(ui->comboBoxDisplayFormat->itemData(index).toInt());
        const auto order = static_cast<RegisterOrder>(ui->comboBoxRegisterOrder->currentData().toInt());
        applyDisplayMode(type, order);
    });

    connect(ui->comboBoxRegisterOrder, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, applyDisplayMode](int index) {
        if (index >= 0 && isMultiRegisterType(_writeParams.DataMode))
            applyDisplayMode(_writeParams.DataMode, static_cast<RegisterOrder>(ui->comboBoxRegisterOrder->itemData(index).toInt()));
    });

    connect(ui->comboBoxByteOrder, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index < 0)
            return;
        _writeParams.Order = static_cast<ByteOrder>(ui->comboBoxByteOrder->itemData(index).toInt());
        setupEditorInputs();
        updateTableWidget();
        updateDisplayBar();
    });

    updateDisplayBar();
}

///
/// \brief DialogForceMultipleRegisters::updateDisplayBar
///
void DialogForceMultipleRegisters::updateDisplayBar()
{
    QSignalBlocker formatBlocker(ui->comboBoxDisplayFormat);
    const int formatIndex = ui->comboBoxDisplayFormat->findData(static_cast<int>(_writeParams.DataMode));
    if (formatIndex >= 0)
        ui->comboBoxDisplayFormat->setCurrentIndex(formatIndex);

    QSignalBlocker orderBlocker(ui->comboBoxRegisterOrder);
    const int orderIndex = ui->comboBoxRegisterOrder->findData(static_cast<int>(_writeParams.RegOrder));
    if (orderIndex >= 0)
        ui->comboBoxRegisterOrder->setCurrentIndex(orderIndex);
    ui->comboBoxRegisterOrder->setEnabled(isMultiRegisterType(_writeParams.DataMode));

    QSignalBlocker byteOrderBlocker(ui->comboBoxByteOrder);
    const int byteOrderIndex = ui->comboBoxByteOrder->findData(static_cast<int>(_writeParams.Order));
    if (byteOrderIndex >= 0)
        ui->comboBoxByteOrder->setCurrentIndex(byteOrderIndex);
}

///
/// \brief DialogForceMultipleRegisters::updateAddressSummary
///
void DialogForceMultipleRegisters::updateAddressSummary()
{
    const int length = qMax(1, _data.size());
    ui->labelAddresses->setText(
        QString("<html><head/><body><p>%3<span style=\" font-weight:700;\">%1 </span>→ %4<span style=\" font-weight:700;\">%2</span></p></body></html>").arg(
        formatAddress(_type, _writeParams.Address, _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams)),
        formatAddress(_type, _writeParams.Address + length - 1, _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams)),
        tr("Starting Address: "), tr("Ending Address: ")));
}

///
/// \brief DialogForceMultipleRegisters::reloadDataFromServer
///
void DialogForceMultipleRegisters::reloadDataFromServer()
{
    const int length = ui->lineEditLength->value<int>();
    if(length <= 0) {
        _data.clear();
        return;
    }

    if(_writeParams.Server == nullptr) {
        _data.resize(length);
        return;
    }

    const int serverAddress = _writeParams.Address - (_writeParams.ZeroBasedAddress ? 0 : 1);
    const auto data = _writeParams.Server->data(
        static_cast<quint8>(_writeParams.DeviceId),
        _type,
        static_cast<quint16>(serverAddress),
        static_cast<quint16>(length));

    _data = data.values();
    if(_data.size() != length)
        _data.resize(length);
}

///
/// \brief DialogForceMultipleRegisters::setupPresetData
///
void DialogForceMultipleRegisters::setupPresetData()
{
    ui->comboBoxPreset->clear();

    auto addOperation = [this](PresetOperations type, const QString& text)
    {
        ui->comboBoxPreset->addItem(text, static_cast<int>(type));
    };

    addOperation(PresetOperations::Constant, tr("Constant"));
    addOperation(PresetOperations::Random, tr("Random"));
    addOperation(PresetOperations::Increment, tr("Increment"));
    addOperation(PresetOperations::Zero, tr("Zero"));

    connect(ui->comboBoxPreset, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0)
            updatePresetControls();
    });

    ui->comboBoxPreset->setCurrentIndex(0);
    updatePresetControls();
}

///
/// \brief DialogForceMultipleRegisters::updatePresetControls
///
void DialogForceMultipleRegisters::updatePresetControls()
{
    const auto op = static_cast<PresetOperations>(ui->comboBoxPreset->currentData().toInt());
    const bool increment = op == PresetOperations::Increment;
    const bool random = op == PresetOperations::Random;
    const bool zero = op == PresetOperations::Zero;

    ui->lineEditValue->setVisible(!increment);
    ui->lineEditValue->setEnabled(!random);
    ui->lineEditValue->setReadOnly(zero);

    ui->horizontalLayoutPresetApplyRight->removeWidget(ui->pushButtonValue);
    ui->horizontalLayoutPresetApply->removeWidget(ui->pushButtonValue);
    if (increment)
        ui->horizontalLayoutPresetApply->addWidget(ui->pushButtonValue);
    else
        ui->horizontalLayoutPresetApplyRight->addWidget(ui->pushButtonValue);
    ui->widgetPresetApplyRight->setVisible(!increment);
    ui->widgetPresetApplyBottom->setVisible(increment);

    ui->labelStart->setVisible(increment);
    ui->lineEditStartValue->setVisible(increment);
    ui->labelStep->setVisible(increment);
    ui->lineEditStep->setVisible(increment);

    if (zero)
        ui->lineEditValue->setValue(0);
}

///
/// \brief DialogForceMultipleRegisters::setupEditorInputs
///
void DialogForceMultipleRegisters::setupEditorInputs()
{
    ui->lineEditValue->setValidator(nullptr);
    ui->lineEditStartValue->setValidator(nullptr);
    ui->lineEditStep->setValidator(nullptr);

    const bool lsrf = _writeParams.RegOrder == RegisterOrder::LSRF;
    const quint16 firstReg = _data.isEmpty() ? 0 : _data[0];
    const quint16 secondReg = _data.size() > 1 ? _data[1] : 0;
    const quint16 thirdReg = _data.size() > 2 ? _data[2] : 0;
    const quint16 fourthReg = _data.size() > 3 ? _data[3] : 0;

    switch(_writeParams.DataMode)
    {
        case DataType::Binary:
            setupLineEdit<quint16>(ui->lineEditValue, NumericLineEdit::BinaryMode, true);
            setupLineEdit<quint16>(ui->lineEditStartValue, NumericLineEdit::BinaryMode, true);
            setupLineEdit<quint16>(ui->lineEditStep, NumericLineEdit::BinaryMode, true);
            ui->lineEditStartValue->setValue(toByteOrderValue(firstReg, _writeParams.Order));
        break;

        case DataType::Hex:
        case DataType::Ansi:
            setupLineEdit<quint16>(ui->lineEditValue, NumericLineEdit::HexMode, true);
            setupLineEdit<quint16>(ui->lineEditStartValue, NumericLineEdit::HexMode, true);
            setupLineEdit<quint16>(ui->lineEditStep, NumericLineEdit::HexMode, true);
            ui->lineEditStartValue->setValue(toByteOrderValue(firstReg, _writeParams.Order));
        break;

        case DataType::Int16:
            setupLineEdit<qint16>(ui->lineEditValue, NumericLineEdit::Int32Mode);
            setupLineEdit<qint16>(ui->lineEditStartValue, NumericLineEdit::Int32Mode);
            setupLineEdit<qint16>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(toByteOrderValue(firstReg, _writeParams.Order));
        break;

        case DataType::UInt16:
            setupLineEdit<quint16>(ui->lineEditValue, NumericLineEdit::UInt32Mode);
            setupLineEdit<quint16>(ui->lineEditStartValue, NumericLineEdit::UInt32Mode);
            setupLineEdit<qint16>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(toByteOrderValue(firstReg, _writeParams.Order));
        break;

        case DataType::Int32:
            setupLineEdit<qint32>(ui->lineEditValue, NumericLineEdit::Int32Mode);
            setupLineEdit<qint32>(ui->lineEditStartValue, NumericLineEdit::Int32Mode);
            setupLineEdit<qint32>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(lsrf ? makeInt32(secondReg, firstReg, _writeParams.Order)
                                                  : makeInt32(firstReg, secondReg, _writeParams.Order));
        break;

        case DataType::UInt32:
            setupLineEdit<quint32>(ui->lineEditValue, NumericLineEdit::UInt32Mode);
            setupLineEdit<quint32>(ui->lineEditStartValue, NumericLineEdit::UInt32Mode);
            setupLineEdit<qint32>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(lsrf ? makeUInt32(secondReg, firstReg, _writeParams.Order)
                                                  : makeUInt32(firstReg, secondReg, _writeParams.Order));
        break;

        case DataType::Int64:
            setupLineEdit<qint64>(ui->lineEditValue, NumericLineEdit::Int64Mode);
            setupLineEdit<qint64>(ui->lineEditStartValue, NumericLineEdit::Int64Mode);
            setupLineEdit<qint64>(ui->lineEditStep, NumericLineEdit::Int64Mode);
            ui->lineEditStartValue->setValue(lsrf ? makeInt64(fourthReg, thirdReg, secondReg, firstReg, _writeParams.Order)
                                                  : makeInt64(firstReg, secondReg, thirdReg, fourthReg, _writeParams.Order));
        break;

        case DataType::UInt64:
            setupLineEdit<quint64>(ui->lineEditValue, NumericLineEdit::UInt64Mode);
            setupLineEdit<quint64>(ui->lineEditStartValue, NumericLineEdit::UInt64Mode);
            setupLineEdit<qint64>(ui->lineEditStep, NumericLineEdit::Int64Mode);
            ui->lineEditStartValue->setValue(lsrf ? makeUInt64(fourthReg, thirdReg, secondReg, firstReg, _writeParams.Order)
                                                  : makeUInt64(firstReg, secondReg, thirdReg, fourthReg, _writeParams.Order));
        break;

        case DataType::Float32:
            setupLineEdit<float>(ui->lineEditValue, NumericLineEdit::FloatMode);
            setupLineEdit<float>(ui->lineEditStartValue, NumericLineEdit::FloatMode);
            setupLineEdit<float>(ui->lineEditStep, NumericLineEdit::FloatMode);
            ui->lineEditStartValue->setValue(lsrf ? makeFloat(secondReg, firstReg, _writeParams.Order)
                                                  : makeFloat(firstReg, secondReg, _writeParams.Order));
        break;

        case DataType::Float64:
            setupLineEdit<double>(ui->lineEditValue, NumericLineEdit::DoubleMode);
            setupLineEdit<double>(ui->lineEditStartValue, NumericLineEdit::DoubleMode);
            setupLineEdit<double>(ui->lineEditStep, NumericLineEdit::DoubleMode);
            ui->lineEditStartValue->setValue(lsrf ? makeDouble(fourthReg, thirdReg, secondReg, firstReg, _writeParams.Order)
                                                  : makeDouble(firstReg, secondReg, thirdReg, fourthReg, _writeParams.Order));
        break;
    }
}

///
/// \brief DialogForceMultipleRegisters::applyValue
/// \param value
/// \param index
/// \param op
///
template<>
void DialogForceMultipleRegisters::applyValue<qint32>(qint32 value, int index, ValueOperation op)
{
    if(_writeParams.DataMode != DataType::Int32) return;

    const bool lsrf = _writeParams.RegOrder == RegisterOrder::LSRF;
    qint32 cur = lsrf ? makeInt32(_data[index + 1], _data[index], _writeParams.Order)
                      : makeInt32(_data[index], _data[index + 1], _writeParams.Order);

    switch(op)
    {
        case ValueOperation::Set:       cur = value; break;
        case ValueOperation::Add:       cur += value; break;
        case ValueOperation::Subtract:  cur -= value; break;
        case ValueOperation::Multiply:  cur *= value; break;
        case ValueOperation::Divide:    cur /= value; break;
    }

    if(lsrf) breakInt32(cur, _data[index + 1], _data[index], _writeParams.Order);
    else     breakInt32(cur, _data[index], _data[index + 1], _writeParams.Order);
}

///
/// \brief DialogForceMultipleRegisters::applyValue
/// \param value
/// \param index
/// \param op
///
template<>
void DialogForceMultipleRegisters::applyValue<quint32>(quint32 value, int index, ValueOperation op)
{
    if(_writeParams.DataMode != DataType::UInt32) return;

    const bool lsrf = _writeParams.RegOrder == RegisterOrder::LSRF;
    quint32 cur = lsrf ? makeUInt32(_data[index + 1], _data[index], _writeParams.Order)
                       : makeUInt32(_data[index], _data[index + 1], _writeParams.Order);

    switch(op)
    {
        case ValueOperation::Set:       cur = value; break;
        case ValueOperation::Add:       cur += value; break;
        case ValueOperation::Subtract:  cur -= value; break;
        case ValueOperation::Multiply:  cur *= value; break;
        case ValueOperation::Divide:    cur /= value; break;
    }

    if(lsrf) breakUInt32(cur, _data[index + 1], _data[index], _writeParams.Order);
    else     breakUInt32(cur, _data[index], _data[index + 1], _writeParams.Order);
}

///
/// \brief DialogForceMultipleRegisters::applyValue
/// \param value
/// \param index
/// \param op
///
template<>
void DialogForceMultipleRegisters::applyValue<float>(float value, int index, ValueOperation op)
{
    if(_writeParams.DataMode != DataType::Float32) return;

    const bool lsrf = _writeParams.RegOrder == RegisterOrder::LSRF;
    float cur = lsrf ? makeFloat(_data[index + 1], _data[index], _writeParams.Order)
                     : makeFloat(_data[index], _data[index + 1], _writeParams.Order);

    switch(op)
    {
        case ValueOperation::Set:       cur = value; break;
        case ValueOperation::Add:       cur += value; break;
        case ValueOperation::Subtract:  cur -= value; break;
        case ValueOperation::Multiply:  cur *= value; break;
        case ValueOperation::Divide:    cur /= value; break;
    }

    if(lsrf) breakFloat(cur, _data[index + 1], _data[index], _writeParams.Order);
    else     breakFloat(cur, _data[index], _data[index + 1], _writeParams.Order);
}

///
/// \brief DialogForceMultipleRegisters::applyValue
/// \param value
/// \param index
/// \param op
///
template<>
void DialogForceMultipleRegisters::applyValue<qint64>(qint64 value, int index, ValueOperation op)
{
    if(_writeParams.DataMode != DataType::Int64) return;

    const bool lsrf = _writeParams.RegOrder == RegisterOrder::LSRF;
    qint64 cur = lsrf ? makeInt64(_data[index + 3], _data[index + 2], _data[index + 1], _data[index], _writeParams.Order)
                      : makeInt64(_data[index], _data[index + 1], _data[index + 2], _data[index + 3], _writeParams.Order);

    switch(op)
    {
        case ValueOperation::Set:       cur = value; break;
        case ValueOperation::Add:       cur += value; break;
        case ValueOperation::Subtract:  cur -= value; break;
        case ValueOperation::Multiply:  cur *= value; break;
        case ValueOperation::Divide:    cur /= value; break;
    }

    if(lsrf) breakInt64(cur, _data[index + 3], _data[index + 2], _data[index + 1], _data[index], _writeParams.Order);
    else     breakInt64(cur, _data[index], _data[index + 1], _data[index + 2], _data[index + 3], _writeParams.Order);
}

///
/// \brief DialogForceMultipleRegisters::applyValue
/// \param value
/// \param index
/// \param op
///
template<>
void DialogForceMultipleRegisters::applyValue<quint64>(quint64 value, int index, ValueOperation op)
{
    if(_writeParams.DataMode != DataType::UInt64) return;

    const bool lsrf = _writeParams.RegOrder == RegisterOrder::LSRF;
    quint64 cur = lsrf ? makeUInt64(_data[index + 3], _data[index + 2], _data[index + 1], _data[index], _writeParams.Order)
                       : makeUInt64(_data[index], _data[index + 1], _data[index + 2], _data[index + 3], _writeParams.Order);

    switch(op)
    {
        case ValueOperation::Set:       cur = value; break;
        case ValueOperation::Add:       cur += value; break;
        case ValueOperation::Subtract:  cur -= value; break;
        case ValueOperation::Multiply:  cur *= value; break;
        case ValueOperation::Divide:    cur /= value; break;
    }

    if(lsrf) breakUInt64(cur, _data[index + 3], _data[index + 2], _data[index + 1], _data[index], _writeParams.Order);
    else     breakUInt64(cur, _data[index], _data[index + 1], _data[index + 2], _data[index + 3], _writeParams.Order);
}

///
/// \brief DialogForceMultipleRegisters::applyValue
/// \param value
/// \param index
/// \param op
///
template<>
void DialogForceMultipleRegisters::applyValue<double>(double value, int index, ValueOperation op)
{
    if(_writeParams.DataMode != DataType::Float64) return;

    const bool lsrf = _writeParams.RegOrder == RegisterOrder::LSRF;
    double cur = lsrf ? makeDouble(_data[index + 3], _data[index + 2], _data[index + 1], _data[index], _writeParams.Order)
                      : makeDouble(_data[index], _data[index + 1], _data[index + 2], _data[index + 3], _writeParams.Order);

    switch(op)
    {
        case ValueOperation::Set:       cur = value; break;
        case ValueOperation::Add:       cur += value; break;
        case ValueOperation::Subtract:  cur -= value; break;
        case ValueOperation::Multiply:  cur *= value; break;
        case ValueOperation::Divide:    cur /= value; break;
    }

    if(lsrf) breakDouble(cur, _data[index + 3], _data[index + 2], _data[index + 1], _data[index], _writeParams.Order);
    else     breakDouble(cur, _data[index], _data[index + 1], _data[index + 2], _data[index + 3], _writeParams.Order);
}

///
/// \brief DialogForceMultipleRegisters::applyToAll
/// \param op
/// \param value
///
void DialogForceMultipleRegisters::applyToAll(ValueOperation op, double value)
{
    for(int i = 0; i < _data.size(); i++)
    {
        switch(_writeParams.DataMode)
        {
            case DataType::Hex:
            case DataType::Binary:
            case DataType::UInt16:
            case DataType::Ansi:
                applyValue<quint16>(static_cast<quint16>(value), i, op);
            break;

            case DataType::Int16:
                applyValue<qint16>(static_cast<qint16>(value), i, op);
            break;

            case DataType::Int32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    applyValue<qint32>(static_cast<qint32>(value), i, op);
            break;

            case DataType::UInt32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    applyValue<quint32>(static_cast<quint32>(value), i, op);
            break;

            case DataType::Int64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    applyValue<qint64>(static_cast<qint64>(value), i, op);
            break;

            case DataType::UInt64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    applyValue<quint64>(static_cast<quint64>(value), i, op);
            break;

            case DataType::Float32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    applyValue<float>(static_cast<float>(value), i, op);
            break;

            case DataType::Float64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    applyValue<double>(static_cast<double>(value), i, op);
            break;
        }
    }

    updateTableWidget();
}


///
/// \brief DialogForceMultipleRegisters::accept
///
void DialogForceMultipleRegisters::accept()
{
    _writeParams.Value = QVariant::fromValue(_data);
    _writeParams.DeviceId = ui->lineEditDeviceId->value<quint32>();
    _writeParams.Address = ui->lineEditAddress->value<quint16>();
    QDialog::accept();
}

///
/// \brief DialogForceMultipleRegisters::on_pushButton0_clicked
///
void DialogForceMultipleRegisters::on_pushButton0_clicked()
{
   applyToAll(ValueOperation::Set, 0);
}

///
/// \brief DialogForceMultipleRegisters::on_pushButtonRandom_clicked
///
void DialogForceMultipleRegisters::on_pushButtonRandom_clicked()
{
    for(int i = 0; i < _data.size(); i++)
    {
        switch(_writeParams.DataMode)
        {
            case DataType::Binary:
            case DataType::Hex:
            case DataType::UInt16:
            case DataType::Ansi:
                applyValue<quint16>(QRandomGenerator::global()->bounded(0, USHRT_MAX), i, ValueOperation::Set);
                break;

            case DataType::Int16:
                applyValue<qint16>(QRandomGenerator::global()->bounded(SHRT_MIN, USHRT_MAX), i, ValueOperation::Set);
                break;

            case DataType::Int32:
                applyValue<qint32>(QRandomGenerator::global()->bounded(INT_MIN, INT_MAX), i, ValueOperation::Set);
                break;

            case DataType::UInt32:
                applyValue<quint32>(QRandomGenerator::global()->bounded(0U, UINT_MAX), i, ValueOperation::Set);
                break;

            case DataType::Float32:
                applyValue<float>(QRandomGenerator::global()->bounded(100), i, ValueOperation::Set);
                break;

            case DataType::Float64:
                applyValue<double>(QRandomGenerator::global()->bounded(100), i, ValueOperation::Set);
                break;

            case DataType::Int64:
                applyValue<qint64>(QRandomGenerator::global()->generate64(), i, ValueOperation::Set);
                break;

            case DataType::UInt64:
                applyValue<quint64>(QRandomGenerator::global()->generate64(), i, ValueOperation::Set);
                break;
        }
    }

    updateTableWidget();
}

///
/// \brief DialogForceMultipleRegisters::on_pushButtonValue_clicked
///
void DialogForceMultipleRegisters::on_pushButtonValue_clicked()
{
    const auto op = static_cast<PresetOperations>(ui->comboBoxPreset->currentData().toInt());
    switch (op) {
    case PresetOperations::Constant:
        if (_writeParams.DataMode == DataType::Binary) {
            applyToAll(ValueOperation::Set, parseBinary16(ui->lineEditValue->text()));
            return;
        }

        applyToAll(ValueOperation::Set, ui->lineEditValue->value<double>());
        return;

    case PresetOperations::Random:
        on_pushButtonRandom_clicked();
        return;

    case PresetOperations::Increment:
        on_pushButtonInc_clicked();
        return;

    case PresetOperations::Zero:
        on_pushButton0_clicked();
        return;
    }
}

///
/// \brief DialogForceMultipleRegisters::on_pushButtonInc_clicked
///
void DialogForceMultipleRegisters::on_pushButtonInc_clicked()
{
    for(int i = 0; i < _data.size(); i++)
    {
        switch(_writeParams.DataMode)
        {
            case DataType::Binary:
            {
                const quint16 start = parseBinary16(ui->lineEditStartValue->text(), 0);
                const quint16 step = parseBinary16(ui->lineEditStep->text(), 1);
                applyValue<quint16>(start + i * step, i, ValueOperation::Set);
            }
            break;

            case DataType::Hex:
            case DataType::UInt16:
            case DataType::Ansi:
                applyValue<quint16>(ui->lineEditStartValue->value<quint16>() + i * ui->lineEditStep->value<qint16>(), i, ValueOperation::Set);
            break;

            case DataType::Int16:
                applyValue<qint16>(ui->lineEditStartValue->value<qint16>() + i * ui->lineEditStep->value<qint16>(), i, ValueOperation::Set);
            break;

            case DataType::Int32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    applyValue<qint32>(ui->lineEditStartValue->value<qint32>() + (i / 2) * ui->lineEditStep->value<qint32>(), i, ValueOperation::Set);
            break;

            case DataType::UInt32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    applyValue<quint32>(ui->lineEditStartValue->value<quint32>() + (i / 2) * ui->lineEditStep->value<qint32>(), i, ValueOperation::Set);
            break;

            case DataType::Int64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    applyValue<qint64>(ui->lineEditStartValue->value<qint64>() + (i / 4) * ui->lineEditStep->value<qint64>(), i, ValueOperation::Set);
            break;

            case DataType::UInt64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    applyValue<quint64>(ui->lineEditStartValue->value<quint64>() + (i / 4) * ui->lineEditStep->value<qint64>(), i, ValueOperation::Set);
            break;

            case DataType::Float32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    applyValue<float>(ui->lineEditStartValue->value<float>() + (i / 2) * ui->lineEditStep->value<float>(), i, ValueOperation::Set);
            break;

            case DataType::Float64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    applyValue<double>(ui->lineEditStartValue->value<double>() + (i / 4) * ui->lineEditStep->value<double>(), i, ValueOperation::Set);
            break;
        }
    }

    updateTableWidget();
}

///
/// \brief DialogForceMultipleRegisters::on_pushButtonImport_clicked
///
void DialogForceMultipleRegisters::on_pushButtonImport_clicked()
{
    auto filename = QFileDialog::getOpenFileName(this, QString(), QString(), tr("CSV files (*.csv)"));
    if(filename.isEmpty())
        return;

    QFile file(filename);
    if(!file.open(QFile::ReadOnly))
    {
        QMessageBox::critical(this, tr("Error"), file.errorString());
        return;
    }

    QTextStream ts(&file);

    QVector<quint16> newData;
    bool headerSkipped = false;

    while(!ts.atEnd())
    {
        QString line = ts.readLine().trimmed();
        if(line.isEmpty()) {
            continue;
        }

        if(!headerSkipped)
        {
            headerSkipped = true;
            continue;
        }

        const QStringList parts = line.split(";");
        if(parts.size() < 2) {
            continue;
        }

        const auto valueStr = parts[1].trimmed();

        bool ok = false;
        quint16 value = 0;

        if(valueStr.startsWith("0x", Qt::CaseInsensitive)) {
            value = valueStr.mid(2).toUShort(&ok, 16);
        }
        else {
            value = valueStr.toUShort(&ok, 10);
        }

        if(!ok)
        {
            QMessageBox::warning(this, tr("Import error"), tr("Invalid value: %1").arg(valueStr));
            return;
        }

        newData.append(value);
    }

    if(newData.isEmpty())
    {
        QMessageBox::warning(this, tr("Warning"), tr("No data found in file."));
        return;
    }

    if(newData.size() != _data.size())
    {
        QMessageBox::warning(this, tr("Warning"), tr("Imported data size (%1) does not match current size (%2).").arg(newData.size()).arg(_data.size()));
    }

    for(int i = 0; i < _data.size(); ++i) {
        if(i < newData.size()) {
            _data[i] = newData[i];
        }
    }

    updateTableWidget();

}

///
/// \brief DialogForceMultipleRegisters::on_pushButtonExport_clicked
///
void DialogForceMultipleRegisters::on_pushButtonExport_clicked()
{
    auto filename = QFileDialog::getSaveFileName(this, QString(), QString(), tr("CSV files (*.csv)"));
    if(filename.isEmpty()) return;

    if(!filename.endsWith(".csv", Qt::CaseInsensitive))
    {
        filename += ".csv";
    }

    QFile file(filename);
    if(!file.open(QFile::WriteOnly))
    {
        QMessageBox::critical(this, tr("Error"), file.errorString());
        return;
    }

    QTextStream ts(&file);
    ts.setGenerateByteOrderMark(true);

    const char* delim = ";";
    ts << "Address" << delim << "Value" << "\n";

    for(int i = 0; i < _data.size(); i++)
    {
        ts << formatAddress(_type, _writeParams.Address + i, _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams))
        << delim
        << QString::number(_data[i])
        << "\n";
    }
}

///
/// \brief DialogForceMultipleRegisters::createNumEdit
/// \param idx
/// \return
///
QLineEdit* DialogForceMultipleRegisters::createNumEdit(int idx)
{
    QLineEdit* lineEdit = nullptr;
    NumericLineEdit* numEdit = nullptr;
    const bool lsrf = _writeParams.RegOrder == RegisterOrder::LSRF;

    switch(_writeParams.DataMode)
    {
        case DataType::Binary:
            numEdit = new NumericLineEdit(NumericLineEdit::BinaryMode, ui->tableWidget);
            numEdit->setInputRange(0, USHRT_MAX);
            numEdit->setLeadingZeroes(true);
            numEdit->setValue(toByteOrderValue(_data[idx], _writeParams.Order));
        break;

        case DataType::Hex:
        case DataType::Ansi:
            numEdit = new NumericLineEdit(NumericLineEdit::HexMode, ui->tableWidget);
            numEdit->setInputRange(0, USHRT_MAX);
            numEdit->setLeadingZeroes(true);
            numEdit->setValue(toByteOrderValue(_data[idx], _writeParams.Order));
        break;

        case DataType::UInt16:
            numEdit = new NumericLineEdit(NumericLineEdit::UInt32Mode, ui->tableWidget);
            numEdit->setInputRange(0, USHRT_MAX);
            numEdit->setLeadingZeroes(true);
            numEdit->setValue(toByteOrderValue(_data[idx], _writeParams.Order));
        break;

        case DataType::Int16:
            numEdit = new NumericLineEdit(NumericLineEdit::Int32Mode, ui->tableWidget);
            numEdit->setInputRange(SHRT_MIN, SHRT_MAX);
            numEdit->setValue((qint16)toByteOrderValue(_data[idx], _writeParams.Order));
        break;

        case DataType::Int32:
            if(!(idx % 2) && (idx + 1 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::Int32Mode, ui->tableWidget);
                numEdit->setValue(lsrf ? makeInt32(_data[idx + 1], _data[idx], _writeParams.Order)
                                       : makeInt32(_data[idx], _data[idx + 1], _writeParams.Order));
            }
        break;

        case DataType::UInt32:
            if(!(idx % 2) && (idx + 1 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::UInt32Mode, ui->tableWidget);
                numEdit->setValue(lsrf ? makeUInt32(_data[idx + 1], _data[idx], _writeParams.Order)
                                       : makeUInt32(_data[idx], _data[idx + 1], _writeParams.Order));
            }
        break;

        case DataType::Float32:
            if(!(idx % 2) && (idx + 1 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::FloatMode, ui->tableWidget);
                numEdit->setValue(lsrf ? makeFloat(_data[idx + 1], _data[idx], _writeParams.Order)
                                       : makeFloat(_data[idx], _data[idx + 1], _writeParams.Order));
            }
        break;

        case DataType::Float64:
            if(!(idx % 4) && (idx + 3 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::DoubleMode, ui->tableWidget);
                numEdit->setValue(lsrf ? makeDouble(_data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order)
                                       : makeDouble(_data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order));
            }
        break;

        case DataType::Int64:
            if(!(idx % 4) && (idx + 3 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::Int64Mode, ui->tableWidget);
                numEdit->setValue(lsrf ? makeInt64(_data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order)
                                       : makeInt64(_data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order));
            }
            break;

        case DataType::UInt64:
            if(!(idx % 4) && (idx + 3 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::UInt64Mode, ui->tableWidget);
                numEdit->setValue(lsrf ? makeUInt64(_data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order)
                                       : makeUInt64(_data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order));
            }
        break;
    }

    if (numEdit) {
        connect(numEdit,
                static_cast<void (NumericLineEdit::*)(const QVariant&)>(&NumericLineEdit::valueChanged),
                this,
                [this, idx](const QVariant& value) {
                    switch (_writeParams.DataMode) {
                        case DataType::Binary:
                        case DataType::Hex:
                        case DataType::Ansi:
                        case DataType::UInt16:
                        case DataType::Int16:
                            applyValue<quint16>(toByteOrderValue(value.value<quint16>(), _writeParams.Order), idx, ValueOperation::Set);
                        break;

                        case DataType::Int32:
                            if (!(idx % 2) && (idx + 1 < _data.size()))
                                applyValue<qint32>(value.value<qint32>(), idx, ValueOperation::Set);
                        break;

                        case DataType::UInt32:
                            if (!(idx % 2) && (idx + 1 < _data.size()))
                                applyValue<quint32>(value.value<quint32>(), idx, ValueOperation::Set);
                        break;

                        case DataType::Float32:
                            if (!(idx % 2) && (idx + 1 < _data.size()))
                                applyValue<float>(value.value<float>(), idx, ValueOperation::Set);
                        break;

                        case DataType::Float64:
                            if (!(idx % 4) && (idx + 3 < _data.size()))
                                applyValue<double>(value.value<double>(), idx, ValueOperation::Set);
                        break;

                        case DataType::Int64:
                            if (!(idx % 4) && (idx + 3 < _data.size()))
                                applyValue<qint64>(value.value<qint64>(), idx, ValueOperation::Set);
                        break;

                        case DataType::UInt64:
                            if (!(idx % 4) && (idx + 3 < _data.size()))
                                applyValue<quint64>(value.value<quint64>(), idx, ValueOperation::Set);
                        break;
                    }
                });
        lineEdit = numEdit;
    }

    if(lineEdit)
    {
        lineEdit->setFrame(false);
        lineEdit->setFixedWidth(150);
        lineEdit->setAlignment(Qt::AlignCenter);
        lineEdit->setToolTip(QString("%1").arg(_writeParams.Address + idx, 5, 10, QLatin1Char('0')));
    }

    return lineEdit;
}

///
/// \brief DialogForceMultipleRegisters::createLineEdit
/// \return
///
QLineEdit* DialogForceMultipleRegisters::createLineEdit()
{
    auto lineEdit = new QLineEdit(ui->tableWidget);
    lineEdit->setText("-");
    lineEdit->setFrame(false);
    lineEdit->setFixedWidth(150);
    lineEdit->setEnabled(false);
    lineEdit->setAlignment(Qt::AlignCenter);
    return lineEdit;
}

///
/// \brief DialogForceMultipleRegisters::updateTableWidget
///
void DialogForceMultipleRegisters::updateTableWidget()
{
    const int columns = 5;
    const auto length = _data.length();

    ui->tableWidget->clear();
    ui->tableWidget->setColumnCount(columns);
    ui->tableWidget->setRowCount(qCeil(length / (double)columns));

    for(int i = 0; i < ui->tableWidget->columnCount(); i++)
    {
        const auto text = QString("+%1").arg(i);
        ui->tableWidget->setHorizontalHeaderItem(i, new QTableWidgetItem(text));
    }

    for(int i = 0; i < ui->tableWidget->rowCount(); i++)
    {
        const auto addressFrom = formatAddress(_type, _writeParams.Address + i * columns, _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams));
        const auto addressTo = formatAddress(_type, _writeParams.Address + qMin(length - 1, (i + 1) * columns - 1), _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams));
        ui->tableWidget->setVerticalHeaderItem(i, new QTableWidgetItem(QString("%1-%2").arg(addressFrom, addressTo)));

        for(int j = 0; j < columns; j++)
        {
            const auto idx = i * columns + j;
            if(idx < length)
            {
                auto* valueEdit = createNumEdit(idx);
                if(valueEdit) ui->tableWidget->setCellWidget(i, j, valueEdit);
                else ui->tableWidget->setCellWidget(i, j, createLineEdit());
            }
            else
            {
                ui->tableWidget->setCellWidget(i, j, createLineEdit());
            }
        }
    }
    ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}
