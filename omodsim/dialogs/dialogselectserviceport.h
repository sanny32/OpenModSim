#ifndef DIALOGSELECTSERVICEPORT_H
#define DIALOGSELECTSERVICEPORT_H

#include "qfixedsizedialog.h"
#include "connectiondetails.h"

namespace Ui {
class DialogSelectServicePort;
}

class DialogSelectServicePort : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogSelectServicePort(TcpConnectionParams& params, QWidget *parent = nullptr);
    ~DialogSelectServicePort();

    void accept() override;

private:
    Ui::DialogSelectServicePort *ui;
    TcpConnectionParams& _params;
};

#endif // DIALOGSELECTSERVICEPORT_H
