#ifndef SCRIPTCONTROL_H
#define SCRIPTCONTROL_H

#include <QTimer>
#include <QJSEngine>
#include <QPlainTextEdit>
#include "consoleobject.h"
#include "scriptobject.h"
#include "modbusserverobject.h"


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

    void initJSEngine(ModbusMultiServer& server);

    QString script() const;
    void setScript(const QString& text);

    bool isRunning() const;

    RunMode runMode() const;
    void setRunMode(RunMode mode);

public slots:
    void runScript(int interval = 0);
    void stopScript();

private slots:
    void executeScript();

private:
    Ui::ScriptControl *ui;

    RunMode _runMode;
    QTimer _timer;
    QJSEngine _jsEngine;

    QSharedPointer<ScriptObject> _scriptObject;
    QSharedPointer<ConsoleObject> _consoleObject;
    QSharedPointer<ModbusServerObject> _mbServerObject;
};

#endif // SCRIPTCONTROL_H
