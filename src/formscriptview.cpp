#include <QCheckBox>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QSpinBox>
#include <QToolButton>
#include <QDateTime>
#include <QTextDocument>
#include "mainwindow.h"
#include "controls/runmodecombobox.h"
#include "formscriptview.h"
#include "ui_formscriptview.h"

///
/// \brief FormScriptView::FormScriptView
/// \param id
/// \param server
/// \param simulator
/// \param parent
///
FormScriptView::FormScriptView(ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent)
    : QWidget(parent)
    , ui(new Ui::FormScriptView)
    , _parent(parent)
{
    Q_ASSERT(parent != nullptr);
    Q_UNUSED(simulator);

    ui->setupUi(this);
    setWindowIcon(QIcon(":/res/icon-show-script.png"));

    _displayDefinition.FormName = windowTitle();
    _displayDefinition.ScriptCfg = _scriptSettings;
    _displayDefinition.normalize();

    ui->scriptControl->setModbusMultiServer(&server);
    ui->scriptControl->setByteOrder(&_byteOrder);
    ui->scriptControl->setScriptSource(windowTitle());

    connect(this, &QWidget::windowTitleChanged, this, &FormScriptView::on_windowTitleChanged);
    connect(ui->scriptControl, &JScriptControl::helpContext, this, &FormScriptView::helpContextRequested);
    connect(ui->scriptControl, &JScriptControl::scriptStopped, this, &FormScriptView::scriptStopped);
    connect(ui->scriptControl, &JScriptControl::consoleMessage, this, &FormScriptView::consoleMessage);

    setupScriptBar();
}

///
/// \brief FormScriptView::~FormScriptView
///
FormScriptView::~FormScriptView()
{
    delete ui;
}

///
/// \brief FormScriptView::saveSettings
/// \param out
///
void FormScriptView::saveSettings(QSettings& out) const
{
    out << const_cast<FormScriptView*>(this);
}

///
/// \brief FormScriptView::print
/// \param printer
///
void FormScriptView::print(QPrinter* printer)
{
    if (!printer) return;

    const auto header = QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
    const auto text = header + "\n\n" + script();

    QTextDocument doc;
    doc.setDefaultFont(font());
    doc.setPlainText(text);
    doc.print(printer);
}

///
/// \brief FormScriptView::loadSettings
/// \param in
///
void FormScriptView::loadSettings(QSettings& in)
{
    in >> this;
}

///
/// \brief FormScriptView::saveXml
/// \param xml
///
void FormScriptView::saveXml(QXmlStreamWriter& xml) const
{
    xml << const_cast<FormScriptView*>(this);
}

///
/// \brief FormScriptView::loadXml
/// \param xml
///
void FormScriptView::loadXml(QXmlStreamReader& xml)
{
    xml >> this;
}

///
/// \brief FormScriptView::on_windowTitleChanged
/// \param title
///
void FormScriptView::on_windowTitleChanged(const QString& title)
{
    ui->scriptControl->setScriptSource(title);
}

///
/// \brief FormScriptView::changeEvent
/// \param e
///
void FormScriptView::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        if (_scriptIntervalSpin)
            _scriptIntervalSpin->setSuffix(tr(" ms"));
        if (_scriptRunOnStartupCheck)
            _scriptRunOnStartupCheck->setText(tr("Run on startup"));
        updateScriptBarToolTips();
    }
    QWidget::changeEvent(e);
}

///
/// \brief FormScriptView::closeEvent
/// \param event
///
void FormScriptView::closeEvent(QCloseEvent* event)
{
    emit closing();
    QWidget::closeEvent(event);
}

///
/// \brief FormScriptView::mouseDoubleClickEvent
/// \param event
///
void FormScriptView::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);
}

///
/// \brief FormScriptView::definitions
/// \return
///
ScriptViewDefinitions FormScriptView::definitions() const
{
    ScriptViewDefinitions dd = _displayDefinition;
    dd.FormName = windowTitle();
    dd.ScriptCfg = _scriptSettings;
    dd.normalize();
    return dd;
}

///
/// \brief FormScriptView::setDefinitions
/// \param dd
///
void FormScriptView::setDefinitions(const ScriptViewDefinitions& dd)
{
    ScriptViewDefinitions next = dd;
    if (!next.FormName.isEmpty())
        setWindowTitle(next.FormName);
    else
        next.FormName = windowTitle();

    next.normalize();
    _displayDefinition = next;
    setScriptSettings(_displayDefinition.ScriptCfg);
}

