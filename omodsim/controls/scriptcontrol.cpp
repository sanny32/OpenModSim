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
    ,_script(nullptr)
    ,_storage(nullptr)
    ,_server(nullptr)
    ,_console(nullptr)
{
    ui->setupUi(this);
    ui->codeEditor->moveCursor(QTextCursor::End);
    ui->helpWidget->setHelp(QApplication::applicationDirPath() + "/docs/jshelp.qhc");

    connect(&_timer, &QTimer::timeout, this, &ScriptControl::executeScript);
    connect(ui->codeEditor, &JSCodeEditor::helpContext, this, &ScriptControl::showHelp);
}

///
/// \brief ScriptControl::~ScriptControl
///
ScriptControl::~ScriptControl()
{
    delete ui;
}

///
/// \brief ScriptControl::setModbusMultiServer
/// \param server
///
void ScriptControl::setModbusMultiServer(ModbusMultiServer* server)
{
    _mbMultiServer = server;
}

///
/// \brief ScriptControl::setByteOrder
/// \param order
///
void ScriptControl::setByteOrder(const ByteOrder* order)
{
    _byteOrder = const_cast<ByteOrder*>(order);
}

///
/// \brief ScriptControl::isAutoCompleteEnabled
/// \return
///
bool ScriptControl::isAutoCompleteEnabled() const
{
    return ui->codeEditor->isAutoCompleteEnabled();
}

///
/// \brief ScriptControl::enableAutoComplete
/// \param enable
///
void ScriptControl::enableAutoComplete(bool enable)
{
    ui->codeEditor->enableAutoComplete(enable);
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
    ui->codeEditor->moveCursor(QTextCursor::End);
}

///
/// \brief ScriptControl::searchText
/// \return
///
QString ScriptControl::searchText() const
{
    return _searchText;
}

///
/// \brief ScriptControl::undo
///
void ScriptControl::undo()
{
    ui->codeEditor->undo();
}

///
/// \brief ScriptControl::redo
///
void ScriptControl::redo()
{
    ui->codeEditor->redo();
}

///
/// \brief ScriptControl::cut
///
void ScriptControl::cut()
{
    ui->codeEditor->cut();
}

///
/// \brief ScriptControl::copy
///
void ScriptControl::copy()
{
    ui->codeEditor->copy();
}

///
/// \brief ScriptControl::paste
///
void ScriptControl::paste()
{
    ui->codeEditor->paste();
}

///
/// \brief ScriptControl::selectAll
///
void ScriptControl::selectAll()
{
    ui->codeEditor->selectAll();
}

///
/// \brief ScriptControl::search
/// \param text
///
void ScriptControl::search(const QString& text)
{
    _searchText = text;
    ui->codeEditor->search(text);
}

///
/// \brief ScriptControl::isRunning
///
bool ScriptControl::isRunning() const
{
   return _timer.isActive();
}

///
/// \brief ScriptControl::setFocus
///
void ScriptControl::setFocus()
{
    ui->codeEditor->setFocus();
}

///
/// \brief ScriptControl::canUndo
/// \return
///
bool ScriptControl::canUndo() const
{
    return true;
}

///
/// \brief ScriptControl::canRedo
/// \return
///
bool ScriptControl::canRedo() const
{
    return true;
}

///
/// \brief ScriptControl::canPaste
/// \return
///
bool ScriptControl::canPaste() const
{
    return ui->codeEditor->canPaste();
}

///
/// \brief ScriptControl::runScript
/// \param interval
///
void ScriptControl::runScript(RunMode mode, int interval)
{
    _scriptCode = script();

    _storage = QSharedPointer<Storage>(new Storage);
    _server = QSharedPointer<Server>(new Server(_mbMultiServer, _byteOrder));
    _script = QSharedPointer<Script>(new Script(interval));
    _console = QSharedPointer<console>(new console(ui->console));
    connect(_script.get(), &Script::stopped, this, &ScriptControl::stopScript, Qt::QueuedConnection);

    _jsEngine.globalObject().setProperty("Storage", _jsEngine.newQObject(_storage.get()));
    _jsEngine.globalObject().setProperty("Script",  _jsEngine.newQObject(_script.get()));
    _jsEngine.globalObject().setProperty("Server",  _jsEngine.newQObject(_server.get()));
    _jsEngine.globalObject().setProperty("console", _jsEngine.newQObject(_console.get()));
    _jsEngine.globalObject().setProperty("Register", _jsEngine.newQMetaObject(&Register::staticMetaObject));
    _jsEngine.setInterrupted(false);

    _console->clear();

    if(!executeScript())
        return;

    switch(mode)
    {
        case RunMode::Once:
            _script->stop();
        break;

        case RunMode::Periodically:
            _timer.start(interval);
        break;
    }
}

