#include <QCheckBox>
#include <QSpinBox>
#include <QToolButton>
#include "mainwindow.h"
#include "controls/runmodecombobox.h"
#include "formscriptview.h"
#include "ui_formscriptview.h"

FormScriptView::FormScriptView(int id, ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent)
    : QWidget(parent)
    , ui(new Ui::FormScriptView)
    , _parent(parent)
    , _formId(id)
{
    Q_ASSERT(parent != nullptr);
    Q_UNUSED(server);
    Q_UNUSED(simulator);

    ui->setupUi(this);

    setWindowTitle(QString("Script%1").arg(_formId));
    setWindowIcon(QIcon(":/res/actionShowScript.png"));

    _displayDefinition.FormName = windowTitle();
    _displayDefinition.ScriptCfg = _scriptSettings;
    _displayDefinition.normalize();

    ui->scriptControl->setScriptSource(windowTitle());
    connect(ui->scriptControl, &JScriptControl::helpContext, this, &FormScriptView::helpContextRequested);
    connect(ui->scriptControl, &JScriptControl::scriptStopped, this, &FormScriptView::scriptStopped);
    connect(ui->scriptControl, &JScriptControl::consoleMessage, this, &FormScriptView::consoleMessage);

    setupScriptBar();
}

FormScriptView::~FormScriptView()
{
    delete ui;
}

void FormScriptView::saveSettings(QSettings& out) const
{
    out << const_cast<FormScriptView*>(this);
}

void FormScriptView::loadSettings(QSettings& in)
{
    in >> this;
}

void FormScriptView::saveXml(QXmlStreamWriter& xml) const
{
    xml << const_cast<FormScriptView*>(this);
}

void FormScriptView::loadXml(QXmlStreamReader& xml)
{
    xml >> this;
}

void FormScriptView::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        if (_scriptIntervalSpin)
            _scriptIntervalSpin->setSuffix(tr(" ms"));
        if (_scriptRunOnStartupCheck)
            _scriptRunOnStartupCheck->setText(tr("Run on startup"));
    }
    QWidget::changeEvent(e);
}

void FormScriptView::closeEvent(QCloseEvent* event)
{
    emit closing();
    QWidget::closeEvent(event);
}

void FormScriptView::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);
}

ScriptViewDefinitions FormScriptView::displayDefinition() const
{
    ScriptViewDefinitions dd = _displayDefinition;
    dd.FormName = windowTitle();
    dd.ScriptCfg = _scriptSettings;
    dd.normalize();
    return dd;
}

void FormScriptView::setDisplayDefinition(const ScriptViewDefinitions& dd)
{
    ScriptViewDefinitions next = dd;
    if (!next.FormName.isEmpty())
        setWindowTitle(next.FormName);
    else
        next.FormName = windowTitle();

    next.ScriptCfg = _scriptSettings;
    next.normalize();
    _displayDefinition = next;
    setScriptSettings(_displayDefinition.ScriptCfg);
}

FormDisplayDefinition FormScriptView::displayDefinitionValue() const
{
    return displayDefinition();
}

void FormScriptView::setDisplayDefinitionValue(const FormDisplayDefinition& dd)
{
    if (const auto value = std::get_if<ScriptViewDefinitions>(&dd))
        setDisplayDefinition(*value);
}

ScriptSettings FormScriptView::scriptSettings() const
{
    return _scriptSettings;
}

void FormScriptView::setScriptSettings(const ScriptSettings& ss)
{
    _scriptSettings = ss;
    if (_scriptRunModeCombo)
        _scriptRunModeCombo->setCurrentRunMode(ss.Mode);
    if (_scriptIntervalSpin)
        _scriptIntervalSpin->setValue(static_cast<int>(ss.Interval));
    if (_scriptRunOnStartupCheck)
        _scriptRunOnStartupCheck->setChecked(ss.RunOnStartup);
    emit scriptSettingsChanged(ss);
}

QString FormScriptView::script() const
{
    return ui->scriptControl->script();
}

void FormScriptView::setScript(const QString& text)
{
    ui->scriptControl->setScript(text);
}

QTextDocument* FormScriptView::scriptDocument() const
{
    return ui->scriptControl->scriptDocument();
}

void FormScriptView::setScriptDocument(QTextDocument* document)
{
    ui->scriptControl->setScriptDocument(document);
}

int FormScriptView::scriptCursorPosition() const
{
    return ui->scriptControl->cursorPosition();
}

void FormScriptView::setScriptCursorPosition(int pos)
{
    ui->scriptControl->setCursorPosition(pos);
}

int FormScriptView::scriptScrollPosition() const
{
    return ui->scriptControl->scrollPosition();
}

void FormScriptView::setScriptScrollPosition(int pos)
{
    ui->scriptControl->setScrollPosition(pos);
}

QString FormScriptView::searchText() const
{
    return ui->scriptControl->searchText();
}

QColor FormScriptView::backgroundColor() const
{
    return ui->scriptControl->palette().color(QPalette::Base);
}

void FormScriptView::setBackgroundColor(const QColor& clr)
{
    auto pal = ui->scriptControl->palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    ui->scriptControl->setPalette(pal);
}

QColor FormScriptView::foregroundColor() const
{
    return ui->scriptControl->palette().color(QPalette::Text);
}

