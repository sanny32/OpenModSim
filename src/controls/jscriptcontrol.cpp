#include <QMetaEnum>
#include <QScrollBar>
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

    _findReplaceBar = new FindReplaceBar(this);
    ui->codeEditor->installEventFilter(this);

    connect(_findReplaceBar, &FindReplaceBar::searchTextChanged, this, [this](const QString& text) {
        ui->codeEditor->highlightAllMatches(text, _findReplaceBar->findFlags());
    });
    connect(_findReplaceBar, &FindReplaceBar::findNext, this, [this](const QString& text) {
        ui->codeEditor->findNext(text);
    });
    connect(_findReplaceBar, &FindReplaceBar::findPrevious, this, [this](const QString& text) {
        ui->codeEditor->findPrevious(text);
    });
    connect(_findReplaceBar, &FindReplaceBar::replaceRequested, this, [this](const QString& text, const QString& replacement) {
        ui->codeEditor->replaceCurrent(text, replacement, _findReplaceBar->findFlags());
    });
    connect(_findReplaceBar, &FindReplaceBar::replaceAllRequested, this, [this](const QString& text, const QString& replacement) {
        ui->codeEditor->replaceAll(text, replacement, _findReplaceBar->findFlags());
    });
    connect(_findReplaceBar, &FindReplaceBar::closed, this, [this]() {
        ui->codeEditor->clearSearchHighlights();
        ui->codeEditor->setFocus();
    });

    connect(&_timer, &QTimer::timeout, this, &JScriptControl::executeScript);
    connect(ui->codeEditor, &JSCodeEditor::helpContext, this, &JScriptControl::helpContext);

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
        if (obj == ui->codeEditor)
        {
            _findReplaceBar->updatePosition();
        }
    }

    return QWidget::eventFilter(obj, event);
}

///
/// \brief JScriptControl::setScriptSource
/// \param source
///
void JScriptControl::setScriptSource(const QString& source)
{
    _scriptSource = source;
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
/// \brief JScriptControl::setFont
/// \param font
///
void JScriptControl::setFont(const QFont& font)
{
    ui->codeEditor->setFont(font);
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
/// \brief JScriptControl::scriptDocument
/// \return
///
QTextDocument* JScriptControl::scriptDocument() const
{
    return ui->codeEditor->codeDocument();
}

///
/// \brief JScriptControl::setScriptDocument
/// \param document
///
void JScriptControl::setScriptDocument(QTextDocument* document)
{
    ui->codeEditor->setCodeDocument(document);
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
    _console = QSharedPointer<console>(new console());
    connect(_script.get(), &Script::stopped, this, &JScriptControl::stopScript, Qt::QueuedConnection);
    connect(_console.get(), &console::messageAdded, this, [this](const QString& text, ConsoleOutput::MessageType type) {
        emit consoleMessage(_scriptSource, text, type);
    });

    _jsEngine.globalObject().setProperty("Storage", _jsEngine.newQObject(_storage.get()));
    _jsEngine.globalObject().setProperty("Script",  _jsEngine.newQObject(_script.get()));
    _jsEngine.globalObject().setProperty("Server",  _jsEngine.newQObject(_server.get()));
    _jsEngine.globalObject().setProperty("console", _jsEngine.newQObject(_console.get()));
    _jsEngine.globalObject().setProperty("Register", _jsEngine.newQMetaObject(&Register::staticMetaObject));
    _jsEngine.globalObject().setProperty("AddressBase", newEnumObject(Address::staticMetaObject, "Base"));
    _jsEngine.globalObject().setProperty("AddressSpace", newEnumObject(Address::staticMetaObject, "Space"));
    _jsEngine.setInterrupted(false);



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

    emit scriptStopped();
}

///
/// \brief JScriptControl::executeScript
///
bool JScriptControl::executeScript()
{
    const auto res = _script->run(_jsEngine, _scriptCode);
    if(res.isError() && !_jsEngine.isInterrupted())
    {
        const QString errMsg = QString("%1 (line %2)").arg(res.toString(), res.property("lineNumber").toString());
        emit consoleMessage(_scriptSource, errMsg, ConsoleOutput::MessageType::Error);

        _script->stop();
        return false;
    }
    return true;
}

int JScriptControl::cursorPosition() const
{
    return ui->codeEditor->textCursor().position();
}

void JScriptControl::setCursorPosition(int pos)
{
    auto cursor = ui->codeEditor->textCursor();
    cursor.setPosition(qMin(pos, ui->codeEditor->document()->characterCount() - 1));
    ui->codeEditor->setTextCursor(cursor);
}

int JScriptControl::scrollPosition() const
{
    return ui->codeEditor->verticalScrollBar()->value();
}

void JScriptControl::setScrollPosition(int pos)
{
    ui->codeEditor->verticalScrollBar()->setValue(pos);
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
    out.setValue("ScriptControl/CursorPosition", ctrl->cursorPosition());
    out.setValue("ScriptControl/ScrollPosition", ctrl->scrollPosition());
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

    const int cursorPos = in.value("ScriptControl/CursorPosition", -1).toInt();
    if(cursorPos >= 0)
        ctrl->setCursorPosition(cursorPos);

    const int scrollPos = in.value("ScriptControl/ScrollPosition", -1).toInt();
    if(scrollPos >= 0)
        ctrl->setScrollPosition(scrollPos);

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
    xml.writeAttribute("CursorPosition", QString::number(ctrl->cursorPosition()));
    xml.writeAttribute("ScrollPosition", QString::number(ctrl->scrollPosition()));
    xml.writeCDATA(ctrl->script());
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
        int cursorPos = -1;
        int scrollPos = -1;
        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("Script")) {
                const QXmlStreamAttributes attrs = xml.attributes();
                if(attrs.hasAttribute("CursorPosition"))
                    cursorPos = attrs.value("CursorPosition").toInt();
                if(attrs.hasAttribute("ScrollPosition"))
                    scrollPos = attrs.value("ScrollPosition").toInt();
                const QString scriptText = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                ctrl->setScript(scriptText);
            }
            else if (xml.name() == QLatin1String("VerticalSplitter")) {
                xml.skipCurrentElement();
            }
            else if (xml.name() == QLatin1String("HorizontalSplitter")) {
                xml.skipCurrentElement(); // obsolete, console is now global
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

        if(cursorPos >= 0)
            ctrl->setCursorPosition(cursorPos);

        if(scrollPos >= 0)
            ctrl->setScrollPosition(scrollPos);
    }

    return xml;
}

