#include "modbuslimits.h"
#include "dialogsetuppresetdata.h"
#include "ui_dialogsetuppresetdata.h"

///
/// \brief DialogSetupPresetData::DialogSetupPresetData
/// \param params
/// \param pointType
/// \param parent
///
DialogSetupPresetData::DialogSetupPresetData(SetupPresetParams& params,  QModbusDataUnit::RegisterType pointType, QWidget *parent) :
     QFixedSizeDialog(parent)
    , ui(new Ui::DialogSetupPresetData)
    ,_params(params)
{
    ui->setupUi(this);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(params.ZeroBasedAddress));
    ui->lineEditNumberOfPoints->setInputRange(ModbusLimits::lengthRange());
    ui->lineEditAddress->setValue(params.PointAddress);
    ui->lineEditNumberOfPoints->setValue(params.Length);

    switch(pointType)
    {
        case QModbusDataUnit::Coils:
            setWindowTitle(tr("PRESET COILS"));
        break;
        case QModbusDataUnit::DiscreteInputs:
             setWindowTitle(tr("PRESET DISCRETE INPUTS"));
        break;
        case QModbusDataUnit::InputRegisters:
            setWindowTitle(tr("PRESET INPUT REGISTERS"));
        break;
        case QModbusDataUnit::HoldingRegisters:
            setWindowTitle(tr("PRESET HOLDING REGISTERS"));
        break;
        default:
        break;
    }

    ui->buttonBox->setFocus();
}

///
/// \brief DialogSetupPresetData::~DialogSetupPresetData
///
DialogSetupPresetData::~DialogSetupPresetData()
{
    delete ui;
}


///
/// \brief DialogSetupPresetData::accept
///
void DialogSetupPresetData::accept()
{
    _params.PointAddress = ui->lineEditAddress->value<int>();
    _params.Length = ui->lineEditNumberOfPoints->value<int>();
    QFixedSizeDialog::accept();
}
