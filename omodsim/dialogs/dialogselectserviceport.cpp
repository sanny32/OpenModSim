#include "dialogselectserviceport.h"
#include "ui_dialogselectserviceport.h"

///
/// \brief DialogSelectServicePort::DialogSelectServicePort
/// \param port
/// \param parent
///
DialogSelectServicePort::DialogSelectServicePort(quint16& port, QWidget *parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogSelectServicePort)
    ,_port(port)
{
    ui->setupUi(this);
    ui->lineEditPort->setInputRange(0, 65535);
    ui->lineEditPort->setValue(_port);
}

///
/// \brief DialogSelectServicePort::~DialogSelectServicePort
///
DialogSelectServicePort::~DialogSelectServicePort()
{
    delete ui;
}

///
/// \brief DialogSelectServicePort::accept
///
void DialogSelectServicePort::accept()
{
    _port = ui->lineEditPort->value<int>();
    QFixedSizeDialog::accept();
}
