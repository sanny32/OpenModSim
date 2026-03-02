#include "modbuslimits.h"
#include "dialogsetuppresetdata.h"
#include "ui_dialogsetuppresetdata.h"

///
/// \brief DialogSetupPresetData::DialogSetupPresetData
/// \param params
/// \param pointType
/// \param hexAddress
/// \param parent
///
DialogSetupPresetData::DialogSetupPresetData(SetupPresetParams& params,  QModbusDataUnit::RegisterType pointType, bool hexAddress, QWidget *parent) :
     QFixedSizeDialog(parent)
    , ui(new Ui::DialogSetupPresetData)
    ,_params(params)
{
    ui->setupUi(this);
    ui->lineEditSlaveDevice->setLeadingZeroes(params.LeadingZeros);
    ui->lineEditSlaveDevice->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditSlaveDevice->setValue(params.DeviceId);

    ui->lineEditAddress->setLeadingZeroes(params.LeadingZeros);
    ui->lineEditAddress->setInputMode(hexAddress ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(params.AddrSpace, params.ZeroBasedAddress));
    ui->lineEditAddress->setValue(params.PointAddress);

    ui->lineEditNumberOfPoints->setInputRange(ModbusLimits::lengthRange(params.PointAddress, params.ZeroBasedAddress, params.AddrSpace));
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
/// \brief DialogSetupPresetData::on_lineEditAddress_valueChanged
///
void DialogSetupPresetData::on_lineEditAddress_valueChanged(const QVariant&)
{
    const int address = ui->lineEditAddress->value<int>();
    const auto lenRange = ModbusLimits::lengthRange(address, _params.ZeroBasedAddress, _params.AddrSpace);

    ui->lineEditNumberOfPoints->setInputRange(lenRange);
    if(ui->lineEditNumberOfPoints->value<int>() > lenRange.to()) {
        ui->lineEditNumberOfPoints->setValue(lenRange.to());
    }
}

///
/// \brief DialogSetupPresetData::accept
///
void DialogSetupPresetData::accept()
{
    _params.DeviceId = ui->lineEditSlaveDevice->value<int>();
    _params.PointAddress = ui->lineEditAddress->value<int>();
    _params.Length = ui->lineEditNumberOfPoints->value<int>();
    QFixedSizeDialog::accept();
}
