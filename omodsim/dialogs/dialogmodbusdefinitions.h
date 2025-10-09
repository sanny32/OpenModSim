#ifndef DIALOGMODBUSDEFINITIONS_H
#define DIALOGMODBUSDEFINITIONS_H

#include <QDialog>
#include <QListWidgetItem>
#include "qfixedsizedialog.h"
#include "modbusmultiserver.h"

namespace Ui {
class DialogModbusDefinitions;
}

class DialogModbusDefinitions : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogModbusDefinitions(ModbusMultiServer& srv, QWidget *parent = nullptr);
    ~DialogModbusDefinitions();

private slots:
    void on_listServers_currentRowChanged(int row);

private:
    Ui::DialogModbusDefinitions *ui;
    ModbusMultiServer& _mbMultiServer;
};

#endif // DIALOGMODBUSDEFINITIONS_H