void FormScriptView::setForegroundColor(const QColor& clr)
{
    auto pal = ui->scriptControl->palette();
    pal.setColor(QPalette::Text, clr);
    ui->scriptControl->setPalette(pal);
}

QFont FormScriptView::font() const
{
    return ui->scriptControl->font();
}

void FormScriptView::setFont(const QFont& font)
{
    ui->scriptControl->setFont(font);
}

int FormScriptView::zoomPercent() const
{
    return 100;
}

void FormScriptView::setZoomPercent(int zoomPercent)
{
    Q_UNUSED(zoomPercent);
}

bool FormScriptView::canRunScript() const
{
    return !ui->scriptControl->script().isEmpty() && !ui->scriptControl->isRunning();
}

bool FormScriptView::canStopScript() const
{
    return ui->scriptControl->isRunning();
}

bool FormScriptView::canUndo() const
{
    return ui->scriptControl->canUndo();
}

bool FormScriptView::canRedo() const
{
    return ui->scriptControl->canRedo();
}

bool FormScriptView::canPaste() const
{
    return ui->scriptControl->canPaste();
}

void FormScriptView::runScript()
{
    emit scriptRunning();
    ui->scriptControl->runScript(_scriptSettings.Mode, _scriptSettings.Interval);
}

void FormScriptView::stopScript()
{
    ui->scriptControl->stopScript();
}

JScriptControl* FormScriptView::scriptControl()
{
    return ui->scriptControl;
}

bool FormScriptView::isAutoCompleteEnabled() const
{
    return ui->scriptControl->isAutoCompleteEnabled();
}

void FormScriptView::enableAutoComplete(bool enable)
{
    ui->scriptControl->enableAutoComplete(enable);
}

void FormScriptView::setupScriptBar()
{
    _scriptRunModeCombo = new RunModeComboBox(ui->toolBarScript);
    _scriptRunModeCombo->setCurrentRunMode(_scriptSettings.Mode);

    _scriptIntervalSpin = new QSpinBox(ui->toolBarScript);
    _scriptIntervalSpin->setRange(500, 10000);
    _scriptIntervalSpin->setSingleStep(100);
    _scriptIntervalSpin->setSuffix(tr(" ms"));
    _scriptIntervalSpin->setValue(static_cast<int>(_scriptSettings.Interval));
    _scriptIntervalSpin->setFixedWidth(90);

    _scriptRunOnStartupCheck = new QCheckBox(tr("Run on startup"), ui->toolBarScript);
    _scriptRunOnStartupCheck->setChecked(_scriptSettings.RunOnStartup);

    QAction* firstAction = nullptr;
    const auto toolbarActions = ui->toolBarScript->actions();
    if (!toolbarActions.isEmpty())
        firstAction = toolbarActions.first();

    if (firstAction) {
        ui->toolBarScript->insertWidget(firstAction, _scriptRunModeCombo);
        ui->toolBarScript->insertWidget(firstAction, _scriptIntervalSpin);
        ui->toolBarScript->insertWidget(firstAction, _scriptRunOnStartupCheck);
        ui->toolBarScript->insertSeparator(firstAction);
    } else {
        ui->toolBarScript->addWidget(_scriptRunModeCombo);
        ui->toolBarScript->addWidget(_scriptIntervalSpin);
        ui->toolBarScript->addWidget(_scriptRunOnStartupCheck);
        ui->toolBarScript->addSeparator();
    }

    connect(_scriptRunModeCombo, &RunModeComboBox::runModeChanged, this, [this](RunMode mode) {
        _scriptSettings.Mode = mode;
    });

    connect(_scriptIntervalSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int value) {
        _scriptSettings.Interval = static_cast<uint>(value);
    });

    connect(_scriptRunOnStartupCheck, &QCheckBox::stateChanged, this, [this](int state) {
        _scriptSettings.RunOnStartup = (state == Qt::Checked);
    });

    if (auto* runButton = qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionRunScript)))
        runButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    if (auto* stopButton = qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionStopScript)))
        stopButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    connect(ui->actionRunScript, &QAction::triggered, this, &FormScriptView::runScript);
    connect(ui->actionStopScript, &QAction::triggered, this, &FormScriptView::stopScript);
    connect(this, &FormScriptView::scriptRunning, this, &FormScriptView::updateScriptBar);
    connect(this, &FormScriptView::scriptStopped, this, &FormScriptView::updateScriptBar);
    connect(ui->scriptControl->scriptDocument(), &QTextDocument::contentsChanged,
            this, &FormScriptView::updateScriptBar);

    updateScriptBar();
}

void FormScriptView::updateScriptBar()
{
    const bool running = canStopScript();
    ui->actionRunScript->setEnabled(canRunScript());
    ui->actionStopScript->setEnabled(running);
    if (_scriptRunModeCombo)
        _scriptRunModeCombo->setEnabled(!running);
    if (_scriptIntervalSpin)
        _scriptIntervalSpin->setEnabled(!running);
    if (_scriptRunOnStartupCheck)
        _scriptRunOnStartupCheck->setEnabled(!running);
}

void FormScriptView::show()
{
    QWidget::show();
    connectEditSlots();
    emit showed();
}

void FormScriptView::connectEditSlots()
{
    disconnectEditSlots();
    connect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    connect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    connect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    connect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}

void FormScriptView::disconnectEditSlots()
{
    disconnect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    disconnect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    disconnect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    disconnect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}
