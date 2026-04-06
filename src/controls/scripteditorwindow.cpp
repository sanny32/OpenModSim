#include <QVBoxLayout>
#include <QWidgetAction>
#include <QCloseEvent>
#include "scripteditorwindow.h"

///
/// \brief ScriptEditorWindow::ScriptEditorWindow
///
ScriptEditorWindow::ScriptEditorWindow(ScriptDocument* doc,
                                       ModbusMultiServer* server,
                                       QWidget* parent)
    : QWidget(parent)
    , _document(doc)
    , _byteOrderStorage(doc->byteOrder())
{
    Q_ASSERT(doc);

    _scriptControl = new JScriptControl(this);
    _scriptControl->setScriptSource(doc->name());
    _scriptControl->setModbusMultiServer(server);
    _scriptControl->setByteOrder(&_byteOrderStorage);
    _scriptControl->setAddressBase(doc->addressBase());

    const auto& prefs = AppPreferences::instance();
    _scriptControl->setFont(prefs.scriptFont());
    _scriptControl->enableAutoComplete(prefs.codeAutoComplete());

    // Load document content
    _scriptControl->setScriptDocument(doc->textDocument());
    if (doc->cursorPosition() >= 0)
        _scriptControl->setCursorPosition(doc->cursorPosition());
    if (doc->scrollPosition() >= 0)
        _scriptControl->setScrollPosition(doc->scrollPosition());

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_scriptControl);

    setupScriptBar();

    connect(_scriptControl, &JScriptControl::scriptStopped, this, [this]() {
        emit scriptStopped();
    });
    connect(_scriptControl, &JScriptControl::consoleMessage,
            this, &ScriptEditorWindow::consoleMessage);
    connect(_scriptControl, &JScriptControl::helpContext,
            this, &ScriptEditorWindow::helpContext);

    // Keep document cursor/scroll in sync when the script is not running
    connect(_scriptControl, &JScriptControl::scriptStopped, this, [this]() {
        _document->setCursorPosition(_scriptControl->cursorPosition());
        _document->setScrollPosition(_scriptControl->scrollPosition());
    });
}

///
/// \brief ScriptEditorWindow::isRunning
///
bool ScriptEditorWindow::isRunning() const
{
    return _scriptControl->isRunning();
}

///
/// \brief ScriptEditorWindow::runScript
///
void ScriptEditorWindow::runScript(RunMode mode, int interval)
{
    _scriptControl->runScript(mode, interval);
    emit scriptRunning();
}

///
/// \brief ScriptEditorWindow::stopScript
///
void ScriptEditorWindow::stopScript()
{
    _scriptControl->stopScript();
}

///
/// \brief ScriptEditorWindow::closeEvent
/// Save cursor/scroll position back to document before closing.
///
void ScriptEditorWindow::closeEvent(QCloseEvent* event)
{
    _document->setCursorPosition(_scriptControl->cursorPosition());
    _document->setScrollPosition(_scriptControl->scrollPosition());
    QWidget::closeEvent(event);
}

///
/// \brief ScriptEditorWindow::setupScriptBar
///
void ScriptEditorWindow::setupScriptBar()
{
    _scriptBar = new QToolBar(this);
    _scriptBar->setIconSize(QSize(16, 16));

    _runModeCombo = new RunModeComboBox(_scriptBar);
    _runModeCombo->setCurrentRunMode(_document->settings().Mode);
    connect(_runModeCombo, &RunModeComboBox::runModeChanged, this, [this](RunMode mode) {
        auto ss = _document->settings();
        ss.Mode = mode;
        _document->setSettings(ss);
    });
    auto modeAction = new QWidgetAction(_scriptBar);
    modeAction->setDefaultWidget(_runModeCombo);
    _scriptBar->addAction(modeAction);

    // Interval spinbox (500 – 10000 ms)
    _intervalSpin = new QSpinBox(_scriptBar);
    _intervalSpin->setRange(500, 10000);
    _intervalSpin->setSingleStep(100);
    _intervalSpin->setSuffix(tr(" ms"));
    _intervalSpin->setValue(static_cast<int>(_document->settings().Interval));
    _intervalSpin->setFixedWidth(90);
    connect(_intervalSpin, &QSpinBox::valueChanged, this, [this](int value) {
        auto ss = _document->settings();
        ss.Interval = static_cast<uint>(value);
        _document->setSettings(ss);
    });
    auto intervalAction = new QWidgetAction(_scriptBar);
    intervalAction->setDefaultWidget(_intervalSpin);
    _scriptBar->addAction(intervalAction);

    // Run on startup checkbox
    _runOnStartupCheck = new QCheckBox(tr("Run on startup"), _scriptBar);
    _runOnStartupCheck->setChecked(_document->settings().RunOnStartup);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(_runOnStartupCheck, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        auto ss = _document->settings();
        ss.RunOnStartup = (state == Qt::Checked);
        _document->setSettings(ss);
    });
#else
    connect(_runOnStartupCheck, &QCheckBox::stateChanged, this, [this](int state) {
        auto ss = _document->settings();
        ss.RunOnStartup = (static_cast<Qt::CheckState>(state) == Qt::Checked);
        _document->setSettings(ss);
    });
#endif
    auto startupAction = new QWidgetAction(_scriptBar);
    startupAction->setDefaultWidget(_runOnStartupCheck);
    _scriptBar->addAction(startupAction);

    _scriptBar->addSeparator();

    _actionRun = _scriptBar->addAction(QIcon(":/res/actionRunScript.png"), tr("Run Script"));
    qobject_cast<QToolButton*>(_scriptBar->widgetForAction(_actionRun))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(_actionRun, &QAction::triggered, this, [this]() {
        const auto ss = _document->settings();
        runScript(ss.Mode, ss.Interval);
    });

    _actionStop = _scriptBar->addAction(QIcon(":/res/actionStopScript.png"), tr("Stop Script"));
    qobject_cast<QToolButton*>(_scriptBar->widgetForAction(_actionStop))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(_actionStop, &QAction::triggered, this, &ScriptEditorWindow::stopScript);

    connect(this, &ScriptEditorWindow::scriptRunning, this, &ScriptEditorWindow::updateScriptBar);
    connect(this, &ScriptEditorWindow::scriptStopped, this, &ScriptEditorWindow::updateScriptBar);
    connect(_scriptControl->scriptDocument(), &QTextDocument::contentsChanged,
            this, &ScriptEditorWindow::updateScriptBar);

    // Insert toolbar at position 0 (before the script control)
    qobject_cast<QVBoxLayout*>(layout())->insertWidget(0, _scriptBar);

    updateScriptBar();
}

///
/// \brief ScriptEditorWindow::updateScriptBar
///
void ScriptEditorWindow::updateScriptBar()
{
    if (!_scriptBar) return;

    const bool running = _scriptControl->isRunning();
    const bool hasScript = !_scriptControl->script().isEmpty();

    _actionRun->setEnabled(!running && hasScript);
    _actionStop->setEnabled(running);
    if (_runModeCombo) _runModeCombo->setEnabled(!running);
    if (_intervalSpin) _intervalSpin->setEnabled(!running);
    if (_runOnStartupCheck) _runOnStartupCheck->setEnabled(!running);
}