///
/// \brief FormScriptView::scriptSettings
/// \return
///
ScriptSettings FormScriptView::scriptSettings() const
{
    return _scriptSettings;
}

///
/// \brief FormScriptView::setScriptSettings
/// \param ss
///
void FormScriptView::setScriptSettings(const ScriptSettings& ss)
{
    if(_scriptSettings == ss)
        return;

    _scriptSettings = ss;
    _scriptRunModeCombo->setCurrentRunMode(ss.Mode);
    _scriptIntervalSpin->setValue(static_cast<int>(ss.Interval));
    _scriptRunOnStartupCheck->setChecked(ss.RunOnStartup);

    emit scriptSettingsChanged(ss);
    updateScriptBar();
}

///
/// \brief FormScriptView::script
/// \return
///
QString FormScriptView::script() const
{
    return ui->scriptControl->script();
}

///
/// \brief FormScriptView::setFormName
/// \param name
///
void FormScriptView::setFormName(const QString& name)
{
    setWindowTitle(name);
    ui->scriptControl->setScriptSource(name);
}

///
/// \brief FormScriptView::setScript
/// \param text
///
void FormScriptView::setScript(const QString& text)
{
    ui->scriptControl->setScript(text);
}

///
/// \brief FormScriptView::scriptDocument
/// \return
///
QTextDocument* FormScriptView::scriptDocument() const
{
    return ui->scriptControl->scriptDocument();
}

///
/// \brief FormScriptView::setScriptDocument
/// \param document
///
void FormScriptView::setScriptDocument(QTextDocument* document)
{
    auto* oldDoc = ui->scriptControl->scriptDocument();
    if(oldDoc == document)
        return;
    if(oldDoc)
        disconnect(oldDoc, &QTextDocument::contentsChanged, this, &FormScriptView::updateScriptBar);
    ui->scriptControl->setScriptDocument(document);
    if(document)
        connect(document, &QTextDocument::contentsChanged, this, &FormScriptView::updateScriptBar);
}

///
/// \brief FormScriptView::scriptCursorPosition
/// \return
///
int FormScriptView::scriptCursorPosition() const
{
    return ui->scriptControl->cursorPosition();
}

///
/// \brief FormScriptView::setScriptCursorPosition
/// \param pos
///
void FormScriptView::setScriptCursorPosition(int pos)
{
    ui->scriptControl->setCursorPosition(pos);
}

///
/// \brief FormScriptView::scriptScrollPosition
/// \return
///
int FormScriptView::scriptScrollPosition() const
{
    return ui->scriptControl->scrollPosition();
}

///
/// \brief FormScriptView::setScriptScrollPosition
/// \param pos
///
void FormScriptView::setScriptScrollPosition(int pos)
{
    ui->scriptControl->setScrollPosition(pos);
}

///
/// \brief FormScriptView::searchText
/// \return
///
QString FormScriptView::searchText() const
{
    return ui->scriptControl->searchText();
}

///
/// \brief FormScriptView::backgroundColor
/// \return
///
QColor FormScriptView::backgroundColor() const
{
    return ui->scriptControl->backgroundColor();
}

///
/// \brief FormScriptView::setBackgroundColor
/// \param clr
///
void FormScriptView::setBackgroundColor(const QColor& clr)
{
    ui->scriptControl->setBackgroundColor(clr);
}

///
/// \brief FormScriptView::foregroundColor
/// \return
///
QColor FormScriptView::foregroundColor() const
{
    return ui->scriptControl->foregroundColor();
}

///
/// \brief FormScriptView::setForegroundColor
/// \param clr
///
void FormScriptView::setForegroundColor(const QColor& clr)
{
    ui->scriptControl->setForegroundColor(clr);
}

///
/// \brief FormScriptView::font
/// \return
///
QFont FormScriptView::font() const
{
    return ui->scriptControl->font();
}

///
/// \brief FormScriptView::setFont
/// \param font
///
void FormScriptView::setFont(const QFont& font)
{
    ui->scriptControl->setFont(font);
}

///
/// \brief FormScriptView::zoomPercent
/// \return
///
int FormScriptView::zoomPercent() const
{
    return 100;
}

