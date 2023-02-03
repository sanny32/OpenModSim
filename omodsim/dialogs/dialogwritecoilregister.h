#ifndef DIALOGWRITECOILREGISTER_H
#define DIALOGWRITECOILREGISTER_H

#include "qfixedsizedialog.h"
#include "modbuswriteparams.h"

namespace Ui {
class DialogWriteCoilRegister;
}

///
/// \brief The DialogWriteCoilRegister class
///
class DialogWriteCoilRegister : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogWriteCoilRegister(ModbusWriteParams& params, QWidget *parent = nullptr);
    ~DialogWriteCoilRegister();

    void accept() override;

private slots:
    void on_pushButtonSimulation_clicked();

private:
    Ui::DialogWriteCoilRegister *ui;

private:
    ModbusWriteParams& _writeParams;
};

#endif // DIALOGWRITECOILREGISTER_H
