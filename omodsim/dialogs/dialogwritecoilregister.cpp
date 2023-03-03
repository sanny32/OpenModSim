#include "modbuslimits.h"
#include "dialogcoilsimulation.h"
#include "dialogwritecoilregister.h"
#include "ui_dialogwritecoilregister.h"

///
/// \brief DialogWriteCoilRegister::DialogWriteCoilRegister
/// \param writeParams
/// \param simParams
/// \param parent
///
DialogWriteCoilRegister::DialogWriteCoilRegister(ModbusWriteParams& writeParams, ModbusSimulationParams& simParams, QWidget *parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogWriteCoilRegister)
    ,_writeParams(writeParams)
    ,_simParams(simParams)
{
    ui->setupUi(this);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange());
    ui->lineEditAddress->setValue(_writeParams.Address);
    ui->radioButtonOn->setChecked(_writeParams.Value.toBool());
    ui->radioButtonOff->setChecked(!_writeParams.Value.toBool());
    ui->buttonBox->setFocus();
}

///
/// \brief DialogWriteCoilRegister::~DialogWriteCoilRegister
///
DialogWriteCoilRegister::~DialogWriteCoilRegister()
{
    delete ui;
}

///
/// \brief DialogWriteCoilRegister::accept
///
void DialogWriteCoilRegister::accept()
{
    _writeParams.Address = ui->lineEditAddress->value<int>();
    _writeParams.Value = ui->radioButtonOn->isChecked();

    QFixedSizeDialog::accept();
}

///
/// \brief DialogWriteCoilRegister::on_pushButtonSimulation_clicked
///
void DialogWriteCoilRegister::on_pushButtonSimulation_clicked()
{
    DialogCoilSimulation dlg(_simParams, this);
    if(dlg.exec() == QDialog::Accepted) done(2);
}
