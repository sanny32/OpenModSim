#ifndef DIALOGSERVERSETTINGS_H
#define DIALOGSERVERSETTINGS_H

#include <QDialog>
#include <QListWidgetItem>
#include "qfixedsizedialog.h"
#include "modbusmultiserver.h"

namespace Ui {
class DialogServerSettings;
}

class DialogServerSettings : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogServerSettings(ModbusMultiServer& srv, QWidget *parent = nullptr);
    ~DialogServerSettings();

private slots:
    void on_listServers_currentRowChanged(int row);

private:
    Ui::DialogServerSettings *ui;
    ModbusMultiServer& _mbMultiServer;
};

#endif // DIALOGSERVERSETTINGS_H
