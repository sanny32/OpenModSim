#ifndef DIALOGSELECTSERVICEPORT_H
#define DIALOGSELECTSERVICEPORT_H

#include "qfixedsizedialog.h"

namespace Ui {
class DialogSelectServicePort;
}

class DialogSelectServicePort : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogSelectServicePort(quint16& port, QWidget *parent = nullptr);
    ~DialogSelectServicePort();

    void accept() override;

private:
    Ui::DialogSelectServicePort *ui;
    quint16& _port;
};

#endif // DIALOGSELECTSERVICEPORT_H
