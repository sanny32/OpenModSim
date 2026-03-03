#include <QFile>
#include <QMetaEnum>
#include "modbusmultiserver.h"
#include "findreplacebar.h"
#include "jscriptcontrol.h"
#include "ui_jscriptcontrol.h"

///
/// \brief JScriptControl::JScriptControl
/// \param parent
///
JScriptControl::JScriptControl(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::JScriptControl)
    ,_script(nullptr)
    ,_storage(nullptr)
    ,_server(nullptr)
    ,_console(nullptr)
{
    ui->setupUi(this);
    ui->codeEditor->moveCursor(QTextCursor::End);

    // Create find/replace bar as overlay on code editor (top-right corner)
    _findReplaceBar = new FindReplaceBar(ui->codeEditor);
    ui->codeEditor->installEventFilter(this);

    // Connect find/replace bar signals
    connect(_findReplaceBar, &FindReplaceBar::searchTextChanged, this, [this](const QString& text) {
        int count = ui->codeEditor->highlightAllMatches(text);
        _findReplaceBar->updateMatchCount(
            count > 0 ? ui->codeEditor->currentMatchIndex() + 1 : 0, count);
    });
    connect(_findReplaceBar, &FindReplaceBar::findNext, this, [this](const QString& text) {
        ui->codeEditor->findNext(text);
        _findReplaceBar->updateMatchCount(
            ui->codeEditor->currentMatchIndex() + 1, ui->codeEditor->totalMatchCount());
    });
    connect(_findReplaceBar, &FindReplaceBar::findPrevious, this, [this](const QString& text) {
        ui->codeEditor->findPrevious(text);
        _findReplaceBar->updateMatchCount(
            ui->codeEditor->currentMatchIndex() + 1, ui->codeEditor->totalMatchCount());
    });
    connect(_findReplaceBar, &FindReplaceBar::replaceRequested, this, [this](const QString& text, const QString& replacement) {
        ui->codeEditor->replaceCurrent(text, replacement);
        _findReplaceBar->updateMatchCount(
            ui->codeEditor->totalMatchCount() > 0 ? ui->codeEditor->currentMatchIndex() + 1 : 0,
            ui->codeEditor->totalMatchCount());
    });
    connect(_findReplaceBar, &FindReplaceBar::replaceAllRequested, this, [this](const QString& text, const QString& replacement) {
        int count = ui->codeEditor->replaceAll(text, replacement);
        _findReplaceBar->updateMatchCount(0, 0);
        Q_UNUSED(count);
    });
    connect(_findReplaceBar, &FindReplaceBar::closed, this, [this]() {
        ui->codeEditor->clearSearchHighlights();
        ui->codeEditor->setFocus();
    });

    auto helpfile = QApplication::applicationDirPath() + "/docs/jshelp.qhc";
    if(!QFile::exists(helpfile)){
        helpfile = QApplication::applicationDirPath() + "/../docs/jshelp.qhc";
    }
    ui->helpWidget->setHelp(helpfile);

    connect(&_timer, &QTimer::timeout, this, &JScriptControl::executeScript);
    connect(ui->codeEditor, &JSCodeEditor::helpContext, this, &JScriptControl::showHelp);
    connect(ui->console, &ConsoleOutput::collapse, this, &JScriptControl::hideConsole);
    connect(ui->helpWidget, &HelpWidget::collapse, this, &JScriptControl::hideHelp);

    ui->helpWidget->installEventFilter(this);
    ui->console->installEventFilter(this);
}

///
/// \brief JScriptControl::~JScriptControl
///
JScriptControl::~JScriptControl()
{
    delete ui;
}

///
/// \brief JScriptControl::eventFilter
/// \param obj
/// \param event
/// \return
///
bool JScriptControl::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Resize)
    {
        if (obj == ui->helpWidget)
        {
            const QList<int> sizes = ui->verticalSplitter->sizes();
            if (sizes.size() == 2)
            {
                const bool visible = sizes.at(1) > 0;
                if (ui->helpWidget->isVisible() != visible)
                    ui->helpWidget->setVisible(visible);
            }
        }

        if (obj == ui->console)
        {
            const QList<int> sizes = ui->horizontalSplitter->sizes();
            if (sizes.size() == 2)
            {
                const bool visible = sizes.at(1) > 0;
                if (ui->console->isVisible() != visible)
                    ui->console->setVisible(visible);
            }
        }

        if (obj == ui->codeEditor)
        {
            _findReplaceBar->updatePosition();
        }
    }

    return QWidget::eventFilter(obj, event);
}