///
/// \brief FormScriptView::setZoomPercent
/// \param zoomPercent
///
void FormScriptView::setZoomPercent(int zoomPercent)
{
    Q_UNUSED(zoomPercent);
}

///
/// \brief FormScriptView::canRunScript
/// \return
///
bool FormScriptView::canRunScript() const
{
    return !_peerRunning && !ui->scriptControl->script().isEmpty() && !ui->scriptControl->isRunning();
}

///
/// \brief FormScriptView::canStopScript
/// \return
///
bool FormScriptView::canStopScript() const
{
    return _peerRunning || ui->scriptControl->isRunning();
}

///
/// \brief FormScriptView::canUndo
/// \return
///
bool FormScriptView::canUndo() const
{
    return ui->scriptControl->canUndo();
}

///
/// \brief FormScriptView::canRedo
/// \return
///
bool FormScriptView::canRedo() const
{
    return ui->scriptControl->canRedo();
}

///
/// \brief FormScriptView::canPaste
/// \return
///
bool FormScriptView::canPaste() const
{
    return ui->scriptControl->canPaste();
}

///
/// \brief FormScriptView::runScript
///
void FormScriptView::runScript()
{
    ui->scriptControl->runScript(_scriptSettings.Mode, _scriptSettings.Interval);
    emit scriptRunning();
}

///
/// \brief FormScriptView::stopScript
///
void FormScriptView::stopScript()
{
    ui->scriptControl->stopScript();
}

///
/// \brief FormScriptView::setPeerRunning
/// Called by a linked peer (split-view clone) to mirror the master's running state.
///
void FormScriptView::setPeerRunning(bool running)
{
    if(_peerRunning == running) return;
    _peerRunning = running;
    updateScriptBar();
}

///
/// \brief FormScriptView::linkRunStopTo
/// Makes this form a visual mirror of \a master:
/// - Run/Stop toolbar buttons delegate to master.
/// - Toolbar state reflects master's running state via setPeerRunning.
///
void FormScriptView::linkRunStopTo(FormScriptView* master)
{
    if(!master) return;
    disconnect(ui->actionRunScript,  &QAction::triggered, this, &FormScriptView::runScript);
    disconnect(ui->actionStopScript, &QAction::triggered, this, &FormScriptView::stopScript);
    connect(ui->actionRunScript,  &QAction::triggered, master, &FormScriptView::runScript);
    connect(ui->actionStopScript, &QAction::triggered, master, &FormScriptView::stopScript);
    connect(master, &FormScriptView::scriptRunning, this, [this]() { setPeerRunning(true); });
    connect(master, &FormScriptView::scriptStopped, this, [this]() { setPeerRunning(false); });
    setPeerRunning(master->canStopScript());
}

///
/// \brief FormScriptView::scriptControl
/// \return
///
JScriptControl* FormScriptView::scriptControl()
{
    return ui->scriptControl;
}

///
/// \brief FormScriptView::isAutoCompleteEnabled
/// \return
///
bool FormScriptView::isAutoCompleteEnabled() const
{
    return ui->scriptControl->isAutoCompleteEnabled();
}

///
/// \brief FormScriptView::enableAutoComplete
/// \param enable
///
void FormScriptView::enableAutoComplete(bool enable)
{
    ui->scriptControl->enableAutoComplete(enable);
}

