#include <QtMath>
#include <QRandomGenerator>
#include <QActionGroup>
#include <QToolButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include "ansimenu.h"
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

    const auto deviceIdStr = displayHexAddresses
        ? QString("0x%1").arg(QString::number(params.DeviceId, 16).toUpper(), 2, '0')
        : QString::number(params.DeviceId);
    const auto lengthStr = displayHexAddresses
        ? QString("0x%1").arg(QString::number(length, 16).toUpper(), 4, '0')
        : QString::number(length);

    ui->labelAddress->setText(QString(ui->labelAddress->text()).arg(
        formatAddress(type, params.Address, params.AddrSpace, _hexAddress)));
    ui->labelLength->setText(QString(ui->labelLength->text()).arg(lengthStr));
    ui->labelSlaveDevice->setText(QString(ui->labelSlaveDevice->text()).arg(deviceIdStr));
    ui->labelAddresses->setText(QString(ui->labelAddresses->text()).arg(
        formatAddress(type, params.Address, params.AddrSpace, _hexAddress),
        formatAddress(type, params.Address + length - 1, params.AddrSpace, _hexAddress)));

    ui->lineEditStep->setValue(1);

    recolorPushButtonIcon(ui->pushButtonExport, Qt::red);
    recolorPushButtonIcon(ui->pushButtonImport, Qt::darkGreen);
    recolorPushButtonIcon(ui->pushButtonValue, Qt::darkMagenta);
    recolorPushButtonIcon(ui->pushButtonInc, Qt::darkMagenta);

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

    setupDisplayBar();
    setupEditorInputs();

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
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    QDialog::changeEvent(event);
}

