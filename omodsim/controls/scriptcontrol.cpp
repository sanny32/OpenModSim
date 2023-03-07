#include "modbusmultiserver.h"
#include "scriptcontrol.h"
#include "ui_scriptcontrol.h"

///
/// \brief ModbusDevice::ModbusObject
///
ModbusDevice::ModbusDevice(ModbusMultiServer& server)
    :_mbMultiServer(server)
{
}

///
/// \brief ModbusDevice::readValue
/// \param address
/// \return
///
quint16 ModbusDevice::readValue(quint16 address)
{
    return 77;
}

///
/// \brief ModbusDevice::writeValue
/// \param address
/// \param value
/// \return
///
bool ModbusDevice::writeValue(quint16 address, quint16 value)
{
    return false;
}

///
/// \brief ScriptControl::ScriptControl
/// \param parent
///
ScriptControl::ScriptControl(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ScriptControl)
{
    ui->setupUi(this);

    _toolBar = new QToolBar(this);
    _toolBar->addAction(ui->actionRun);

    ((QVBoxLayout*)layout())->insertWidget(0, _toolBar);

    setupJSEngine();
}

///
/// \brief ScriptControl::~ScriptControl
///
ScriptControl::~ScriptControl()
{
    delete ui;
    delete _mbDevice;
}

///
/// \brief ScriptControl::setupJSEngine
///
void ScriptControl::setupJSEngine()
{
    _jsEngine.globalObject().setProperty("console", _jsEngine.newQObject(ui->console));
    //_jsEngine.globalObject().setProperty("device", _jsEngine.newQObject(_mbDevice));
}

///
/// \brief ScriptControl::on_actionRun_triggered
///
void ScriptControl::on_actionRun_triggered()
{
    ui->console->clear();
    const auto res = _jsEngine.evaluate(ui->codeEditor->toPlainText());
    if(res.isError()) ui->console->log(res.toString());
}
