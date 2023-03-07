#ifndef SCRIPTCONTROL_H
#define SCRIPTCONTROL_H

#include <QToolBar>
#include <QJSEngine>
#include <QPlainTextEdit>

namespace Ui {
class ScriptControl;
}

///
/// \brief The ModbusObject class
///
class ModbusObject : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit ModbusObject();

    Q_INVOKABLE quint16 readValue(quint16 address);
    Q_INVOKABLE bool writeValue(quint16 address, quint16 value);
};

///
/// \brief The ScriptControl class
///
class ScriptControl : public QWidget
{
    Q_OBJECT

public:
    explicit ScriptControl(QWidget *parent = nullptr);
    ~ScriptControl();

private slots:
    void on_actionRun_triggered();

private:
    void setupJSEngine();

private:
    Ui::ScriptControl *ui;
    QToolBar* _toolBar;
    ModbusObject* _mbObject;
    QJSEngine _jsEngine;
};

#endif // SCRIPTCONTROL_H