///
/// \brief JScriptControl::isHelpVisible
/// \return
///
bool JScriptControl::isHelpVisible() const
{
    return ui->helpWidget->isVisible();
}

///
/// \brief JScriptControl::isConsoleOutputVisible
/// \return
///
bool JScriptControl::isConsoleVisible() const
{
    return ui->console->isVisible();
}

///
/// \brief JScriptControl::setModbusMultiServer
/// \param server
///
void JScriptControl::setModbusMultiServer(ModbusMultiServer* server)
{
    _mbMultiServer = server;
}

///
/// \brief JScriptControl::setByteOrder
/// \param order
///
void JScriptControl::setByteOrder(const ByteOrder* order)
{
    _byteOrder = const_cast<ByteOrder*>(order);
}

///
/// \brief JScriptControl::setAddressBase
/// \param base
///
void JScriptControl::setAddressBase(AddressBase base)
{
    _addressBase = base;
}

///
/// \brief JScriptControl::isAutoCompleteEnabled
/// \return
///
bool JScriptControl::isAutoCompleteEnabled() const
{
    return ui->codeEditor->isAutoCompleteEnabled();
}

///
/// \brief JScriptControl::enableAutoComplete
/// \param enable
///
void JScriptControl::enableAutoComplete(bool enable)
{
    ui->codeEditor->enableAutoComplete(enable);
}

///
/// \brief JScriptControl::script
/// \return
///
QString JScriptControl::script() const
{
    return ui->codeEditor->toPlainText();
}

///
/// \brief JScriptControl::setScript
/// \param text
///
void JScriptControl::setScript(const QString& text)
{
    ui->codeEditor->setPlainText(text);
    ui->codeEditor->moveCursor(QTextCursor::End);
}

///
/// \brief JScriptControl::searchText
/// \return
///
QString JScriptControl::searchText() const
{
    return _searchText;
}

///
/// \brief JScriptControl::undo
///
void JScriptControl::undo()
{
    ui->codeEditor->undo();
}

///
/// \brief JScriptControl::redo
///
void JScriptControl::redo()
{
    ui->codeEditor->redo();
}

///
/// \brief JScriptControl::cut
///
void JScriptControl::cut()
{
    ui->codeEditor->cut();
}

///
/// \brief JScriptControl::copy
///
void JScriptControl::copy()
{
    ui->codeEditor->copy();
}

///
/// \brief JScriptControl::paste
///
void JScriptControl::paste()
{
    ui->codeEditor->paste();
}

///
/// \brief JScriptControl::selectAll
///
void JScriptControl::selectAll()
{
    ui->codeEditor->selectAll();
}

///
/// \brief JScriptControl::search
/// \param text
///
void JScriptControl::search(const QString& text)
{
    _searchText = text;
    ui->codeEditor->search(text);
}

///
/// \brief JScriptControl::showFind
///
void JScriptControl::showFind()
{
    auto cursor = ui->codeEditor->textCursor();
    _findReplaceBar->showFind(cursor.selectedText());
}

///
/// \brief JScriptControl::showReplace
///
void JScriptControl::showReplace()
{
    auto cursor = ui->codeEditor->textCursor();
    _findReplaceBar->showReplace(cursor.selectedText());
}

///
/// \brief JScriptControl::isRunning
///
bool JScriptControl::isRunning() const
{
   return _timer.isActive();
}

///
/// \brief JScriptControl::setFocus
///
void JScriptControl::setFocus()
{
    ui->codeEditor->setFocus();
}

///
/// \brief JScriptControl::canUndo
/// \return
///
bool JScriptControl::canUndo() const
{
    return true;
}

///
/// \brief JScriptControl::canRedo
/// \return
///
bool JScriptControl::canRedo() const
{
    return true;
}

///
/// \brief JScriptControl::canPaste
/// \return
///
bool JScriptControl::canPaste() const
{
    return ui->codeEditor->canPaste();
}

///
/// \brief JScriptControl::newEnumObject
/// \param metaObj
/// \param enumName
/// \return
///
QJSValue JScriptControl::newEnumObject(const QMetaObject& metaObj, const QString& enumName)
{
    QMetaEnum baseEnum = metaObj.enumerator(metaObj.indexOfEnumerator(enumName.toUtf8()));

    QJSValue jsObj = _jsEngine.newObject();
    for (int i = 0; i < baseEnum.keyCount(); ++i) {
        jsObj.setProperty(baseEnum.key(i), baseEnum.value(i));
    }

    return jsObj;
}

