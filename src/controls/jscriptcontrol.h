#ifndef JSCRIPTCONTROL_H
#define JSCRIPTCONTROL_H

#include <QTimer>
#include <QJSEngine>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QXmlStreamWriter>
#include <QFrame>
#include "console.h"
#include "script.h"
#include "storage.h"
#include "server.h"

class FindReplaceBar;

namespace Ui {
class JScriptControl;
}

///
/// \brief The JScriptControl class
///
class JScriptControl : public QFrame
{
    Q_OBJECT

    friend QSettings& operator <<(QSettings& out, const JScriptControl* ctrl);
    friend QSettings& operator >>(QSettings& in, JScriptControl* ctrl);

    friend QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const JScriptControl* ctrl);
    friend QXmlStreamReader& operator >>(QXmlStreamReader& xml, JScriptControl* ctrl);

public:
    explicit JScriptControl(QWidget *parent = nullptr);
    ~JScriptControl();

    void setModbusMultiServer(ModbusMultiServer* server);
    void setByteOrder(const ByteOrder* order);
    void setAddressBase(AddressBase base);

    bool isAutoCompleteEnabled() const;
    void enableAutoComplete(bool enable);

    void setFont(const QFont& font);

    QString script() const;
    void setScript(const QString& text);
    QTextDocument* scriptDocument() const;
    void setScriptDocument(QTextDocument* document);

    QString searchText() const;

    int cursorPosition() const;
    void setCursorPosition(int pos);

    int scrollPosition() const;
    void setScrollPosition(int pos);

    bool isRunning() const;

    void setFocus();

    bool canUndo() const;
    bool canRedo() const;
    bool canPaste() const;

public slots:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();

    void search(const QString& text);

    void showFind();
    void showReplace();

    void setScriptSource(const QString& source);

    void runScript(RunMode mode, int interval = 0);
    void stopScript();

signals:
    void scriptStopped();
    void helpContext(const QString& helpKey);
    void consoleMessage(const QString& source, const QString& text, ConsoleOutput::MessageType type);

private slots:
    bool executeScript();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QJSValue newEnumObject(const QMetaObject& metaObj, const QString& enumName);

private:
    Ui::JScriptControl *ui;
    FindReplaceBar* _findReplaceBar;

    QTimer _timer;
    QJSEngine _jsEngine;
    QString _scriptCode;
    QString _searchText;

    QSharedPointer<Script> _script;
    QSharedPointer<Storage> _storage;
    QSharedPointer<Server> _server;
    QSharedPointer<console> _console;

    ByteOrder* _byteOrder = nullptr;
    AddressBase _addressBase = AddressBase::Base1;
    ModbusMultiServer* _mbMultiServer = nullptr;
    QString _scriptSource;
};

#endif // JSCRIPTCONTROL_H
