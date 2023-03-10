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
    ,_script(nullptr)
    ,_console(nullptr)
    ,_server(nullptr)
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
    _server = QSharedPointer<Server>(new Server(server));
    _console = QSharedPointer<Console>(new Console(ui->console));
    _storage = QSharedPointer<Storage>(new Storage());

    _script = QSharedPointer<Script>(new Script(&_jsEngine));
    connect(_script.get(), &Script::stopped, this, &ScriptControl::stopScript);

    _jsEngine.globalObject().setProperty("Script", _jsEngine.newQObject(_script.get()));
    _jsEngine.globalObject().setProperty("Server", _jsEngine.newQObject(_server.get()));
    _jsEngine.globalObject().setProperty("console", _jsEngine.newQObject(_console.get()));
    _jsEngine.globalObject().setProperty("Storage", _jsEngine.newQObject(_storage.get()));
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
    _console->clear();
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
    _storage->clear();
    _jsEngine.setInterrupted(true);
}

///
/// \brief ScriptControl::executeScript
///
void ScriptControl::executeScript()
{
    const auto res = _jsEngine.evaluate(script());
    if(res.isError() && !_jsEngine.isInterrupted())
    {
        _console->error(res.toString());
        stopScript();
    }
 }
