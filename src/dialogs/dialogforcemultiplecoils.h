#ifndef DIALOGFORCEMULTIPLECOILS_H
#define DIALOGFORCEMULTIPLECOILS_H

#include <QDialog>
#include <QTableWidgetItem>
#include <QModbusDataUnit>
#include "qadjustedsizedialog.h"
#include "modbuswriteparams.h"
#include "displaydefinition.h"

namespace Ui {
class DialogForceMultipleCoils;
}

class DialogForceMultipleCoils : public QAdjustedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogForceMultipleCoils(ModbusWriteParams& params, QModbusDataUnit::RegisterType type, int length, const DisplayDefinition& dd, QWidget *parent = nullptr);
    ~DialogForceMultipleCoils();

    void accept() override;

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_pushButton0_clicked();
    void on_pushButton1_clicked();
    void on_pushButtonRandom_clicked();
    void on_pushButtonImport_clicked();
    void on_pushButtonExport_clicked();
    void on_tableWidget_itemDoubleClicked(QTableWidgetItem *item);

private:
    void updateTableWidget();

private:
    Ui::DialogForceMultipleCoils *ui;
    QVector<quint16> _data;
    ModbusWriteParams& _writeParams;
    QModbusDataUnit::RegisterType _type;
    bool _hexAddress;
    bool _hexViewDeviceId = false;
    bool _hexViewLength   = false;
};

#endif // DIALOGFORCEMULTIPLECOILS_H
