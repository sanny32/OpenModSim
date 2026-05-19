// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogcoilsimulation.h
/// \brief Declares the dialogcoilsimulation interfaces.
///

#ifndef DIALOGCOILSIMULATION_H
#define DIALOGCOILSIMULATION_H

#include "qfixedsizedialog.h"
#include "modbussimulationparams.h"

namespace Ui {
class DialogCoilSimulation;
}

class DialogCoilSimulation : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogCoilSimulation(ModbusSimulationParams& params, QWidget *parent = nullptr);
    ~DialogCoilSimulation();

    void accept() override;

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_checkBoxEnabled_toggled();
    void on_comboBoxSimulationType_currentIndexChanged(int);

private:
    Ui::DialogCoilSimulation *ui;

private:
    ModbusSimulationParams& _params;
};

#endif // DIALOGCOILSIMULATION_H