///
/// \brief FormScriptView::setupScriptBar
///
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

    QAction* firstAction = nullptr;
    const auto toolbarActions = ui->toolBarScript->actions();
    if (!toolbarActions.isEmpty())
        firstAction = toolbarActions.first();

    _scriptRunOnStartupCheck = new QCheckBox(tr("Run on startup"), ui->toolBarScript);
    _scriptRunOnStartupCheck->setChecked(_scriptSettings.RunOnStartup);

    auto* leftWidget = new QWidget(ui->toolBarScript);
    auto* leftLayout = new QHBoxLayout(leftWidget);
    leftLayout->setContentsMargins(9, 0, 9, 0);
    leftLayout->setSpacing(6);
    leftLayout->addWidget(_scriptRunModeCombo);
    leftLayout->addWidget(_scriptIntervalSpin);

    auto* rightSpacer = new QWidget(ui->toolBarScript);
    rightSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* rightWidget = new QWidget(ui->toolBarScript);
    auto* rightLayout = new QHBoxLayout(rightWidget);
    rightLayout->setContentsMargins(9, 0, 9, 0);
    rightLayout->setSpacing(0);
    rightLayout->addWidget(_scriptRunOnStartupCheck);

    const auto prependToolbarWidget = [this, firstAction](QWidget* widget) {
        if (firstAction)
            ui->toolBarScript->insertWidget(firstAction, widget);
        else
            ui->toolBarScript->addWidget(widget);
    };

    prependToolbarWidget(leftWidget);

    if (firstAction)
        ui->toolBarScript->insertSeparator(firstAction);
    else
        ui->toolBarScript->addSeparator();

    ui->toolBarScript->addWidget(rightSpacer);
    ui->toolBarScript->addWidget(rightWidget);

    connect(_scriptRunModeCombo, &RunModeComboBox::runModeChanged, this, [this](RunMode mode) {
        if(_scriptSettings.Mode == mode) return;
        _scriptSettings.Mode = mode;
        emit scriptSettingsChanged(_scriptSettings);
        updateScriptBar();
    });

    connect(_scriptIntervalSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int value) {
        const auto v = static_cast<uint>(value);
        if(_scriptSettings.Interval == v) return;
        _scriptSettings.Interval = v;
        emit scriptSettingsChanged(_scriptSettings);
    });

    connect(_scriptRunOnStartupCheck, &QCheckBox::stateChanged, this, [this](int state) {
        const bool checked = (state == Qt::Checked);
        if(_scriptSettings.RunOnStartup == checked) return;
        _scriptSettings.RunOnStartup = checked;
        emit scriptSettingsChanged(_scriptSettings);
    });

    if (auto* runButton = qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionRunScript)))
        runButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    if (auto* stopButton = qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionStopScript)))
        stopButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    updateScriptBarToolTips();

    connect(ui->actionRunScript, &QAction::triggered, this, &FormScriptView::runScript);
    connect(ui->actionStopScript, &QAction::triggered, this, &FormScriptView::stopScript);
    connect(this, &FormScriptView::scriptRunning, this, &FormScriptView::updateScriptBar);
    connect(this, &FormScriptView::scriptStopped, this, &FormScriptView::updateScriptBar);
    connect(ui->scriptControl->scriptDocument(), &QTextDocument::contentsChanged, this, &FormScriptView::updateScriptBar);

    updateScriptBar();
}

///
/// \brief FormScriptView::updateScriptBarToolTips
///
void FormScriptView::updateScriptBarToolTips()
{
    ui->actionRunScript->setToolTip(ui->actionRunScript->text());
    ui->actionStopScript->setToolTip(ui->actionStopScript->text());

    if (auto* runButton = qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionRunScript)))
        runButton->setToolTip(ui->actionRunScript->toolTip());
    if (auto* stopButton = qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionStopScript)))
        stopButton->setToolTip(ui->actionStopScript->toolTip());

    if (_scriptRunModeCombo)
        _scriptRunModeCombo->setToolTip(tr("Script run mode"));
    if (_scriptIntervalSpin)
        _scriptIntervalSpin->setToolTip(tr("Script run interval"));
    if (_scriptRunOnStartupCheck)
        _scriptRunOnStartupCheck->setToolTip(_scriptRunOnStartupCheck->text());
}

///
/// \brief FormScriptView::updateScriptBar
///
void FormScriptView::updateScriptBar()
{
    const bool running = canStopScript();
    ui->actionRunScript->setEnabled(canRunScript());
    ui->actionStopScript->setEnabled(running);
    if (_scriptRunModeCombo)
        _scriptRunModeCombo->setEnabled(!running);
    if (_scriptIntervalSpin)
        _scriptIntervalSpin->setEnabled(!running && _scriptSettings.Mode != RunMode::Once);
    if (_scriptRunOnStartupCheck)
        _scriptRunOnStartupCheck->setEnabled(!running);
}

///
/// \brief FormScriptView::show
///
void FormScriptView::show()
{
    QWidget::show();
    connectEditSlots();
    emit showed();
}

///
/// \brief FormScriptView::connectEditSlots
///
void FormScriptView::connectEditSlots()
{
    disconnectEditSlots();
    connect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    connect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    connect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    connect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}

///
/// \brief FormScriptView::disconnectEditSlots
///
void FormScriptView::disconnectEditSlots()
{
    disconnect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    disconnect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    disconnect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    disconnect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}

