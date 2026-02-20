#include "modbuslimits.h"
#include "dialogcoilsimulation.h"
#include "dialogwritecoilregister.h"
#include "ui_dialogwritecoilregister.h"

///
/// \brief DialogWriteCoilRegister::DialogWriteCoilRegister
/// \param writeParams
/// \param simParams
/// \param hexAddress
/// \param parent
///
DialogWriteCoilRegister::DialogWriteCoilRegister(ModbusWriteParams& writeParams, ModbusSimulationParams& simParams, bool hexAddress, QWidget *parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogWriteCoilRegister)
    ,_writeParams(writeParams)
    ,_simParams(simParams)
{
    ui->setupUi(this);
    ui->lineEditAddress->setInputMode(hexAddress ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(writeParams.AddrSpace, writeParams.ZeroBasedAddress));
    ui->lineEditAddress->setValue(_writeParams.Address);
    ui->radioButtonOn->setChecked(_writeParams.Value.toBool());
    ui->radioButtonOff->setChecked(!_writeParams.Value.toBool());

    switch(simParams.Mode)
    {
        case SimulationMode::Disabled:
            ui->pushButtonSimulation->setText(tr("Auto Simulation: ON"));
            ui->pushButtonSimulation->setEnabled(false);
        break;

        case SimulationMode::Off:
        break;

        default:
            ui->pushButtonSimulation->setText(tr("Auto Simulation: ON"));
            ui->pushButtonSimulation->setStyleSheet(R"(
                    QPushButton {
                        color: white;
                        padding: 4px 12px;
                        background-color: #4CAF50;
                        border: 1px solid #3e8e41;
                        border-radius: 4px;
                    }
                    QPushButton:hover {
                        background-color: #45a049;
                    }
                    QPushButton:pressed {
                        background-color: #3e8e41;
                    }
                )");
        break;
    }

    if(ui->radioButtonOff->isChecked())
        ui->radioButtonOn->setFocus();
    else
        ui->radioButtonOff->setFocus();
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
