#ifndef DIALOGFORCESTATUSREGISTERS_H
#define DIALOGFORCESTATUSREGISTERS_H

#include <QDialog>
#include <QTableWidgetItem>
#include <QModbusDataUnit>
#include "qadjustedsizedialog.h"
#include "modbuswriteparams.h"
#include "displaydefinition.h"

namespace Ui {
class DialogForceStatusRegisters;
}

class DialogForceStatusRegisters : public QAdjustedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogForceStatusRegisters(ModbusWriteParams& params, QModbusDataUnit::RegisterType type, int length, bool displayHexAddresses, QWidget *parent = nullptr);
    ~DialogForceStatusRegisters();

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
    void setupAddressControls(int length);
    void updateAddressSummary();
    void reloadDataFromServer();
    void updateTableWidget();

private:
    Ui::DialogForceStatusRegisters *ui;
    QVector<quint16> _data;
    ModbusWriteParams& _writeParams;
    QModbusDataUnit::RegisterType _type;
    bool _hexAddress;
};

#endif // DIALOGFORCESTATUSREGISTERS_H

