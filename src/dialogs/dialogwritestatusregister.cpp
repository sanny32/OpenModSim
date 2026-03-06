#include "modbuslimits.h"
#include "modbusmultiserver.h"
#include "datasimulator.h"
#include "dialogcoilsimulation.h"
#include "dialogwritestatusregister.h"
#include "ui_dialogwritestatusregister.h"

///
/// \brief DialogWriteStatusRegister::DialogWriteStatusRegister
/// \param params
/// \param hexAddress
/// \param dataSimulator
/// \param parent
///
DialogWriteStatusRegister::DialogWriteStatusRegister(ModbusWriteParams& params, QModbusDataUnit::RegisterType type,
                                                    bool hexAddress, DataSimulator* dataSimulator, QWidget *parent)
    : QFixedSizeDialog(parent),
      ui(new Ui::DialogWriteStatusRegister)
    ,_writeParams(params)
    ,_type(type)
    ,_dataSimulator(dataSimulator)
{
    ui->setupUi(this);

    ui->lineEditNode->setLeadingZeroes(params.LeadingZeros);
    ui->lineEditNode->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditNode->setValue(params.DeviceId);

    ui->lineEditAddress->setLeadingZeroes(params.LeadingZeros);
    ui->lineEditAddress->setInputMode(hexAddress ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(params.AddrSpace, params.ZeroBasedAddress));
    ui->lineEditAddress->setValue(params.Address);

    ui->radioButtonOn->setChecked(params.Value.toBool());
    ui->radioButtonOff->setChecked(!params.Value.toBool());

    if(_dataSimulator != nullptr)
    {
        const int simAddr = params.Address - (params.ZeroBasedAddress ? 0 : 1);
        _simParams = _dataSimulator->simulationParams(params.DeviceId, _type, simAddr);
    }
    updateSimulationButton();

    if(ui->radioButtonOff->isChecked())
        ui->radioButtonOn->setFocus();
    else
        ui->radioButtonOff->setFocus();
}

///
/// \brief DialogWriteStatusRegister::~DialogWriteStatusRegister
///
DialogWriteStatusRegister::~DialogWriteStatusRegister()
{
    delete ui;
}

///
/// rief DialogWriteStatusRegister::changeEvent
///
void DialogWriteStatusRegister::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    QDialog::changeEvent(event);
}

///
/// \brief DialogWriteStatusRegister::accept
///
void DialogWriteStatusRegister::accept()
{
    _writeParams.Address = ui->lineEditAddress->value<int>();
    _writeParams.Value = ui->radioButtonOn->isChecked();
    _writeParams.DeviceId = ui->lineEditNode->value<int>();

    QFixedSizeDialog::accept();
}

///
/// \brief DialogWriteStatusRegister::updateSimulationButton
///
void DialogWriteStatusRegister::updateSimulationButton()
{
    switch(_simParams.Mode)
    {
        case SimulationMode::Disabled:
            ui->pushButtonSimulation->setEnabled(false);
            ui->pushButtonSimulation->setText(tr("Auto Simulation: ON"));
            ui->pushButtonSimulation->setStyleSheet("padding: 4px 12px;");
        break;

        case SimulationMode::Off:
            ui->pushButtonSimulation->setEnabled(true);
            ui->pushButtonSimulation->setText(tr("Auto Simulation: OFF"));
            ui->pushButtonSimulation->setStyleSheet("padding: 4px 12px;");
        break;

        default:
            ui->pushButtonSimulation->setEnabled(true);
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
}

///
/// \brief DialogWriteStatusRegister::updateValue
///
void DialogWriteStatusRegister::updateValue()
{
    ModbusMultiServer* srv = _writeParams.Server;
    if(srv == nullptr) return;

    const quint8 deviceId = ui->lineEditNode->value<int>();
    const int simAddr = ui->lineEditAddress->value<int>() - (_writeParams.ZeroBasedAddress ? 0 : 1);
    const bool value = srv->data(deviceId, _type, simAddr, 1).value(0) != 0;
    ui->radioButtonOn->setChecked(value);
    ui->radioButtonOff->setChecked(!value);
}

///
/// \brief DialogWriteStatusRegister::on_lineEditAddress_valueChanged
/// \param value
///
void DialogWriteStatusRegister::on_lineEditAddress_valueChanged(const QVariant& value)
{
    if(_dataSimulator != nullptr)
    {
        const quint8 deviceId = ui->lineEditNode->value<int>();
        const int simAddr = value.toInt() - (_writeParams.ZeroBasedAddress ? 0 : 1);
        _simParams = _dataSimulator->simulationParams(deviceId, _type, simAddr);
        updateSimulationButton();
    }
    updateValue();
}

///
/// \brief DialogWriteStatusRegister::on_lineEditNode_valueChanged
/// \param value
///
void DialogWriteStatusRegister::on_lineEditNode_valueChanged(const QVariant& value)
{
    if(_dataSimulator != nullptr)
    {
        const quint8 deviceId = value.toUInt();
        const int simAddr = ui->lineEditAddress->value<int>() - (_writeParams.ZeroBasedAddress ? 0 : 1);
        _simParams = _dataSimulator->simulationParams(deviceId, _type, simAddr);
        updateSimulationButton();
    }
    updateValue();
}

///
/// \brief DialogWriteStatusRegister::on_pushButtonSimulation_clicked
///
void DialogWriteStatusRegister::on_pushButtonSimulation_clicked()
{
    if(_dataSimulator == nullptr) return;

    DialogCoilSimulation dlg(_simParams, this);
    if(dlg.exec() == QDialog::Accepted)
    {
        const quint8 deviceId = ui->lineEditNode->value<int>();
        const int simAddr = ui->lineEditAddress->value<int>() - (_writeParams.ZeroBasedAddress ? 0 : 1);
        if(_simParams.Mode == SimulationMode::Off)
            _dataSimulator->stopSimulation(deviceId, _type, simAddr);
        else
            _dataSimulator->startSimulation(deviceId, _type, simAddr, _simParams);
        done(2);
    }
}
