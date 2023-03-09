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

    QString script() const;
    void setScript(const QString& text);

    void runScript();
    void stopScript();

private:
    Ui::ScriptControl *ui;

    QJSEngine _jsEngine;
    QSharedPointer<ConsoleObject> _consoleObject;
    QSharedPointer<ModbusServerObject> _mbServerObject;
};

#endif // SCRIPTCONTROL_H
