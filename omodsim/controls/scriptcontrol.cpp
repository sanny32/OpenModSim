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
}

///
/// \brief ScriptControl::~ScriptControl
///
ScriptControl::~ScriptControl()
{
    delete ui;
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
/// \brief ScriptControl::script
/// \return
///
QString ScriptControl::script() const
{
    return ui->codeEditor->toPlainText();
}

///
/// \brief ScriptControl::setScript
/// \param text
///
void ScriptControl::setScript(const QString& text)
{
    ui->codeEditor->setPlainText(text);
}

///
/// \brief ScriptControl::runScript
///
void ScriptControl::runScript()
{
    _consoleObject->clear();
    const auto res = _jsEngine.evaluate(script());
    if(res.isError()) _consoleObject->log(res.toString());
}

///
/// \brief ScriptControl::stopScript
///
void ScriptControl::stopScript()
{

}
