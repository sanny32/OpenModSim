#include <QLoggingCategory>
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
    ,_script(new Script)
    ,_storage(new Storage)
    ,_server(nullptr)
    ,_logger(nullptr)
{
    ui->setupUi(this);

    _jsEngine.installExtensions(QJSEngine::ConsoleExtension);
    _logger = QSharedPointer<ConsoleLogger>(new ConsoleLogger(ui->console));

    _jsEngine.globalObject().setProperty("Script", _jsEngine.newQObject(_script.get()));
    _jsEngine.globalObject().setProperty("Storage", _jsEngine.newQObject(_storage.get()));

    connect(&_timer, &QTimer::timeout, this, &ScriptControl::executeScript);
    connect(_script.get(), &Script::stopped, this, &ScriptControl::stopScript);
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
    _jsEngine.globalObject().setProperty("Server", _jsEngine.newQObject(_server.get()));
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
    _logger->clear();
    _scriptCode = script();
    _jsEngine.setInterrupted(false);

    executeScript();
    switch(_runMode)
    {
        case RunMode::Once:
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
    const auto res = _jsEngine.evaluate(_scriptCode);
    if(res.isError() && !_jsEngine.isInterrupted())
    {
        qCritical(js).noquote().nospace() << res.toString() << " (line " << res.property("lineNumber").toInt() << ")";
        stopScript();
    }
 }
