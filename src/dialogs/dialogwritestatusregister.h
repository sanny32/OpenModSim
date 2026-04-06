#ifndef DIALOGWRITESTATUSREGISTER_H
#define DIALOGWRITESTATUSREGISTER_H

#include "qfixedsizedialog.h"
#include "modbuswriteparams.h"
#include "modbussimulationparams.h"

class DataSimulator;

namespace Ui {
class DialogWriteStatusRegister;
}

///
/// \brief The DialogWriteStatusRegister class
///
class DialogWriteStatusRegister : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogWriteStatusRegister(ModbusWriteParams& params, QModbusDataUnit::RegisterType type,
                                     bool hexAddress, DataSimulator* dataSimulator = nullptr, QWidget *parent = nullptr);
    ~DialogWriteStatusRegister();

    void accept() override;

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_pushButtonSimulation_clicked();
    void on_lineEditAddress_valueChanged(const QVariant& value);
    void on_lineEditNode_valueChanged(const QVariant& value);

private:
    void updateSimulationButton();
    void updateValue();

private:
    Ui::DialogWriteStatusRegister *ui;

private:
    ModbusWriteParams& _writeParams;
    QModbusDataUnit::RegisterType _type;
    ModbusSimulationParams _simParams;
    DataSimulator* _dataSimulator;
};

#endif // DIALOGWRITESTATUSREGISTER_H

