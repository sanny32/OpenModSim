#include "scriptcontrol.h"
#include "ui_scriptcontrol.h"

///
/// \brief ModbusObject::ModbusObject
///
ModbusObject::ModbusObject()
{
}

///
/// \brief ModbusObject::readValue
/// \param address
/// \return
///
quint16 ModbusObject::readValue(quint16 address)
{
    return 77;
}

///
/// \brief ModbusObject::writeValue
/// \param address
/// \param value
/// \return
///
bool ModbusObject::writeValue(quint16 address, quint16 value)
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
    ,_mbObject(new ModbusObject)
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
}

///
/// \brief ScriptControl::setupJSEngine
///
void ScriptControl::setupJSEngine()
{
    _jsEngine.globalObject().setProperty("console", _jsEngine.newQObject(ui->console));
    _jsEngine.globalObject().setProperty("device", _jsEngine.newQObject(_mbObject));
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
