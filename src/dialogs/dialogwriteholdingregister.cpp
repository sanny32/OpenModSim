#include <float.h>
#include "modbuslimits.h"
#include "dialogautosimulation.h"
#include "dialogwriteholdingregister.h"
#include "ui_dialogwriteholdingregister.h"

///
/// \brief DialogWriteHoldingRegister::DialogWriteHoldingRegister
/// \param writeParams
/// \param simParams
/// \param hexAddress
/// \param parent
///
DialogWriteHoldingRegister::DialogWriteHoldingRegister(ModbusWriteParams& writeParams, ModbusSimulationParams& simParams, bool hexAddress, QWidget* parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogWriteHoldingRegister)
    ,_writeParams(writeParams)
    ,_simParams(simParams)
{
    ui->setupUi(this);
    ui->lineEditAddress->setInputMode(hexAddress ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(writeParams.AddrSpace, writeParams.ZeroBasedAddress));
    ui->lineEditAddress->setValue(_writeParams.Address);

    if(simParams.Mode != SimulationMode::No)
    {
        QLabel* iconLabel = new QLabel(ui->pushButtonSimulation);
        iconLabel->setPixmap(QIcon(":/res/pointGreen.png").pixmap(4, 4));
        iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        QLabel* textLabel = new QLabel(ui->pushButtonSimulation->text(), ui->pushButtonSimulation);
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        auto layout = new QHBoxLayout(ui->pushButtonSimulation);
        layout->setContentsMargins(4,0,4,0);
        layout->addWidget(iconLabel);
        layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        layout->addWidget(textLabel);
        layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        ui->pushButtonSimulation->setText(QString());
        ui->pushButtonSimulation->setLayout(layout);
    }

    switch(_writeParams.DisplayMode)
    {
        case DataDisplayMode::Binary:
        break;

        case DataDisplayMode::UInt16:
            ui->lineEditValue->setInputRange(0, USHRT_MAX);
            ui->lineEditValue->setValue(_writeParams.Value.toUInt());
        break;

        case DataDisplayMode::Int16:
            ui->lineEditValue->setInputRange(SHRT_MIN, SHRT_MAX);
            ui->lineEditValue->setValue(_writeParams.Value.toInt());
        break;

        case DataDisplayMode::Hex:
            ui->lineEditValue->setInputRange(0, USHRT_MAX);
            ui->labelValue->setText("Value, (HEX): ");
            ui->lineEditValue->setLeadingZeroes(true);
            ui->lineEditValue->setInputMode(NumericLineEdit::HexMode);
            ui->lineEditValue->setValue(_writeParams.Value.toUInt());
        break;

        case DataDisplayMode::Ansi:
            ui->labelValue->setText(tr("Value, (ANSI): "));
            ui->lineEditValue->setInputMode(NumericLineEdit::AnsiMode);
            ui->lineEditValue->setCodepage(_writeParams.Codepage);
            ui->lineEditValue->setValue(_writeParams.Value.toUInt());
        break;

        case DataDisplayMode::FloatingPt:
        case DataDisplayMode::SwappedFP:
            ui->lineEditValue->setInputMode(NumericLineEdit::FloatMode);
            ui->lineEditValue->setValue(_writeParams.Value.toFloat());
        break;

        case DataDisplayMode::DblFloat:
        case DataDisplayMode::SwappedDbl:
            ui->lineEditValue->setInputMode(NumericLineEdit::DoubleMode);
            ui->lineEditValue->setValue(_writeParams.Value.toDouble());
        break;
            
        case DataDisplayMode::Int32:
        case DataDisplayMode::SwappedInt32:
            ui->lineEditValue->setInputMode(NumericLineEdit::Int32Mode);
            ui->lineEditValue->setValue(_writeParams.Value.toInt());
        break;

        case DataDisplayMode::UInt32:
        case DataDisplayMode::SwappedUInt32:
            ui->lineEditValue->setInputMode(NumericLineEdit::UInt32Mode);
            ui->lineEditValue->setValue(_writeParams.Value.toUInt());
        break;

        case DataDisplayMode::Int64:
        case DataDisplayMode::SwappedInt64:
            ui->lineEditValue->setInputMode(NumericLineEdit::Int64Mode);
            ui->lineEditValue->setValue(_writeParams.Value.toLongLong());
        break;

        case DataDisplayMode::UInt64:
        case DataDisplayMode::SwappedUInt64:
            ui->lineEditValue->setInputMode(NumericLineEdit::UInt64Mode);
            ui->lineEditValue->setValue(_writeParams.Value.toULongLong());
        break;
    }
    ui->lineEditValue->setFocus();
}

///
/// \brief DialogWriteHoldingRegister::~DialogWriteHoldingRegister
///
DialogWriteHoldingRegister::~DialogWriteHoldingRegister()
{
    delete ui;
}

///
/// \brief DialogWriteHoldingRegister::accept
///
void DialogWriteHoldingRegister::accept()
{
    _writeParams.Address = ui->lineEditAddress->value<int>();
    _writeParams.Value = ui->lineEditValue->value<QVariant>();

    QFixedSizeDialog::accept();
}

///
/// \brief DialogWriteHoldingRegister::on_pushButtonSimulation_clicked
///
void DialogWriteHoldingRegister::on_pushButtonSimulation_clicked()
{
    DialogAutoSimulation dlg(_writeParams.DisplayMode, _simParams, this);
    if(dlg.exec() == QDialog::Accepted) done(2);
}
