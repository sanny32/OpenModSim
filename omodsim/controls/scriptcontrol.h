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
/// \brief The ModbusServerObject class
///
class ModbusServerObject : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit ModbusServerObject(ModbusMultiServer& server);

    Q_INVOKABLE quint16 readHolding(quint16 address);
    Q_INVOKABLE void writeHolding(quint16 address, quint16 value);

    Q_INVOKABLE quint16 readInput(quint16 address);
    Q_INVOKABLE void writeInput(quint16 address, quint16 value);

    Q_INVOKABLE bool readDiscrete(quint16 address);
    Q_INVOKABLE void writeDiscrete(quint16 address, bool value);

    Q_INVOKABLE bool readCoil(quint16 address);
    Q_INVOKABLE void writeCoil(quint16 address, bool value);

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

    void initJSEngine(ModbusMultiServer& server);

private slots:
    void on_actionRun_triggered();

private:
    Ui::ScriptControl *ui;
    QToolBar* _toolBar;
    QJSEngine _jsEngine;
    QSharedPointer<ModbusServerObject> _mbServerObject;
};

#endif // SCRIPTCONTROL_H
