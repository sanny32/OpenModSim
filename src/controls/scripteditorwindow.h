#ifndef SCRIPTEDITORWINDOW_H
#define SCRIPTEDITORWINDOW_H

#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QToolBar>
#include "scriptdocument.h"
#include "jscriptcontrol.h"
#include "modbusmultiserver.h"
#include "apppreferences.h"
#include "runmodecombobox.h"

///
/// \brief The ScriptEditorWindow class
/// MDI widget that wraps JScriptControl for a standalone ScriptDocument.
///
class ScriptEditorWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ScriptEditorWindow(ScriptDocument* doc,
                                ModbusMultiServer* server,
                                QWidget* parent = nullptr);

    ScriptDocument* document() const { return _document; }
    JScriptControl* scriptControl() const { return _scriptControl; }

    bool isRunning() const;

public slots:
    void runScript(RunMode mode, int interval);
    void stopScript();

signals:
    void scriptRunning();
    void scriptStopped();
    void consoleMessage(const QString& source, const QString& text,
                        ConsoleOutput::MessageType type);
    void helpContext(const QString& helpKey);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupScriptBar();
    void updateScriptBar();

private:
    ScriptDocument* _document;
    JScriptControl* _scriptControl;
    ByteOrder _byteOrderStorage;

    QToolBar*        _scriptBar = nullptr;
    QAction*         _actionRun = nullptr;
    QAction*         _actionStop = nullptr;
    RunModeComboBox* _runModeCombo = nullptr;
    QSpinBox*        _intervalSpin = nullptr;
    QCheckBox*       _runOnStartupCheck = nullptr;
};

#endif // SCRIPTEDITORWINDOW_H
