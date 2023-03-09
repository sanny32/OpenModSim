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
    ,_runMode(RunMode::Once)
    ,_scriptObject(nullptr)
    ,_consoleObject(nullptr)
    ,_mbServerObject(nullptr)
{
    ui->setupUi(this);
    connect(&_timer, &QTimer::timeout, this, &ScriptControl::executeScript);
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

    _scriptObject = QSharedPointer<ScriptObject>(new ScriptObject(this));
    connect(_scriptObject.get(), &ScriptObject::stopped, this, &ScriptControl::stopScript);

    _jsEngine.globalObject().setProperty("Script", _jsEngine.newQObject(_scriptObject.get()));
    _jsEngine.globalObject().setProperty("Server", _jsEngine.newQObject(_mbServerObject.get()));
    _jsEngine.globalObject().setProperty("console", _jsEngine.newQObject(_consoleObject.get()));
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
/// \brief ScriptControl::isRunning
///
bool ScriptControl::isRunning() const
{
   return _timer.isActive();
}

///
/// \brief ScriptControl::runMode
/// \return
///
RunMode ScriptControl::runMode() const
{
    return _runMode;
}

///
/// \brief ScriptControl::setRunMode
/// \param mode
///
void ScriptControl::setRunMode(RunMode mode)
{
    _runMode = mode;
}

///
/// \brief ScriptControl::runScript
/// \param interval
///
void ScriptControl::runScript(int interval)
{
    _consoleObject->clear();
    _jsEngine.setInterrupted(false);
    switch(_runMode)
    {
        case RunMode::Once:
            executeScript();
        break;

        case RunMode::Periodically:
            _timer.start(qMax(1000, interval));
        break;
    }
}

///
/// \brief ScriptControl::stopScript
///
void ScriptControl::stopScript()
{
    _timer.stop();
    _jsEngine.setInterrupted(true);
}

///
/// \brief ScriptControl::executeScript
///
void ScriptControl::executeScript()
{
    const auto res = _jsEngine.evaluate(script());
    if(res.isError() && !_jsEngine.isInterrupted())
        _consoleObject->log(res.toString());
}
