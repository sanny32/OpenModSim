#ifndef DIALOGWRITEREGISTER_H
#define DIALOGWRITEREGISTER_H

#include "enums.h"
#include "modbuswriteparams.h"
#include "modbussimulationparams.h"
#include "qfixedsizedialog.h"
#include "checkablegroupbox.h"
#include "bitpatterncontrol.h"

class DataSimulator;

namespace Ui {
class DialogWriteRegister;
}

///
/// \brief The DialogWriteRegister class
///
class DialogWriteRegister : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogWriteRegister(ModbusWriteParams& params, QModbusDataUnit::RegisterType type,
                                 bool hexAddress, DataSimulator* dataSimulator = nullptr, QWidget *parent = nullptr);
    ~DialogWriteRegister();

    void accept() override;

private slots:
    void on_pushButtonSimulation_clicked();
    void on_lineEditAddress_valueChanged(const QVariant& value);
    void on_lineEditNode_valueChanged(const QVariant& value);

private:
    void updateSimulationButton();
    void updateValue();

private:
    Ui::DialogWriteRegister *ui;

private:
    ModbusWriteParams& _writeParams;
    QModbusDataUnit::RegisterType _type;
    ModbusSimulationParams _simParams;
    DataSimulator* _dataSimulator;
};

#endif // DIALOGWRITEREGISTER_H
