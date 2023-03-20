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

    void initJSEngine(ModbusMultiServer& server, const ByteOrder& order);

    bool isAutoCompleteEnabled() const;
    void enableAutoComplete(bool enable);

    QString script() const;
    void setScript(const QString& text);

    bool isRunning() const;

    void setFocus();

public slots:
    void runScript(RunMode mode, int interval = 0);
    void stopScript();

private slots:
    bool executeScript();

private:
    Ui::ScriptControl *ui;

    QTimer _timer;
    QJSEngine _jsEngine;
    QString _scriptCode;

    QSharedPointer<Script> _script;
    QSharedPointer<Storage> _storage;
    QSharedPointer<Server> _server;
    QSharedPointer<console> _console;
};

#endif // SCRIPTCONTROL_H