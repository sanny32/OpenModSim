#ifndef DIALOGSETUPSERIALPORT_H
#define DIALOGSETUPSERIALPORT_H

#include "qfixedsizedialog.h"
#include "connectiondetails.h"

namespace Ui {
class DialogSetupSerialPort;
}

class DialogSetupSerialPort : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogSetupSerialPort(SerialConnectionParams& params, QWidget *parent = nullptr);
    ~DialogSetupSerialPort();

    void accept() override;

private:
    Ui::DialogSetupSerialPort *ui;

private:
    SerialConnectionParams& _serialParams;
};

#endif // DIALOGSETUPSERIALPORT_H