///
/// \brief ScriptControl::stopScript
///
void ScriptControl::stopScript()
{
    _timer.stop();

    if(_script != nullptr)
        disconnect(_script.get(), &Script::stopped, this, &ScriptControl::stopScript);

    _jsEngine.setInterrupted(true);
    _jsEngine.globalObject().deleteProperty("Storage");
    _jsEngine.globalObject().deleteProperty("Script");
    _jsEngine.globalObject().deleteProperty("Server");
    _jsEngine.globalObject().deleteProperty("console");
    _jsEngine.globalObject().deleteProperty("Register");

    _storage = nullptr;
    _server = nullptr;
    _script = nullptr;
    _console = nullptr;
}

///
/// \brief ScriptControl::showHelp
/// \param helpKey
///
void ScriptControl::showHelp(const QString& helpKey)
{
    if(ui->verticalSplitter->sizes().at(1) == 0)
    {
        const int w = size().width();
        ui->verticalSplitter->setSizes(QList<int>() << w * 5 / 7 << w * 2 / 7);
    }
    ui->helpWidget->showHelp(helpKey);
}

///
/// \brief ScriptControl::executeScript
///
bool ScriptControl::executeScript()
{
    const auto res = _script->run(_jsEngine, _scriptCode);
    if(res.isError() && !_jsEngine.isInterrupted())
    {
        _console->error(QString("%1 (line %2)").arg(res.toString(), res.property("lineNumber").toString()));
        _script->stop();
        return false;
    }
    return true;
}

///
/// \brief operator <<
/// \param out
/// \param frm
/// \return
///
QSettings& operator <<(QSettings& out, const ScriptControl* ctrl)
{
    out.setValue("ScriptControl/Script", ctrl->script().toUtf8().toBase64());
    out.setValue("ScriptControl/VSplitter", ctrl->ui->verticalSplitter->saveState());
    out.setValue("ScriptControl/HSplitter", ctrl->ui->horizontalSplitter->saveState());
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param ctrl
/// \return
///
QSettings& operator >>(QSettings& in, ScriptControl* ctrl)
{
    const auto script = QByteArray::fromBase64(in.value("ScriptControl/Script").toString().toUtf8());
    ctrl->setScript(script);

    const auto vstate = in.value("ScriptControl/VSplitter").toByteArray();
    if(!vstate.isEmpty()) ctrl->ui->verticalSplitter->restoreState(vstate);

    const auto hstate = in.value("ScriptControl/HSplitter").toByteArray();
    if(!hstate.isEmpty()) ctrl->ui->horizontalSplitter->restoreState(hstate);

    return in;
}

///
/// \brief operator <<
/// \param out
/// \param ctrl
/// \return
///
QDataStream& operator <<(QDataStream& out, const ScriptControl* ctrl)
{
    QMap<QString, QVariant> m;
    m["script"] = ctrl->script();
    m["vsplitter"] = ctrl->ui->verticalSplitter->saveState();
    m["hsplitter"] = ctrl->ui->horizontalSplitter->saveState();

    out << m;
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param ctrl
/// \return
///
QDataStream& operator >>(QDataStream& in, ScriptControl* ctrl)
{
    QMap<QString, QVariant> m;
    in >> m;

    ctrl->setScript(m["script"].toString());

    const auto vstate = m["vsplitter"].toByteArray();
    if(!vstate.isEmpty()) ctrl->ui->verticalSplitter->restoreState(vstate);

    const auto hstate = m["hsplitter"].toByteArray();
    if(!hstate.isEmpty()) ctrl->ui->horizontalSplitter->restoreState(hstate);

    return in;
}
