#include <QtMath>
#include <QRandomGenerator>
#include "formatutils.h"
#include "numericutils.h"
#include "numericlineedit.h"
#include "dialogforcemultipleregisters.h"
#include "ui_dialogforcemultipleregisters.h"

///
/// \brief DialogForceMultipleRegisters::DialogForceMultipleRegisters
/// \param params
/// \param type
/// \param length
/// \param hexAddress
/// \param parent
///
DialogForceMultipleRegisters::DialogForceMultipleRegisters(ModbusWriteParams& params, QModbusDataUnit::RegisterType type, int length, bool hexAddress, QWidget *parent) :
      QDialog(parent)
    , ui(new Ui::DialogForceMultipleRegisters)
    ,_writeParams(params)
    ,_type(type)
    ,_hexAddress(hexAddress)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog |
                   Qt::CustomizeWindowHint |
                   Qt::WindowTitleHint);

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

    ui->labelAddress->setText(QString(tr("Address: <b>%1</b>")).arg(formatAddress(type, params.Address, params.AddrSpace, _hexAddress)));
    ui->labelLength->setText(QString(tr("Length: <b>%1</b>")).arg(length, 3, 10, QLatin1Char('0')));

    _data = params.Value.value<QVector<quint16>>();
    if(_data.length() != length) _data.resize(length);

    switch(_writeParams.DisplayMode)
    {
        case DataDisplayMode::Hex:
            setupLineEdit<quint16>(ui->lineEditValue, NumericLineEdit::HexMode, true);
            setupLineEdit<quint16>(ui->lineEditStartValue, NumericLineEdit::HexMode, true);
            setupLineEdit<quint16>(ui->lineEditStep, NumericLineEdit::HexMode, true);
            ui->lineEditStartValue->setValue(toByteOrderValue(_data[0], _writeParams.Order));
        break;

        case DataDisplayMode::Ansi:
            setupLineEdit<quint16>(ui->lineEditValue, NumericLineEdit::AnsiMode);
            ui->lineEditValue->setCodepage(params.Codepage);
            setupLineEdit<quint16>(ui->lineEditStartValue, NumericLineEdit::UInt32Mode);
            setupLineEdit<qint16>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(toByteOrderValue(_data[0], _writeParams.Order));
        break;

        case DataDisplayMode::Int16:
            setupLineEdit<qint16>(ui->lineEditValue, NumericLineEdit::Int32Mode);
            setupLineEdit<qint16>(ui->lineEditStartValue, NumericLineEdit::Int32Mode);
            setupLineEdit<qint16>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(toByteOrderValue(_data[0], _writeParams.Order));
        break;

        case DataDisplayMode::Binary:
        case DataDisplayMode::UInt16:
            setupLineEdit<quint16>(ui->lineEditValue, NumericLineEdit::UInt32Mode, true);
            setupLineEdit<quint16>(ui->lineEditStartValue, NumericLineEdit::UInt32Mode, true);
            setupLineEdit<quint16>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(toByteOrderValue(_data[0], _writeParams.Order));
            break;

        case DataDisplayMode::Int32:
            setupLineEdit<qint32>(ui->lineEditValue, NumericLineEdit::Int32Mode);
            setupLineEdit<qint32>(ui->lineEditStartValue, NumericLineEdit::Int32Mode);
            setupLineEdit<qint32>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(makeInt32(_data[0], _data[1], _writeParams.Order));
        break;
        case DataDisplayMode::SwappedInt32:
            setupLineEdit<qint32>(ui->lineEditValue, NumericLineEdit::Int32Mode);
            setupLineEdit<qint32>(ui->lineEditStartValue, NumericLineEdit::Int32Mode);
            setupLineEdit<qint32>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(makeInt32(_data[1], _data[0], _writeParams.Order));
        break;

        case DataDisplayMode::UInt32:
            setupLineEdit<quint32>(ui->lineEditValue, NumericLineEdit::UInt32Mode);
            setupLineEdit<quint32>(ui->lineEditStartValue, NumericLineEdit::UInt32Mode);
            setupLineEdit<qint32>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(makeUInt32(_data[0], _data[1], _writeParams.Order));
        break;
        case DataDisplayMode::SwappedUInt32:
            setupLineEdit<quint32>(ui->lineEditValue, NumericLineEdit::UInt32Mode);
            setupLineEdit<quint32>(ui->lineEditStartValue, NumericLineEdit::UInt32Mode);
            setupLineEdit<qint32>(ui->lineEditStep, NumericLineEdit::Int32Mode);
            ui->lineEditStartValue->setValue(makeUInt32(_data[1], _data[0], _writeParams.Order));
        break;

        case DataDisplayMode::Int64:
            setupLineEdit<qint64>(ui->lineEditValue, NumericLineEdit::Int64Mode);
            setupLineEdit<qint64>(ui->lineEditStartValue, NumericLineEdit::Int64Mode);
            setupLineEdit<qint64>(ui->lineEditStep, NumericLineEdit::Int64Mode);
            ui->lineEditStartValue->setValue(makeInt64(_data[0], _data[1], _data[2], _data[3], _writeParams.Order));
        break;
        case DataDisplayMode::SwappedInt64:
            setupLineEdit<qint64>(ui->lineEditValue, NumericLineEdit::Int64Mode);
            setupLineEdit<qint64>(ui->lineEditStartValue, NumericLineEdit::Int64Mode);
            setupLineEdit<qint64>(ui->lineEditStep, NumericLineEdit::Int64Mode);
            ui->lineEditStartValue->setValue(makeInt64(_data[3], _data[2], _data[1], _data[0], _writeParams.Order));
        break;

        case DataDisplayMode::UInt64:
            setupLineEdit<quint64>(ui->lineEditValue, NumericLineEdit::UInt64Mode);
            setupLineEdit<quint64>(ui->lineEditStartValue, NumericLineEdit::UInt64Mode);
            setupLineEdit<qint64>(ui->lineEditStep, NumericLineEdit::Int64Mode);
            ui->lineEditStartValue->setValue(makeInt64(_data[0], _data[1], _data[2], _data[3], _writeParams.Order));
        break;
        case DataDisplayMode::SwappedUInt64:
            setupLineEdit<quint64>(ui->lineEditValue, NumericLineEdit::UInt64Mode);
            setupLineEdit<quint64>(ui->lineEditStartValue, NumericLineEdit::UInt64Mode);
            setupLineEdit<qint64>(ui->lineEditStep, NumericLineEdit::Int64Mode);
            ui->lineEditStartValue->setValue(makeUInt64(_data[3], _data[2], _data[1], _data[0], _writeParams.Order));
        break;

        case DataDisplayMode::FloatingPt:
            setupLineEdit<float>(ui->lineEditValue, NumericLineEdit::FloatMode);
            setupLineEdit<float>(ui->lineEditStartValue, NumericLineEdit::FloatMode);
            setupLineEdit<float>(ui->lineEditStep, NumericLineEdit::FloatMode);
            ui->lineEditStartValue->setValue(makeFloat(_data[0], _data[1], _writeParams.Order));
        break;
        case DataDisplayMode::SwappedFP:
            setupLineEdit<float>(ui->lineEditValue, NumericLineEdit::FloatMode);
            setupLineEdit<float>(ui->lineEditStartValue, NumericLineEdit::FloatMode);
            setupLineEdit<float>(ui->lineEditStep, NumericLineEdit::FloatMode);
            ui->lineEditStartValue->setValue(makeFloat(_data[1], _data[0], _writeParams.Order));
        break;

        case DataDisplayMode::DblFloat:
            setupLineEdit<double>(ui->lineEditValue, NumericLineEdit::DoubleMode);
            setupLineEdit<double>(ui->lineEditStartValue, NumericLineEdit::DoubleMode);
            setupLineEdit<double>(ui->lineEditStep, NumericLineEdit::DoubleMode);
            ui->lineEditStartValue->setValue(makeDouble(_data[0], _data[1], _data[2], _data[3], _writeParams.Order));
        break;
        case DataDisplayMode::SwappedDbl:
            setupLineEdit<double>(ui->lineEditValue, NumericLineEdit::DoubleMode);
            setupLineEdit<double>(ui->lineEditStartValue, NumericLineEdit::DoubleMode);
            setupLineEdit<double>(ui->lineEditStep, NumericLineEdit::DoubleMode);
            ui->lineEditStartValue->setValue(makeDouble(_data[3], _data[2], _data[1], _data[0], _writeParams.Order));
        break;
    }

    ui->lineEditStep->setValue(1);
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

            switch(_writeParams.DisplayMode)
            {
                case DataDisplayMode::Binary:
                case DataDisplayMode::Hex:
                case DataDisplayMode::Ansi:
                case DataDisplayMode::UInt16:
                case DataDisplayMode::Int16:
                {
                    auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                    _data[idx] = toByteOrderValue(numEdit->value<quint16>(), _writeParams.Order);
                }
                break;
                    
                case DataDisplayMode::Int32:
                    if(!(idx % 2) && (idx + 1 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakInt32(numEdit->value<qint32>(), _data[idx], _data[idx + 1], _writeParams.Order);
                    }
                break;

                case DataDisplayMode::SwappedInt32:
                    if(!(idx % 2) && (idx + 1 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakInt32(numEdit->value<qint32>(), _data[idx + 1], _data[idx], _writeParams.Order);
                    }
                break;

                case DataDisplayMode::UInt32:
                    if(!(idx % 2) && (idx + 1 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakUInt32(numEdit->value<quint32>(), _data[idx], _data[idx + 1], _writeParams.Order);
                    }
                break;

                case DataDisplayMode::SwappedUInt32:
                    if(!(idx % 2) && (idx + 1 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakUInt32(numEdit->value<quint32>(), _data[idx + 1], _data[idx], _writeParams.Order);
                    }
                break;

                case DataDisplayMode::FloatingPt:
                    if(!(idx % 2) && (idx + 1 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakFloat(numEdit->value<double>(), _data[idx], _data[idx + 1], _writeParams.Order);
                    }
                break;

                case DataDisplayMode::SwappedFP:
                    if(!(idx % 2) && (idx + 1 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakFloat(numEdit->value<double>(), _data[idx + 1], _data[idx], _writeParams.Order);
                    }
                break;

                case DataDisplayMode::DblFloat:
                    if(!(idx % 4) && (idx + 3 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakDouble(numEdit->value<double>(), _data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order);
                    }
                break;

                case DataDisplayMode::SwappedDbl:
                    if(!(idx % 4) && (idx + 3 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakDouble(numEdit->value<double>(), _data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order);
                    }
                break;

                case DataDisplayMode::Int64:
                    if(!(idx % 4) && (idx + 3 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakInt64(numEdit->value<qint64>(), _data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order);
                    }
                    break;

                case DataDisplayMode::SwappedInt64:
                    if(!(idx % 4) && (idx + 3 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakInt64(numEdit->value<qint64>(), _data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order);
                    }
                    break;

                case DataDisplayMode::UInt64:
                    if(!(idx % 4) && (idx + 3 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakUInt64(numEdit->value<quint64>(), _data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order);
                    }
                    break;

                case DataDisplayMode::SwappedUInt64:
                    if(!(idx % 4) && (idx + 3 < _data.size()))
                    {
                        auto numEdit = (NumericLineEdit*)ui->tableWidget->cellWidget(i, j);
                        breakUInt64(numEdit->value<quint64>(), _data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order);
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
    for(auto& v : _data)
    {
        v = 0;
    }

    updateTableWidget();
}

///
/// \brief DialogForceMultipleRegisters::on_pushButtonRandom_clicked
///
void DialogForceMultipleRegisters::on_pushButtonRandom_clicked()
{
    for(int i = 0; i < _data.size(); i++)
    {
        switch(_writeParams.DisplayMode)
        {
            case DataDisplayMode::Binary:
            case DataDisplayMode::Hex:
            case DataDisplayMode::Ansi:
            case DataDisplayMode::UInt16:
                _data[i] = QRandomGenerator::global()->bounded(0, USHRT_MAX);
            break;

            case DataDisplayMode::Int16:
                _data[i] = QRandomGenerator::global()->bounded(SHRT_MIN, SHRT_MAX);
            break;
                
            case DataDisplayMode::Int32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakInt32(QRandomGenerator::global()->bounded(INT_MIN, INT_MAX), _data[i], _data[i + 1], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedInt32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakInt32(QRandomGenerator::global()->bounded(INT_MIN, INT_MAX), _data[i + 1], _data[i], _writeParams.Order);
            break;

            case DataDisplayMode::UInt32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakUInt32(QRandomGenerator::global()->bounded(0U, UINT_MAX), _data[i], _data[i + 1], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedUInt32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakUInt32(QRandomGenerator::global()->bounded(0U, UINT_MAX), _data[i + 1], _data[i], _writeParams.Order);
            break;

            case DataDisplayMode::FloatingPt:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakFloat(QRandomGenerator::global()->bounded(100.), _data[i], _data[i + 1], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedFP:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakFloat(QRandomGenerator::global()->bounded(100.), _data[i + 1], _data[i], _writeParams.Order);
            break;

            case DataDisplayMode::DblFloat:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakDouble(QRandomGenerator::global()->bounded(100.), _data[i], _data[i + 1], _data[i + 2], _data[i + 3], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedDbl:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakDouble(QRandomGenerator::global()->bounded(100.), _data[i + 3], _data[i + 2], _data[i + 1], _data[i], _writeParams.Order);
            break;

            case DataDisplayMode::Int64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakInt64((qint64)QRandomGenerator::global()->generate64(), _data[i], _data[i + 1], _data[i + 2], _data[i + 3], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedInt64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakInt64((qint64)QRandomGenerator::global()->generate64(), _data[i + 3], _data[i + 2], _data[i + 1], _data[i], _writeParams.Order);
            break;

            case DataDisplayMode::UInt64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakUInt64(QRandomGenerator::global()->generate64(), _data[i], _data[i + 1], _data[i + 2], _data[i + 3], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedUInt64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakUInt64(QRandomGenerator::global()->generate64(), _data[i + 3], _data[i + 2], _data[i + 1], _data[i], _writeParams.Order);
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
    for(int i = 0; i < _data.size(); i++)
    {
        switch(_writeParams.DisplayMode)
        {
            case DataDisplayMode::Hex:
            case DataDisplayMode::Ansi:
            case DataDisplayMode::Binary:
            case DataDisplayMode::UInt16:
                _data[i] = ui->lineEditValue->value<quint16>();
            break;

            case DataDisplayMode::Int16:
                _data[i] = ui->lineEditValue->value<qint16>();
            break;

            case DataDisplayMode::Int32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakInt32(ui->lineEditValue->value<qint32>(), _data[i], _data[i + 1], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedInt32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakInt32(ui->lineEditValue->value<qint32>(), _data[i + 1], _data[i], _writeParams.Order);
            break;

            case DataDisplayMode::UInt32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakUInt32(ui->lineEditValue->value<quint32>(), _data[i], _data[i + 1], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedUInt32:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakUInt32(ui->lineEditValue->value<quint32>(), _data[i + 1], _data[i], _writeParams.Order);
            break;

            case DataDisplayMode::Int64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakInt64(ui->lineEditValue->value<qint64>(), _data[i], _data[i + 1], _data[i + 2], _data[i + 3], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedInt64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakInt64(ui->lineEditValue->value<qint64>(), _data[i + 3], _data[i + 2], _data[i + 1], _data[i], _writeParams.Order);
            break;

            case DataDisplayMode::UInt64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakUInt64(ui->lineEditValue->value<quint64>(), _data[i], _data[i + 1], _data[i + 2], _data[i + 3], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedUInt64:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakUInt64(ui->lineEditValue->value<quint64>(), _data[i + 3], _data[i + 2], _data[i + 1], _data[i], _writeParams.Order);
            break;

            case DataDisplayMode::FloatingPt:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakFloat(ui->lineEditValue->value<float>(), _data[i], _data[i + 1], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedFP:
                if(!(i % 2) && (i + 1 < _data.size()))
                    breakFloat(ui->lineEditValue->value<float>(), _data[i + 1], _data[i], _writeParams.Order);
            break;

            case DataDisplayMode::DblFloat:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakDouble(ui->lineEditValue->value<double>(), _data[i], _data[i + 1], _data[i + 2], _data[i + 3], _writeParams.Order);
            break;

            case DataDisplayMode::SwappedDbl:
                if(!(i % 4) && (i + 3 < _data.size()))
                    breakDouble(ui->lineEditValue->value<double>(), _data[i + 3], _data[i + 2], _data[i + 1], _data[i], _writeParams.Order);
            break;
        }
    }

    updateTableWidget();
}

///
/// \brief DialogForceMultipleRegisters::on_pushButtonInc_clicked
///
void DialogForceMultipleRegisters::on_pushButtonInc_clicked()
{
    for(int i = 0; i < _data.size(); i++)
    {
        switch(_writeParams.DisplayMode)
        {
            case DataDisplayMode::Hex:
            case DataDisplayMode::Ansi:
            case DataDisplayMode::Binary:
            case DataDisplayMode::UInt16:
                _data[i] = ui->lineEditStartValue->value<quint16>() + i * ui->lineEditStep->value<qint16>();
            break;

            case DataDisplayMode::Int16:
                _data[i] = ui->lineEditStartValue->value<qint16>() + i * ui->lineEditStep->value<qint16>();
            break;

            case DataDisplayMode::Int32:
                if(!(i % 2) && (i + 1 < _data.size())) {
                    const qint32 value = ui->lineEditStartValue->value<qint32>() + (i / 2) * ui->lineEditStep->value<qint32>();
                    breakInt32(value, _data[i], _data[i + 1], _writeParams.Order);
                }
            break;

            case DataDisplayMode::SwappedInt32:
                if(!(i % 2) && (i + 1 < _data.size())) {
                    const qint32 value = ui->lineEditStartValue->value<qint32>() + (i / 2) * ui->lineEditStep->value<qint32>();
                    breakInt32(value, _data[i + 1], _data[i], _writeParams.Order);
                }
            break;

            case DataDisplayMode::UInt32:
                if(!(i % 2) && (i + 1 < _data.size())) {
                    const quint32 value = ui->lineEditStartValue->value<quint32>() + (i / 2) * ui->lineEditStep->value<qint32>();
                    breakUInt32(value, _data[i], _data[i + 1], _writeParams.Order);
                }
            break;

            case DataDisplayMode::SwappedUInt32:
                if(!(i % 2) && (i + 1 < _data.size())) {
                    const quint32 value = ui->lineEditStartValue->value<quint32>() + (i / 2) * ui->lineEditStep->value<qint32>();
                    breakUInt32(value, _data[i + 1], _data[i], _writeParams.Order);
                }
            break;

            case DataDisplayMode::Int64:
                if(!(i % 4) && (i + 3 < _data.size())) {
                    const qint64 value = ui->lineEditStartValue->value<qint64>() + (i / 4) * ui->lineEditStep->value<qint64>();
                    breakInt64(value, _data[i], _data[i + 1], _data[i + 2], _data[i + 3], _writeParams.Order);
                }
            break;

            case DataDisplayMode::SwappedInt64:
                if(!(i % 4) && (i + 3 < _data.size())) {
                    const qint64 value = ui->lineEditStartValue->value<qint64>() + (i / 4) * ui->lineEditStep->value<qint64>();
                    breakInt64(value, _data[i + 3], _data[i + 2], _data[i + 1], _data[i], _writeParams.Order);
                }
            break;

            case DataDisplayMode::UInt64:
                if(!(i % 4) && (i + 3 < _data.size())) {
                    const quint64 value = ui->lineEditStartValue->value<quint64>() + (i / 4) * ui->lineEditStep->value<qint64>();
                    breakUInt64(value, _data[i], _data[i + 1], _data[i + 2], _data[i + 3], _writeParams.Order);
                }
            break;

            case DataDisplayMode::SwappedUInt64:
                if(!(i % 4) && (i + 3 < _data.size())) {
                    const quint64 value = ui->lineEditStartValue->value<quint64>() + (i / 4) * ui->lineEditStep->value<qint64>();
                    breakUInt64(value, _data[i + 3], _data[i + 2], _data[i + 1], _data[i], _writeParams.Order);
                }
            break;

            case DataDisplayMode::FloatingPt:
                if(!(i % 2) && (i + 1 < _data.size())) {
                    const float value = ui->lineEditStartValue->value<float>() + (i / 2) * ui->lineEditStep->value<float>();
                    breakFloat(value, _data[i], _data[i + 1], _writeParams.Order);
                }
            break;

            case DataDisplayMode::SwappedFP:
                if(!(i % 2) && (i + 1 < _data.size())) {
                    const float value = ui->lineEditStartValue->value<float>() + (i / 2) * ui->lineEditStep->value<float>();
                    breakFloat(value, _data[i + 1], _data[i], _writeParams.Order);
                }
            break;

            case DataDisplayMode::DblFloat:
                if(!(i % 4) && (i + 3 < _data.size())) {
                    const double value = ui->lineEditStartValue->value<double>() + (i / 4) * ui->lineEditStep->value<double>();
                    breakDouble(value, _data[i], _data[i + 1], _data[i + 2], _data[i + 3], _writeParams.Order);
                }
            break;

            case DataDisplayMode::SwappedDbl:
                if(!(i % 4) && (i + 3 < _data.size())) {
                    const double value = ui->lineEditStartValue->value<double>() + (i / 4) * ui->lineEditStep->value<double>();
                    breakDouble(value, _data[i + 3], _data[i + 2], _data[i + 1], _data[i], _writeParams.Order);
                }
            break;
        }
    }

    updateTableWidget();
}

///
/// \brief DialogForceMultipleRegisters::createNumEdit
/// \param idx
/// \return
///
NumericLineEdit* DialogForceMultipleRegisters::createNumEdit(int idx)
{
    NumericLineEdit* numEdit = nullptr;
    switch(_writeParams.DisplayMode)
    {
        case DataDisplayMode::Binary:
        case DataDisplayMode::Hex:
            numEdit = new NumericLineEdit(NumericLineEdit::HexMode, ui->tableWidget);
            numEdit->setInputRange(0, USHRT_MAX);
            numEdit->setPaddingZeroes(true);
            numEdit->setValue(toByteOrderValue(_data[idx], _writeParams.Order));
        break;

        case DataDisplayMode::Ansi:
            numEdit = new NumericLineEdit(NumericLineEdit::AnsiMode, ui->tableWidget);
            numEdit->setInputRange(0, USHRT_MAX);
            numEdit->setCodepage(_writeParams.Codepage);
            numEdit->setValue(toByteOrderValue(_data[idx], _writeParams.Order));
            break;

        case DataDisplayMode::UInt16:
            numEdit = new NumericLineEdit(NumericLineEdit::UInt32Mode, ui->tableWidget);
            numEdit->setInputRange(0, USHRT_MAX);
            numEdit->setPaddingZeroes(true);
            numEdit->setValue(toByteOrderValue(_data[idx], _writeParams.Order));
        break;

        case DataDisplayMode::Int16:
            numEdit = new NumericLineEdit(NumericLineEdit::Int32Mode, ui->tableWidget);
            numEdit->setInputRange(SHRT_MIN, SHRT_MAX);
            numEdit->setValue((qint16)toByteOrderValue(_data[idx], _writeParams.Order));
        break;
            
        case DataDisplayMode::Int32:
            if(!(idx % 2) && (idx + 1 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::Int32Mode, ui->tableWidget);
                numEdit->setValue(makeInt32(_data[idx], _data[idx + 1], _writeParams.Order));
            }
        break;

        case DataDisplayMode::SwappedInt32:
            if(!(idx % 2) && (idx + 1 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::Int32Mode, ui->tableWidget);
                numEdit->setValue(makeInt32(_data[idx + 1], _data[idx], _writeParams.Order));
            }
        break;

        case DataDisplayMode::UInt32:
            if(!(idx % 2) && (idx + 1 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::UInt32Mode, ui->tableWidget);
                numEdit->setValue(makeUInt32(_data[idx], _data[idx + 1], _writeParams.Order));
            }
        break;

        case DataDisplayMode::SwappedUInt32:
            if(!(idx % 2) && (idx + 1 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::UInt32Mode, ui->tableWidget);
                numEdit->setValue(makeUInt32(_data[idx + 1], _data[idx], _writeParams.Order));
            }
        break;

        case DataDisplayMode::FloatingPt:
            if(!(idx % 2) && (idx + 1 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::FloatMode, ui->tableWidget);
                numEdit->setValue(makeFloat(_data[idx], _data[idx + 1], _writeParams.Order));
            }
        break;

        case DataDisplayMode::SwappedFP:
            if(!(idx % 2) && (idx + 1 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::FloatMode, ui->tableWidget);
                numEdit->setValue(makeFloat(_data[idx + 1], _data[idx], _writeParams.Order));
            }
        break;

        case DataDisplayMode::DblFloat:
            if(!(idx % 4) && (idx + 3 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::DoubleMode, ui->tableWidget);
                numEdit->setValue(makeDouble(_data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order));
            }
        break;

        case DataDisplayMode::SwappedDbl:
            if(!(idx % 4) && (idx + 3 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::DoubleMode, ui->tableWidget);
                numEdit->setValue(makeDouble(_data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order));
            }
        break;

        case DataDisplayMode::Int64:
            if(!(idx % 4) && (idx + 3 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::Int64Mode, ui->tableWidget);
                numEdit->setValue(makeInt64(_data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order));
            }
            break;

        case DataDisplayMode::SwappedInt64:
            if(!(idx % 4) && (idx + 3 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::Int64Mode, ui->tableWidget);
                numEdit->setValue(makeInt64(_data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order));
            }
            break;

        case DataDisplayMode::UInt64:
            if(!(idx % 4) && (idx + 3 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::UInt64Mode, ui->tableWidget);
                numEdit->setValue(makeUInt64(_data[idx], _data[idx + 1], _data[idx + 2], _data[idx + 3], _writeParams.Order));
            }
            break;

        case DataDisplayMode::SwappedUInt64:
            if(!(idx % 4) && (idx + 3 < _data.size()))
            {
                numEdit = new NumericLineEdit(NumericLineEdit::UInt64Mode, ui->tableWidget);
                numEdit->setValue(makeUInt64(_data[idx + 3], _data[idx + 2], _data[idx + 1], _data[idx], _writeParams.Order));
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
                auto numEdit = createNumEdit(idx);
                if(numEdit) ui->tableWidget->setCellWidget(i, j, numEdit);
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
