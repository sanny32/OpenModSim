#include "dialogsetupserialport.h"
#include "ui_dialogsetupserialport.h"

///
/// \brief DialogSetupSerialPort::DialogSetupSerialPort
/// \param params
/// \param parent
///
DialogSetupSerialPort::DialogSetupSerialPort(SerialConnectionParams& params, QWidget *parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogSetupSerialPort)
    ,_serialParams(params)
{
    ui->setupUi(this);
    ui->comboBoxFlowControl->setCurrentIndex(-1);
    ui->comboBoxBaudRate->setCurrentValue(_serialParams.BaudRate);
    ui->comboBoxWordLength->setCurrentValue(_serialParams.WordLength);
    ui->comboBoxParity->setCurrentParity(_serialParams.Parity);
    ui->comboBoxStopBits->setCurrentValue(_serialParams.StopBits);
    ui->comboBoxFlowControl->setCurrentFlowControl(_serialParams.FlowControl);
    ui->comboBoxDTRControl->setCurrentValue(_serialParams.SetDTR);
    ui->comboBoxRTSControl->setCurrentValue(_serialParams.SetRTS);
    setWindowTitle(QString("%1 %2").arg(windowTitle(), _serialParams.PortName));
}

///
/// \brief DialogSetupSerialPort::~DialogSetupSerialPort
///
DialogSetupSerialPort::~DialogSetupSerialPort()
{
    delete ui;
}

///
/// \brief DialogSetupSerialPort::accept
///
void DialogSetupSerialPort::accept()
{
    _serialParams.BaudRate = (QSerialPort::BaudRate)ui->comboBoxBaudRate->currentValue();
    _serialParams.WordLength = (QSerialPort::DataBits)ui->comboBoxWordLength->currentValue();
    _serialParams.Parity = ui->comboBoxParity->currentParity();
    _serialParams.StopBits = (QSerialPort::StopBits)ui->comboBoxStopBits->currentValue();
    _serialParams.FlowControl = ui->comboBoxFlowControl->currentFlowControl();
    _serialParams.SetDTR = ui->comboBoxDTRControl->currentValue();
    _serialParams.SetRTS = ui->comboBoxRTSControl->currentValue();
    QFixedSizeDialog::accept();
}
