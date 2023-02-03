#include "dialogcoilsimulation.h"
#include "ui_dialogcoilsimulation.h"

///
/// \brief DialogCoilSimulation::DialogCoilSimulation
/// \param params
/// \param parent
///
DialogCoilSimulation::DialogCoilSimulation(ModbusSimulationParams& params, QWidget *parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogCoilSimulation)
    ,_params(params)
{
    ui->setupUi(this);
}

///
/// \brief DialogCoilSimulation::~DialogCoilSimulation
///
DialogCoilSimulation::~DialogCoilSimulation()
{
    delete ui;
}

///
/// \brief DialogCoilSimulation::accept
///
void DialogCoilSimulation::accept()
{
    QFixedSizeDialog::accept();
}