///
/// \brief DialogForceMultipleRegisters::setupDisplayBar
///
void DialogForceMultipleRegisters::setupDisplayBar()
{
    auto group = new QActionGroup(ui->frameDisplay);
    group->setExclusive(true);

    auto addModeAction = [&](DataType type, RegisterOrder order, QAction* action)
    {
        group->addAction(action);
        _displayModeActions[{type, order}] = action;

        connect(action, &QAction::triggered, this, [this, type, order](bool checked) {
            if (!checked) return;
            _writeParams.DataMode = type;
            _writeParams.RegOrder = order;
            setupEditorInputs();
            updateTableWidget();
            updateDisplayBar();
        });
    };

    const auto msrf = RegisterOrder::MSRF;
    const auto lsrf = RegisterOrder::LSRF;

    addModeAction(DataType::Binary,  msrf, ui->actionDisplayBinary);
    addModeAction(DataType::Hex,     msrf, ui->actionDisplayHex);
    addModeAction(DataType::Ansi,    msrf, ui->actionDisplayAnsi);
    addModeAction(DataType::Int16,   msrf, ui->actionDisplayInt16);
    addModeAction(DataType::UInt16,  msrf, ui->actionDisplayUInt16);
    addModeAction(DataType::Int32,   msrf, ui->actionDisplayInt32);
    addModeAction(DataType::Int32,   lsrf, ui->actionDisplayInt32LSRF);
    addModeAction(DataType::UInt32,  msrf, ui->actionDisplayUInt32);
    addModeAction(DataType::UInt32,  lsrf, ui->actionDisplayUInt32LSRF);
    addModeAction(DataType::Int64,   msrf, ui->actionDisplayInt64);
    addModeAction(DataType::Int64,   lsrf, ui->actionDisplayInt64LSRF);
    addModeAction(DataType::UInt64,  msrf, ui->actionDisplayUInt64);
    addModeAction(DataType::UInt64,  lsrf, ui->actionDisplayUInt64LSRF);
    addModeAction(DataType::Float32, msrf, ui->actionDisplayFloat32);
    addModeAction(DataType::Float32, lsrf, ui->actionDisplayFloat32LSRF);
    addModeAction(DataType::Float64, msrf, ui->actionDisplayFloat64);
    addModeAction(DataType::Float64, lsrf, ui->actionDisplayFloat64LSRF);

    auto bindButton = [](QToolButton* button, QAction* action) {
        button->setIconSize(QSize(24, 24));
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setDefaultAction(action);
    };

    bindButton(ui->toolButtonDisplayBinary, ui->actionDisplayBinary);
    bindButton(ui->toolButtonDisplayHex, ui->actionDisplayHex);
    bindButton(ui->toolButtonDisplayAnsi, ui->actionDisplayAnsi);
    bindButton(ui->toolButtonDisplayInt16, ui->actionDisplayInt16);
    bindButton(ui->toolButtonDisplayUInt16, ui->actionDisplayUInt16);
    bindButton(ui->toolButtonDisplayInt32, ui->actionDisplayInt32);
    bindButton(ui->toolButtonDisplayInt32LSRF, ui->actionDisplayInt32LSRF);
    bindButton(ui->toolButtonDisplayUInt32, ui->actionDisplayUInt32);
    bindButton(ui->toolButtonDisplayUInt32LSRF, ui->actionDisplayUInt32LSRF);
    bindButton(ui->toolButtonDisplayInt64, ui->actionDisplayInt64);
    bindButton(ui->toolButtonDisplayInt64LSRF, ui->actionDisplayInt64LSRF);
    bindButton(ui->toolButtonDisplayUInt64, ui->actionDisplayUInt64);
    bindButton(ui->toolButtonDisplayUInt64LSRF, ui->actionDisplayUInt64LSRF);
    bindButton(ui->toolButtonDisplayFloat32, ui->actionDisplayFloat32);
    bindButton(ui->toolButtonDisplayFloat32LSRF, ui->actionDisplayFloat32LSRF);
    bindButton(ui->toolButtonDisplayFloat64, ui->actionDisplayFloat64);
    bindButton(ui->toolButtonDisplayFloat64LSRF, ui->actionDisplayFloat64LSRF);
    bindButton(ui->toolButtonDisplaySwapBytes, ui->actionDisplaySwapBytes);

    _ansiMenu = new AnsiMenu(this);
    connect(_ansiMenu, &AnsiMenu::codepageSelected, this, [this](const QString& name) {
        _writeParams.Codepage = name;
        if (_writeParams.DataMode == DataType::Ansi) {
            setupEditorInputs();
            updateTableWidget();
            updateDisplayBar();
        }
    });
    ui->actionDisplayAnsi->setMenu(_ansiMenu);
    ui->toolButtonDisplayAnsi->setPopupMode(QToolButton::DelayedPopup);

    connect(ui->actionDisplaySwapBytes, &QAction::triggered, this, [this](bool checked) {
        _writeParams.Order = checked ? ByteOrder::Swapped : ByteOrder::Direct;
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
    const auto it = _displayModeActions.find({_writeParams.DataMode, _writeParams.RegOrder});
    if (it != _displayModeActions.end()) {
        it.value()->setChecked(true);
    } else {
        const auto fallback = _displayModeActions.find({_writeParams.DataMode, RegisterOrder::MSRF});
        if (fallback != _displayModeActions.end())
            fallback.value()->setChecked(true);
    }

    ui->actionDisplaySwapBytes->setChecked(_writeParams.Order == ByteOrder::Swapped);

    if (_ansiMenu) {
        _ansiMenu->selectCodepage(_writeParams.Codepage);
    }
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
        case DataType::Hex:
            setupLineEdit<quint16>(ui->lineEditValue, NumericLineEdit::HexMode, true);
            setupLineEdit<quint16>(ui->lineEditStartValue, NumericLineEdit::HexMode, true);
            setupLineEdit<quint16>(ui->lineEditStep, NumericLineEdit::HexMode, true);
            ui->lineEditStartValue->setValue(toByteOrderValue(firstReg, _writeParams.Order));
        break;

        case DataType::Ansi:
            setupLineEdit<quint16>(ui->lineEditValue, NumericLineEdit::AnsiMode);
            ui->lineEditValue->setCodepage(_writeParams.Codepage);
            setupLineEdit<quint16>(ui->lineEditStartValue, NumericLineEdit::UInt32Mode);
            setupLineEdit<qint16>(ui->lineEditStep, NumericLineEdit::Int32Mode);
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
            case DataType::Ansi:
            case DataType::Binary:
            case DataType::UInt16:
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
    for(int i = 0; i < ui->tableWidget->rowCount(); i++)
    {
        for(int j = 0; j < ui->tableWidget->columnCount(); j++)
        {
            const auto idx = i *  ui->tableWidget->columnCount() + j;
            if(idx >= _data.size())
            {
                break;
            }

            const bool lsrf = _writeParams.RegOrder == RegisterOrder::LSRF;

            switch(_writeParams.DataMode)
            {
                case DataType::Binary:
                {
                    auto* lineEdit = qobject_cast<QLineEdit*>(ui->tableWidget->cellWidget(i, j));
                    bool ok = false;
                    const auto parsed = lineEdit ? lineEdit->text().trimmed().toUShort(&ok, 2) : 0;
                    if (ok) {
                        _data[idx] = toByteOrderValue(parsed, _writeParams.Order);
                    }
                }
                break;

                case DataType::Hex:
                case DataType::Ansi:
                case DataType::UInt16:
                case DataType::Int16:
                {
                    auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                    _data[idx] = toByteOrderValue(numEdit->value<quint16>(), _writeParams.Order);
                }
                break;

                case DataType::Int32:
                    if(!(idx % 2) && (idx + 1 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        if(lsrf) breakInt32(numEdit->value<qint32>(), _data[idx + 1], _data[idx], _writeParams.Order);
                        else     breakInt32(numEdit->value<qint32>(), _data[idx], _data[idx + 1], _writeParams.Order);
                    }
                break;

                case DataType::UInt32:
                    if(!(idx % 2) && (idx + 1 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        if(lsrf) breakUInt32(numEdit->value<quint32>(), _data[idx + 1], _data[idx], _writeParams.Order);
                        else     breakUInt32(numEdit->value<quint32>(), _data[idx], _data[idx + 1], _writeParams.Order);
                    }
                break;

                case DataType::Float32:
                    if(!(idx % 2) && (idx + 1 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        if(lsrf) breakFloat(numEdit->value<double>(), _data[idx + 1], _data[idx], _writeParams.Order);
                        else     breakFloat(numEdit->value<double>(), _data[idx], _data[idx + 1], _writeParams.Order);
                    }
                break;

                case DataType::Float64:
                    if(!(idx % 4) && (idx + 3 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        if(lsrf) breakDouble(numEdit->value<double>(), _data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order);
                        else     breakDouble(numEdit->value<double>(), _data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order);
                    }
                break;

                case DataType::Int64:
                    if(!(idx % 4) && (idx + 3 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        if(lsrf) breakInt64(numEdit->value<qint64>(), _data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order);
                        else     breakInt64(numEdit->value<qint64>(), _data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order);
                    }
                    break;

                case DataType::UInt64:
                    if(!(idx % 4) && (idx + 3 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        if(lsrf) breakUInt64(numEdit->value<quint64>(), _data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order);
                        else     breakUInt64(numEdit->value<quint64>(), _data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order);
                    }
                break;
            }
        }
    }

    _writeParams.Value = QVariant::fromValue(_data);
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
        case DataType::Ansi:
        case DataType::UInt16:
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
    if (_writeParams.DataMode == DataType::Binary) {
        applyToAll(ValueOperation::Set, parseBinary16(ui->lineEditValue->text()));
        return;
    }

    applyToAll(ValueOperation::Set, ui->lineEditValue->value<double>());
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
            case DataType::Ansi:
            case DataType::UInt16:
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
        ts << formatAddress(_type, _writeParams.Address + i, _writeParams.AddrSpace, _hexAddress)
        << delim
///
/// \brief QString::number
///
        << QString::number(_data[i])
        << "\n";
    }
}

///
/// \brief DialogForceMultipleRegisters::createNumEdit
/// \param idx
/// \return
///
NumericLineEdit* DialogForceMultipleRegisters::createNumEdit(int idx)
{
    NumericLineEdit* numEdit = nullptr;
    const bool lsrf = _writeParams.RegOrder == RegisterOrder::LSRF;

    switch(_writeParams.DataMode)
    {
        case DataType::Hex:
            numEdit = new NumericLineEdit(NumericLineEdit::HexMode, ui->tableWidget);
            numEdit->setInputRange(0, USHRT_MAX);
            numEdit->setLeadingZeroes(true);
            numEdit->setValue(toByteOrderValue(_data[idx], _writeParams.Order));
        break;

        case DataType::Ansi:
            numEdit = new NumericLineEdit(NumericLineEdit::AnsiMode, ui->tableWidget);
            numEdit->setInputRange(0, USHRT_MAX);
            numEdit->setCodepage(_writeParams.Codepage);
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

    if(numEdit)
    {
        numEdit->setFrame(false);
        numEdit->setFixedWidth(150);
        numEdit->setAlignment(Qt::AlignCenter);
        numEdit->setToolTip(QString("%1").arg(_writeParams.Address + idx, 5, 10, QLatin1Char('0')));
    }

    return numEdit;
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

QLineEdit* DialogForceMultipleRegisters::createBinaryEdit(int idx)
{
    auto* lineEdit = new QLineEdit(ui->tableWidget);
    const auto value = toByteOrderValue(_data[idx], _writeParams.Order);
    lineEdit->setText(QStringLiteral("%1").arg(value, 16, 2, QLatin1Char('0')));
    lineEdit->setFrame(false);
    lineEdit->setFixedWidth(150);
    lineEdit->setAlignment(Qt::AlignCenter);
    lineEdit->setToolTip(QString("%1").arg(_writeParams.Address + idx, 5, 10, QLatin1Char('0')));
    lineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[01]{1,16}$"), lineEdit));
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
        const auto addressFrom = formatAddress(QModbusDataUnit::HoldingRegisters, _writeParams.Address + i * columns, _writeParams.AddrSpace, _hexAddress);
        const auto addressTo = formatAddress(QModbusDataUnit::HoldingRegisters, _writeParams.Address + qMin(length - 1, (i + 1) * columns - 1), _writeParams.AddrSpace, _hexAddress);
        ui->tableWidget->setVerticalHeaderItem(i, new QTableWidgetItem(QString("%1-%2").arg(addressFrom, addressTo)));

        for(int j = 0; j < columns; j++)
        {
            const auto idx = i * columns + j;
            if(idx < length)
            {
                if (_writeParams.DataMode == DataType::Binary) {
                    ui->tableWidget->setCellWidget(i, j, createBinaryEdit(idx));
                } else {
                    auto numEdit = createNumEdit(idx);
                    if(numEdit) ui->tableWidget->setCellWidget(i, j, numEdit);
                    else ui->tableWidget->setCellWidget(i, j, createLineEdit());
                }
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
