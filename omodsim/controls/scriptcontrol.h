#ifndef SCRIPTCONTROL_H
#define SCRIPTCONTROL_H

#include <QTimer>
#include <QJSEngine>
#include <QPlainTextEdit>
#include "console.h"
#include "script.h"
#include "storage.h"
#include "server.h"

namespace Ui {
class ScriptControl;
}

///
/// \brief The ScriptControl class
///
class ScriptControl : public QWidget
{
    Q_OBJECT

public:
    explicit ScriptControl(QWidget *parent = nullptr);
    ~ScriptControl();

    void setModbusMultiServer(ModbusMultiServer* server);
    void setByteOrder(const ByteOrder* order);

    bool isAutoCompleteEnabled() const;
    void enableAutoComplete(bool enable);

    QString script() const;
    void setScript(const QString& text);

    QString searchText() const;

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
    void runScript(RunMode mode, int interval = 0);
    void stopScript();

private slots:
    bool executeScript();

private:
    Ui::ScriptControl *ui;

    QTimer _timer;
    QJSEngine _jsEngine;
    QString _scriptCode;
    QString _searchText;

    QSharedPointer<Script> _script;
    QSharedPointer<Storage> _storage;
    QSharedPointer<Server> _server;
    QSharedPointer<console> _console;

    ByteOrder* _byteOrder = nullptr;
    ModbusMultiServer* _mbMultiServer = nullptr;
};

#endif // SCRIPTCONTROL_H
