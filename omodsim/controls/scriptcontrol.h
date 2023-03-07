#ifndef SCRIPTCONTROL_H
#define SCRIPTCONTROL_H

#include <QToolBar>
#include <QJSEngine>
#include <QPlainTextEdit>

namespace Ui {
class ScriptControl;
}

class ModbusMultiServer;

///
/// \brief The ModbusDevice class
///
class ModbusDevice : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit ModbusDevice(ModbusMultiServer& server);

    Q_INVOKABLE quint16 readValue(quint16 address);
    Q_INVOKABLE bool writeValue(quint16 address, quint16 value);

private:
    ModbusMultiServer& _mbMultiServer;
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
    ModbusDevice* _mbDevice;
    QJSEngine _jsEngine;
};

#endif // SCRIPTCONTROL_H
