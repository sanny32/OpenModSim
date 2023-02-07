#include <QPushButton>
#include "dialogautosimulation.h"
#include "ui_dialogautosimulation.h"

///
/// \brief DialogAutoSimulation::DialogAutoSimulation
/// \param params
/// \param parent
///
DialogAutoSimulation::DialogAutoSimulation(ModbusSimulationParams& params, QWidget *parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogAutoSimulation)
    ,_params(params)
{
    ui->setupUi(this);
    ui->checkBoxEnabled->setChecked(_params.Mode != SimulationMode::No);
    ui->comboBoxSimulationType->setup(QModbusDataUnit::HoldingRegisters);

    if(_params.Mode != SimulationMode::No)
        ui->comboBoxSimulationType->setCurrentSimulationMode(_params.Mode);
    else
        ui->comboBoxSimulationType->setCurrentIndex(0);

    ui->lineEditInterval->setInputRange(1, 60000);
    ui->lineEditInterval->setValue(_params.Interval);

    ui->lineEditLowLimit->setInputRange(0, 65535);
    ui->lineEditHighLimit->setInputRange(0, 65535);

    on_checkBoxEnabled_toggled();
}

///
/// \brief DialogAutoSimulation::~DialogAutoSimulation
///
DialogAutoSimulation::~DialogAutoSimulation()
{
    delete ui;
}

///
/// \brief DialogAutoSimulation::accept
///
void DialogAutoSimulation::accept()
{
    if(ui->checkBoxEnabled->isChecked())
    {
        _params.Mode = ui->comboBoxSimulationType->currentSimulationMode();
        _params.Interval = ui->lineEditInterval->value<int>();

        switch(_params.Mode)
        {
            case SimulationMode::Random:
                _params.RandomParams.Range = QRange<quint16>(ui->lineEditLowLimit->value<int>(),
                                                             ui->lineEditHighLimit->value<int>());
            break;

            case SimulationMode::Increment:
                _params.IncrementParams.Step = ui->lineEditStepValue->value<int>();
                _params.IncrementParams.Range = QRange<quint16>(ui->lineEditLowLimit->value<int>(),
                                                                ui->lineEditHighLimit->value<int>());
            break;

            case SimulationMode::Decrement:
                _params.DecrementParams.Step = ui->lineEditStepValue->value<int>();
                _params.DecrementParams.Range = QRange<quint16>(ui->lineEditLowLimit->value<int>(),
                                                                ui->lineEditHighLimit->value<int>());
            break;

            default:
            break;
        }

    }
    else
    {
        _params.Mode = SimulationMode::No;
    }

    QFixedSizeDialog::accept();
}

///
/// \brief DialogAutoSimulation::on_checkBoxEnabled_toggled
///
void DialogAutoSimulation::on_checkBoxEnabled_toggled()
{
    const bool enabled = ui->checkBoxEnabled->isChecked();
    const auto mode = ui->comboBoxSimulationType->currentSimulationMode();
    ui->labelSimulationType->setEnabled(enabled);
    ui->comboBoxSimulationType->setEnabled(enabled);
    ui->labelInterval->setEnabled(enabled);
    ui->lineEditInterval->setEnabled(enabled);
    ui->labelStepValue->setEnabled(enabled && mode != SimulationMode::Random);
    ui->lineEditStepValue->setEnabled(enabled && mode != SimulationMode::Random);
    ui->groupBoxSimulatioRange->setEnabled(enabled);
}

///
/// \brief DialogAutoSimulation::on_comboBoxSimulationType_currentIndexChanged
/// \param idx
///
void DialogAutoSimulation::on_comboBoxSimulationType_currentIndexChanged(int idx)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(idx != -1);
    updateLimits();
}

///
/// \brief DialogAutoSimulation::updateLimits
///
void DialogAutoSimulation::updateLimits()
{
    switch(ui->comboBoxSimulationType->currentSimulationMode())
    {
        case SimulationMode::Random:
            ui->labelStepValue->setEnabled(false);
            ui->lineEditStepValue->setEnabled(false);
            ui->lineEditLowLimit->setValue(_params.RandomParams.Range.from());
            ui->lineEditHighLimit->setValue(_params.RandomParams.Range.to());
        break;

        case SimulationMode::Increment:
            ui->labelStepValue->setEnabled(true);
            ui->lineEditStepValue->setEnabled(true);
            ui->lineEditStepValue->setValue(_params.IncrementParams.Step);
            ui->lineEditLowLimit->setValue(_params.IncrementParams.Range.from());
            ui->lineEditHighLimit->setValue(_params.IncrementParams.Range.to());
        break;

        case SimulationMode::Decrement:
            ui->labelStepValue->setEnabled(true);
            ui->lineEditStepValue->setEnabled(true);
            ui->lineEditStepValue->setValue(_params.DecrementParams.Step);
            ui->lineEditLowLimit->setValue(_params.DecrementParams.Range.from());
            ui->lineEditHighLimit->setValue(_params.DecrementParams.Range.to());
        break;

        default:
        break;
    }
}
