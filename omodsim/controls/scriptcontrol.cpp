#include "modbusmultiserver.h"
#include "scriptcontrol.h"
#include "ui_scriptcontrol.h"

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

}

///
/// \brief ScriptControl::~ScriptControl
///
ScriptControl::~ScriptControl()
{
    delete ui;
    delete _toolBar;
}

///
/// \brief ScriptControl::initJSEngine
///
void ScriptControl::initJSEngine(ModbusMultiServer& server)
{
    _mbServerObject = QSharedPointer<ModbusServerObject>(new ModbusServerObject(server));
    _consoleObject = QSharedPointer<ConsoleObject>(new ConsoleObject(ui->console->document()));
    _jsEngine.globalObject().setProperty("console", _jsEngine.newQObject(_consoleObject.get()));
    _jsEngine.globalObject().setProperty("Server", _jsEngine.newQObject(_mbServerObject.get()));
}

///
/// \brief ScriptControl::runScript
///
void ScriptControl::runScript()
{
    const auto res = _jsEngine.evaluate(ui->codeEditor->toPlainText());
    if(res.isError()) _consoleObject->log(res.toString());
}

///
/// \brief ScriptControl::on_actionRun_triggered
///
void ScriptControl::on_actionRun_triggered()
{
    _consoleObject->clear();
    runScript();
}