///
/// \brief JScriptControl::runScript
/// \param interval
///
void JScriptControl::runScript(RunMode mode, int interval)
{
    _scriptCode = script();

    _storage = QSharedPointer<Storage>(new Storage);
    _server = QSharedPointer<Server>(new Server(_mbMultiServer, _byteOrder, _addressBase, &_jsEngine));
    _script = QSharedPointer<Script>(new Script(interval));
    _console = QSharedPointer<console>(new console(ui->console));
    connect(_script.get(), &Script::stopped, this, &JScriptControl::stopScript, Qt::QueuedConnection);

    _jsEngine.globalObject().setProperty("Storage", _jsEngine.newQObject(_storage.get()));
    _jsEngine.globalObject().setProperty("Script",  _jsEngine.newQObject(_script.get()));
    _jsEngine.globalObject().setProperty("Server",  _jsEngine.newQObject(_server.get()));
    _jsEngine.globalObject().setProperty("console", _jsEngine.newQObject(_console.get()));
    _jsEngine.globalObject().setProperty("Register", _jsEngine.newQMetaObject(&Register::staticMetaObject));
    _jsEngine.globalObject().setProperty("AddressBase", newEnumObject(Address::staticMetaObject, "Base"));
    _jsEngine.globalObject().setProperty("AddressSpace", newEnumObject(Address::staticMetaObject, "Space"));
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
/// \brief JScriptControl::stopScript
///
void JScriptControl::stopScript()
{
    _timer.stop();

    if(_script != nullptr)
        disconnect(_script.get(), &Script::stopped, this, &JScriptControl::stopScript);

    _jsEngine.setInterrupted(true);
    _jsEngine.globalObject().deleteProperty("Storage");
    _jsEngine.globalObject().deleteProperty("Script");
    _jsEngine.globalObject().deleteProperty("Server");
    _jsEngine.globalObject().deleteProperty("console");
    _jsEngine.globalObject().deleteProperty("Register");
    _jsEngine.globalObject().deleteProperty("AddressBase");
    _jsEngine.globalObject().deleteProperty("AddressSpace");

    _storage = nullptr;
    _server = nullptr;
    _script = nullptr;
    _console = nullptr;
}

///
/// \brief JScriptControl::showHelp
/// \param helpKey
///
void JScriptControl::showHelp(const QString& helpKey)
{
    if(ui->verticalSplitter->sizes().at(1) == 0)
    {
        const int w = size().width();
        ui->verticalSplitter->setSizes(QList<int>() << w * 5 / 7 << w * 2 / 7);
        ui->helpWidget->setVisible(true);
    }

    if(!helpKey.isEmpty()) {
        ui->helpWidget->showHelp(helpKey);
    }
}

///
/// \brief JScriptControl::hideHelp
///
void JScriptControl::hideHelp()
{
    ui->verticalSplitter->setSizes(QList<int>() << 1 << 0);
}

///
/// \brief JScriptControl::showConsole
///
void JScriptControl::showConsole()
{
    if(ui->horizontalSplitter->sizes().at(1) == 0)
    {
        const int h = size().height();
        ui->horizontalSplitter->setSizes(QList<int>() << h * 6 / 7 << h * 1 / 7);
        ui->console->setVisible(true);
    }
}

///
/// \brief JScriptControl::hideConsole
///
void JScriptControl::hideConsole()
{
    ui->horizontalSplitter->setSizes(QList<int>() << 1 << 0);
}

///
/// \brief JScriptControl::executeScript
///
bool JScriptControl::executeScript()
{
    const auto res = _script->run(_jsEngine, _scriptCode);
    if(res.isError() && !_jsEngine.isInterrupted())
    {
        showConsole();
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
QSettings& operator <<(QSettings& out, const JScriptControl* ctrl)
{
    out.setValue("ScriptControl/Script", ctrl->script().toUtf8().toBase64());
    out.setValue("ScriptControl/VSplitter", ctrl->ui->verticalSplitter->saveState());
    out.setValue("ScriptControl/HSplitter", ctrl->ui->horizontalSplitter->saveState());
    out.setValue("ScriptControl/HelpVisible",    ctrl->isHelpVisible());
    out.setValue("ScriptControl/ConsoleVisible", ctrl->isConsoleVisible());
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param ctrl
/// \return
///
QSettings& operator >>(QSettings& in, JScriptControl* ctrl)
{
    const auto script = QByteArray::fromBase64(in.value("ScriptControl/Script").toString().toUtf8());
    ctrl->setScript(script);

    ctrl->ui->helpWidget->setVisible(in.value("ScriptControl/HelpVisible", true).toBool());
    ctrl->ui->console->setVisible(in.value("ScriptControl/ConsoleVisible", true).toBool());

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
QDataStream& operator <<(QDataStream& out, const JScriptControl* ctrl)
{
    QMap<QString, QVariant> m;
    m["script"]        = ctrl->script();
    m["vsplitter"]     = ctrl->ui->verticalSplitter->saveState();
    m["hsplitter"]     = ctrl->ui->horizontalSplitter->saveState();
    m["helpVisible"]   = ctrl->isHelpVisible();
    m["consoleVisible"]= ctrl->isConsoleVisible();

    out << m;
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param ctrl
/// \return
///
QDataStream& operator >>(QDataStream& in, JScriptControl* ctrl)
{
    QMap<QString, QVariant> m;
    in >> m;

    ctrl->setScript(m["script"].toString());

    ctrl->ui->helpWidget->setVisible(m.value("helpVisible", true).toBool());
    ctrl->ui->console->setVisible(m.value("consoleVisible", true).toBool());

    const auto vstate = m["vsplitter"].toByteArray();
    if(!vstate.isEmpty()) ctrl->ui->verticalSplitter->restoreState(vstate);

    const auto hstate = m["hsplitter"].toByteArray();
    if(!hstate.isEmpty()) ctrl->ui->horizontalSplitter->restoreState(hstate);

    return in;
}

///
/// \brief operator <<
/// \param xml
/// \param ctrl
/// \return
///
QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const JScriptControl* ctrl)
{
    if(!ctrl) return xml;

    xml.writeStartElement("JScriptControl");

    xml.writeStartElement("Script");
    xml.writeCDATA(ctrl->script());
    xml.writeEndElement();

    xml.writeStartElement("VerticalSplitter");
    xml.writeAttribute("Visible", boolToString(ctrl->isHelpVisible()));
    xml.writeCharacters(ctrl->ui->verticalSplitter->saveState().toBase64());
    xml.writeEndElement();

    xml.writeStartElement("HorizontalSplitter");
    xml.writeAttribute("Visible", boolToString(ctrl->isConsoleVisible()));
    xml.writeCharacters(ctrl->ui->horizontalSplitter->saveState().toBase64());
    xml.writeEndElement();

    xml.writeStartElement("AutoComplete");
    xml.writeAttribute("Enabled", boolToString(ctrl->isAutoCompleteEnabled()));
    xml.writeEndElement();

    xml.writeEndElement(); // JScriptControl

    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param ctrl
/// \return
///
QXmlStreamReader& operator>>(QXmlStreamReader& xml, JScriptControl* ctrl)
{
    if (!ctrl)
        return xml;

    if (xml.isStartElement() && xml.name() == QLatin1String("JScriptControl")) {
        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("Script")) {
                const QString scriptText = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                ctrl->setScript(scriptText);
            }
            else if (xml.name() == QLatin1String("VerticalSplitter")) {
                const QXmlStreamAttributes attrs = xml.attributes();
                const bool visible = !attrs.hasAttribute("Visible") || stringToBool(attrs.value("Visible").toString());
                ctrl->ui->helpWidget->setVisible(visible);
                const QByteArray state = QByteArray::fromBase64(xml.readElementText().toLatin1());
                if (!state.isEmpty()) ctrl->ui->verticalSplitter->restoreState(state);
            }
            else if (xml.name() == QLatin1String("HorizontalSplitter")) {
                const QXmlStreamAttributes attrs = xml.attributes();
                const bool visible = !attrs.hasAttribute("Visible") || stringToBool(attrs.value("Visible").toString());
                ctrl->ui->console->setVisible(visible);
                const QByteArray state = QByteArray::fromBase64(xml.readElementText().toLatin1());
                if (!state.isEmpty()) ctrl->ui->horizontalSplitter->restoreState(state);
            }
            else if (xml.name() == QLatin1String("AutoComplete")) {
                const QXmlStreamAttributes attrs = xml.attributes();
                if (attrs.hasAttribute("Enabled")) {
                    const bool enabled = stringToBool(attrs.value("Enabled").toString());
                    ctrl->enableAutoComplete(enabled);
                }
                xml.skipCurrentElement();
            }
            else {
                xml.skipCurrentElement();
            }
        }
    }

    return xml;
}

