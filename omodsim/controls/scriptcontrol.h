#ifndef SCRIPTCONTROL_H
#define SCRIPTCONTROL_H

#include <QToolBar>
#include <QJSEngine>
#include <QPlainTextEdit>
#include "consoleobject.h"
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

    void runScript();

private slots:
    void on_actionRun_triggered();

private:
    Ui::ScriptControl *ui;
    QToolBar* _toolBar;
    QJSEngine _jsEngine;

    QSharedPointer<ConsoleObject> _consoleObject;
    QSharedPointer<ModbusServerObject> _mbServerObject;
};

#endif // SCRIPTCONTROL_H
