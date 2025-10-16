#ifndef DIALOGMODBUSDEFINITIONS_H
#define DIALOGMODBUSDEFINITIONS_H

#include <QDialog>
#include <QListWidgetItem>
#include <QAbstractButton>
#include "qfixedsizedialog.h"
#include "modbusdefinitions.h"
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

    void accept() override;

private slots:
    void on_listServers_currentRowChanged(int row);
    void on_buttonBox_clicked(QAbstractButton* btn);

private:
    void apply();
    void updateModbusDefinitions(const ModbusDefinitions& md);

private:
    Ui::DialogModbusDefinitions *ui;
    ModbusMultiServer& _mbMultiServer;
};

#endif // DIALOGMODBUSDEFINITIONS_H
