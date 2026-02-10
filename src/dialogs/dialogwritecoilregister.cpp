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
    ui->buttonBox->setFocus();

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
        layout->addWidget(textLabel);

        ui->pushButtonSimulation->setText(QString());
        ui->pushButtonSimulation->setLayout(layout);
        ui->pushButtonSimulation->setMinimumWidth(layout->sizeHint().width());
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
