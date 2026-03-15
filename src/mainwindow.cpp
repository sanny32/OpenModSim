#include <QtWidgets>
#include <QBuffer>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QPageSetupDialog>
#include "apppreferences.h"
#include "dialogabout.h"
#include "dialogmsgparser.h"
#include "dialogpreferences.h"
#include "dialogwindowsmanager.h"
#include "dialogprintsettings.h"
#include "dialogdisplaydefinition.h"
#include "dialogselectserviceport.h"
#include "dialogsetupserialport.h"
#include "dialogsetuppresetdata.h"
#include "dialogforcemultiplecoils.h"
#include "dialogforcemultipleregisters.h"
#include "dialogmodbusdefinitions.h"
#include "controls/trafficlogwindow.h"
#include "mainstatusbar.h"
#include "menuconnect.h"
#include "controls/mdiareaex.h"
#include "mainwindow.h"
#include "uiutils.h"
#include "ui_mainwindow.h"

// Forward declaration (defined later in this file)
static QString getSettingsFilePath();

namespace {
constexpr const char* kSplitMirrorPeerId = "SplitMirrorPeerId";
constexpr const char* kSplitScriptRunning = "SplitScriptRunning";
}

///
/// \brief MainWindow::MainWindow
/// \param profile
/// \param useSession
/// \param parent
///
MainWindow::MainWindow(const QString& profile, bool useSession, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,_lang(translationLang())
    ,_windowCounter(0)
    ,_useSession(useSession)
    ,_dataSimulator(new DataSimulator(this))
    ,_helpDockWidget(nullptr)
    ,_helpWidget(nullptr)
{
    ui->setupUi(this);

    setLanguage(_lang);
    setWindowTitle(APP_NAME);
    setUnifiedTitleAndToolBarOnMac(true);
    setStatusBar(new MainStatusBar(_mbMultiServer, this));

    _ansiMenu = new AnsiMenu(this);
    connect(_ansiMenu, &AnsiMenu::codepageSelected, this, &MainWindow::setCodepage);
    ui->actionAnsi->setMenu(_ansiMenu);

    auto menuConnect = new MenuConnect(MenuConnect::ConnectMenu, _mbMultiServer, this);
    connect(menuConnect, &MenuConnect::connectAction, this, &MainWindow::on_connectAction);

    ui->actionConnect->setMenu(menuConnect);
    qobject_cast<QToolButton*>(ui->toolBarMain->widgetForAction(ui->actionConnect))->setPopupMode(QToolButton::InstantPopup);
    qobject_cast<QToolButton*>(ui->toolBarMain->widgetForAction(ui->actionConnect))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    auto menuDisconnect = new MenuConnect(MenuConnect::DisconnectMenu, _mbMultiServer, this);
    connect(menuDisconnect, &MenuConnect::disconnectAction, this, &MainWindow::on_disconnectAction);

    ui->actionDisconnect->setMenu(menuDisconnect);
    qobject_cast<QToolButton*>(ui->toolBarMain->widgetForAction(ui->actionDisconnect))->setPopupMode(QToolButton::InstantPopup);
    qobject_cast<QToolButton*>(ui->toolBarMain->widgetForAction(ui->actionDisconnect))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    const auto defaultPrinter = QPrinterInfo::defaultPrinter();
    if(!defaultPrinter.isNull())
        _selectedPrinter = QSharedPointer<QPrinter>(new QPrinter(defaultPrinter));

    _windowActionList = new WindowActionList(ui->menuWindow, ui->actionWindows);
    connect(_windowActionList, &WindowActionList::triggered, this, &MainWindow::windowActivate);

    auto dispatcher = QAbstractEventDispatcher::instance();
    connect(dispatcher, &QAbstractEventDispatcher::awake, this, &MainWindow::on_awake);

    _helpWidget = new HelpWidget(this);
    auto helpfile = QApplication::applicationDirPath() + "/docs/jshelp.qhc";
    if(!QFile::exists(helpfile)){
        helpfile = QApplication::applicationDirPath() + "/../docs/jshelp.qhc";
    }
    _helpWidget->setHelp(helpfile);

    _helpDockWidget = new QDockWidget(tr("Script Help"), this);
    _helpDockWidget->setObjectName("helpDockWidget");
    _helpDockWidget->setWidget(_helpWidget);
    _helpDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, _helpDockWidget);
    _helpDockWidget->setVisible(false);

    connect(_helpDockWidget, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if(_helpDockWidget->isVisible())
            _helpDockWidget->setProperty("WasShown", false);
    });

    _projectTree = new ProjectTreeWidget(this);
    _projectDockWidget = new QDockWidget(tr("Project"), this);
    _projectDockWidget->setObjectName("projectDockWidget");
    _projectDockWidget->setWidget(_projectTree);
    _projectDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, _projectDockWidget);

    connect(_projectTree, &ProjectTreeWidget::formActivated, this, [this](FormModSim* frm) {
        // If the form is currently open in a tab, activate it
        const auto list = ui->mdiArea->subWindowList();
        for (auto wnd : list) {
            if (qobject_cast<FormModSim*>(wnd->widget()) == frm) {
                ui->mdiArea->setActiveSubWindow(wnd);
                return;
            }
        }
        // Form tab was closed but form is still alive — re-open it
        if (_closedForms.contains(frm))
            rewrapMdiChild(frm);
    });
    connect(_projectTree, &ProjectTreeWidget::formDeleteRequested, this, [this](FormModSim* frm) {
        deleteForm(frm);
    });
    connect(_projectTree, &ProjectTreeWidget::scriptDeleteRequested, this, [this](ScriptDocument* doc) {
        deleteScript(doc);
    });
    connect(_projectTree, &ProjectTreeWidget::scriptActivated, this, [this](ScriptDocument* doc) {
        openScriptEditor(doc);
    });

    _globalConsole = new ConsoleOutput(this);
    _consoleDockWidget = new QDockWidget(tr("Output"), this);
    _consoleDockWidget->setObjectName("consoleDockWidget");
    _consoleDockWidget->setWidget(_globalConsole);
    _consoleDockWidget->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, _consoleDockWidget);
    _consoleDockWidget->setVisible(false);

    connect(_globalConsole, &ConsoleOutput::collapse, this, [this]() {
        _consoleDockWidget->setVisible(false);
    });

    ui->mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    connect(ui->mdiArea, &MdiAreaEx::subWindowActivated, this, &MainWindow::updateMenuWindow);
    connect(ui->mdiArea, &MdiAreaEx::splitViewAboutToDisable, this, [this]() {
        _splitDisableInProgress = true;
        clearSplitMirrorsFromSecondary();
    });
    connect(ui->mdiArea, &MdiAreaEx::splitViewToggled, this, [this](bool enabled) {
        _splitDisableInProgress = false;

        if(enabled) {
            syncSplitForms();
            return;
        }

        for(auto&& wnd : ui->mdiArea->localSubWindowList()) {
            _windowActionList->addWindow(wnd);
        }
    });
    connect(&_mbMultiServer, &ModbusMultiServer::connectionError, this, &MainWindow::on_connectionError);

    if(_useSession) {
        if(!loadProfile(profile)) {
            ui->actionNew->trigger();
        }
    }
    else {
        // Load AppPreferences even without a session
        const QString settingsFile = getSettingsFilePath();
        if(QFile::exists(settingsFile)) {
            QSettings m(settingsFile, QSettings::IniFormat);
            AppPreferences::instance().load(m);
        }
    }
}

///
/// \brief MainWindow::~MainWindow
///
MainWindow::~MainWindow()
{
    // Close any remaining MDI subwindows (ScriptEditorWindows) before scripts are deleted.
    // QObject child cleanup runs after this destructor body, in reverse construction order,
    // meaning ScriptDocuments (added later) would be deleted before QMdiArea (added early by
    // setupUi). Explicitly close here to ensure QPlainTextEdit is destroyed before _document.
    ui->mdiArea->closeAllSubWindows();

    // Explicitly delete closed (hidden) forms and standalone scripts now, while ui is still
    // valid, to avoid accessing stale pointers during QObject child cleanup.
    for (auto frm : std::as_const(_closedForms))
        delete frm;   // QObject::~QObject removes frm from MainWindow's children list
    _closedForms.clear();
    qDeleteAll(_standaloneScripts);
    _standaloneScripts.clear();

    delete ui;
}

///
/// \brief MainWindow::setLanguage
/// \param lang
///
void MainWindow::setLanguage(const QString& lang)
{
    if(lang == "en")
    {
        _lang = lang;
        qApp->removeTranslator(&_appTranslator);
        qApp->removeTranslator(&_qtTranslator);
    }
    else if(_appTranslator.load(QString(":/translations/omodsim_%1").arg(lang)))
    {
        _lang = lang;
        qApp->installTranslator(&_appTranslator);

        if(_qtTranslator.load(QString("%1/translations/qt_%2").arg(qApp->applicationDirPath(), lang))) {
            qApp->installTranslator(&_qtTranslator);
        }
    }
}

///
/// \brief MainWindow::changeEvent
/// \param event
///
void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    QMainWindow::changeEvent(event);
}

///
/// \brief MainWindow::closeEvent
/// \param event
///
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(_useSession) {
        saveProfile(); // also saves AppPreferences
    }
    else {
        // Save preferences even when session tracking is off
        const QString filepath = getSettingsFilePath();
        QSettings m(filepath, QSettings::IniFormat, this);
        AppPreferences::instance().save(m);
    }

    ui->mdiArea->closeAllSubWindows();
    if (ui->mdiArea->currentSubWindow())
    {
        event->ignore();
    }
}

///
/// \brief MainWindow::eventFilter
/// \param obj
/// \param e
/// \return
///
bool MainWindow::eventFilter(QObject* obj, QEvent* e)
{
    switch (e->type())
    {
        case QEvent::Close:
            if(auto wnd = qobject_cast<QMdiSubWindow*>(obj))
            {
                _windowActionList->removeWindow(wnd);

                if(_splitDisableInProgress)
                    break;

                auto frm = qobject_cast<FormModSim*>(wnd->widget());
                const int peerId = frm ? frm->property(kSplitMirrorPeerId).toInt() : 0;
                if(peerId > 0)
                {
                    auto peer = findMdiChild(peerId);
                    if(peer)
                    {
                        frm->setProperty(kSplitMirrorPeerId, QVariant());
                        peer->setProperty(kSplitMirrorPeerId, QVariant());

                        if(auto peerWnd = qobject_cast<QMdiSubWindow*>(peer->parentWidget())) {
                            if(peerWnd != wnd)
                                peerWnd->close();
                        } else {
                            peer->close();
                        }
                    }
                }
                else if (frm)
                {
                    // Primary form: reparent before subwindow is destroyed so frm survives
                    frm->setParent(this);
                    frm->hide();
                    if (!_closedForms.contains(frm))
                        _closedForms.append(frm);
                    _projectTree->setFormOpen(frm, false);
                }
            }
        break;
        case QEvent::Move:
            if(auto wnd = qobject_cast<const QMdiSubWindow*>(obj))
            {
                if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
                {
                    if (!wnd->isMinimized() && !wnd->isMaximized())
                    {
                        frm->setParentGeometry(wnd->geometry());
                    }
                }
            }
        break;
        default:
            qt_noop();
    }
    return QObject::eventFilter(obj, e);
}

///
/// \brief MainWindow::on_awake
///
void MainWindow::on_awake()
{
    auto frm = currentMdiChild();

    ui->menuSetup->setEnabled(frm != nullptr);
    ui->menuWindow->setEnabled(frm != nullptr);

    ui->actionPrintSetup->setEnabled(_selectedPrinter != nullptr);
    ui->actionPrint->setEnabled(_selectedPrinter != nullptr && frm && frm->displayMode() == DisplayMode::Data);

    ui->actionUndo->setEnabled(frm != nullptr);
    ui->actionRedo->setEnabled(frm != nullptr);
    ui->actionCut->setEnabled(frm != nullptr);
    ui->actionCopy->setEnabled(frm != nullptr);
    ui->actionPaste->setEnabled(frm != nullptr);
    ui->actionSelectAll->setEnabled(frm != nullptr);
    const bool isScriptActive = (frm && frm->displayMode() == DisplayMode::Script) ||
                                 qobject_cast<ScriptEditorWindow*>(
                                     ui->mdiArea->activeSubWindow() ? ui->mdiArea->activeSubWindow()->widget() : nullptr) != nullptr;
    ui->actionFind->setEnabled(isScriptActive);
    ui->actionReplace->setEnabled(isScriptActive);

    ui->actionDataDefinition->setEnabled(frm != nullptr);
    ui->actionShowData->setEnabled(frm != nullptr);
    ui->actionShowTraffic->setEnabled(frm != nullptr);
    ui->actionShowScript->setEnabled(frm != nullptr);
    ui->actionBinary->setEnabled(frm != nullptr);
    ui->actionUInt16->setEnabled(frm != nullptr);
    ui->actionInt16->setEnabled(frm != nullptr);
    ui->actionInt32->setEnabled(frm != nullptr);
    ui->actionSwappedInt32->setEnabled(frm != nullptr);
    ui->actionUInt32->setEnabled(frm != nullptr);
    ui->actionSwappedUInt32->setEnabled(frm != nullptr);
    ui->actionInt64->setEnabled(frm != nullptr);
    ui->actionSwappedInt64->setEnabled(frm != nullptr);
    ui->actionUInt64->setEnabled(frm != nullptr);
    ui->actionSwappedUInt64->setEnabled(frm != nullptr);
    ui->actionHex->setEnabled(frm != nullptr);
    ui->actionAnsi->setEnabled(frm != nullptr);
    ui->actionHex->setEnabled(frm != nullptr);
    ui->actionFloatingPt->setEnabled(frm != nullptr);
    ui->actionSwappedFP->setEnabled(frm != nullptr);
    ui->actionDblFloat->setEnabled(frm != nullptr);
    ui->actionSwappedDbl->setEnabled(frm != nullptr);
    ui->actionSwapBytes->setEnabled(frm != nullptr);

    ui->actionRawDataLog->setChecked(_trafficLogSubWindow != nullptr);

    ui->actionTextCapture->setEnabled(frm && frm->captureMode() == CaptureMode::Off);
    ui->actionCaptureOff->setEnabled(frm && frm->captureMode() == CaptureMode::TextCapture);

    const bool scriptRunning = frm && isScriptRunningOnSplitPair(frm);
    ui->actionImportScript->setEnabled(frm != nullptr);
    ui->actionRunScript->setEnabled(frm && !scriptRunning && frm->canRunScript());
    ui->actionStopScript->setEnabled(scriptRunning);

    ui->actionTabbedView->setChecked(ui->mdiArea->viewMode() == QMdiArea::TabbedView);
    ui->actionSplitView->setVisible(ui->mdiArea->viewMode() == QMdiArea::TabbedView);
    ui->actionSplitView->setChecked(ui->mdiArea->isSplitView());
    ui->actionToolbar->setChecked(ui->toolBarMain->isVisible());
    ui->actionStatusBar->setChecked(statusBar()->isVisible());
    ui->actionScriptHelp->setChecked(_helpDockWidget->isVisible());
    // Show script help when any script editor is the active window
    const bool scriptEditorActive = qobject_cast<ScriptEditorWindow*>(
        ui->mdiArea->activeSubWindow() ? ui->mdiArea->activeSubWindow()->widget() : nullptr) != nullptr;
    const bool formInScriptMode = frm && frm->displayMode() == DisplayMode::Script;
    ui->actionScriptHelp->setVisible(scriptEditorActive || formInScriptMode);
    ui->actionConsoleOutput->setVisible(true);
    ui->actionConsoleOutput->setChecked(_consoleDockWidget->isVisible());

    ui->actionTile->setEnabled(ui->mdiArea->viewMode() == QMdiArea::SubWindowView);
    ui->actionCascade->setEnabled(ui->mdiArea->viewMode() == QMdiArea::SubWindowView);

    if(frm != nullptr)
    {
        const auto dd = frm->displayDefinition();
        ui->actionUInt16->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionInt16->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionHex->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionInt32->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionSwappedInt32->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionUInt32->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionSwappedUInt32->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionInt64->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionSwappedInt64->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionUInt64->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionSwappedUInt64->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionAnsi->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionFloatingPt->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionSwappedFP->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionDblFloat->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionSwappedDbl->setEnabled(dd.PointType > QModbusDataUnit::Coils);
        ui->actionSwapBytes->setEnabled(dd.PointType > QModbusDataUnit::Coils);

        const auto ddm = frm->dataDisplayMode();
        ui->actionBinary->setChecked(ddm == DataDisplayMode::Binary);
        ui->actionUInt16->setChecked(ddm == DataDisplayMode::UInt16);
        ui->actionInt16->setChecked(ddm == DataDisplayMode::Int16);
        ui->actionInt32->setChecked(ddm == DataDisplayMode::Int32);
        ui->actionSwappedInt32->setChecked(ddm == DataDisplayMode::SwappedInt32);
        ui->actionUInt32->setChecked(ddm == DataDisplayMode::UInt32);
        ui->actionSwappedUInt32->setChecked(ddm == DataDisplayMode::SwappedUInt32);
        ui->actionInt64->setChecked(ddm == DataDisplayMode::Int64);
        ui->actionSwappedInt64->setChecked(ddm == DataDisplayMode::SwappedInt64);
        ui->actionUInt64->setChecked(ddm == DataDisplayMode::UInt64);
        ui->actionSwappedUInt64->setChecked(ddm == DataDisplayMode::SwappedUInt64);
        ui->actionHex->setChecked(ddm == DataDisplayMode::Hex);
        ui->actionAnsi->setChecked(ddm == DataDisplayMode::Ansi);
        ui->actionFloatingPt->setChecked(ddm == DataDisplayMode::FloatingPt);
        ui->actionSwappedFP->setChecked(ddm == DataDisplayMode::SwappedFP);
        ui->actionDblFloat->setChecked(ddm == DataDisplayMode::DblFloat);
        ui->actionSwappedDbl->setChecked(ddm == DataDisplayMode::SwappedDbl);

        const auto byteOrder = frm->byteOrder();
        ui->actionSwapBytes->setChecked(byteOrder == ByteOrder::Swapped);

        ui->actionHexAddresses->setChecked(frm->displayHexAddresses());

        ui->actionShowData->setChecked(true);
        ui->actionShowTraffic->setChecked(_trafficLogSubWindow != nullptr);
        ui->actionShowScript->setChecked(false);

        _projectTree->activateForm(frm);
    }
}

///
/// \brief MainWindow::on_actionNew_triggered
///
void MainWindow::on_actionNew_triggered()
{
    const auto cur = currentMdiChild();
    auto frm = createMdiChild(++_windowCounter);

    const auto& prefs = AppPreferences::instance();

    if(cur) {
        frm->setByteOrder(cur->byteOrder());
        frm->setDataDisplayMode(cur->dataDisplayMode());
        frm->setFont(cur->font());
        frm->setStatusColor(cur->statusColor());
        frm->setBackgroundColor(cur->backgroundColor());
        frm->setForegroundColor(cur->foregroundColor());
    }
    else {
        frm->setFont(prefs.font());
        frm->setBackgroundColor(prefs.backgroundColor());
        frm->setForegroundColor(prefs.foregroundColor());
        frm->setStatusColor(prefs.statusColor());
    }
    frm->setScriptFont(prefs.scriptFont());
    frm->setZoomPercent(prefs.fontZoom());

    // Display definition always comes from application preferences
    {
        auto dd = frm->displayDefinition();
        const auto& prefDd = prefs.displayDefinition();
        dd.ZeroBasedAddress        = prefDd.ZeroBasedAddress;
        dd.HexAddress              = prefDd.HexAddress;
        dd.LeadingZeros            = prefDd.LeadingZeros;
        dd.DataViewColumnsDistance = prefDd.DataViewColumnsDistance;
        dd.AutoscrollLog           = prefDd.AutoscrollLog;
        dd.VerboseLogging          = prefDd.VerboseLogging;
        dd.LogViewLimit            = prefDd.LogViewLimit;
        dd.ScriptCfg               = prefDd.ScriptCfg;
        frm->setDisplayDefinition(dd);
    }

    syncSplitPeerState(frm);
    frm->show();
}

///
/// \brief MainWindow::on_actionClose_triggered
///
void MainWindow::on_actionClose_triggered()
{
    const auto wnd = ui->mdiArea->currentSubWindow();
    if(!wnd) return;

    wnd->close();
}

///
/// \brief MainWindow::on_actionCloseAll_triggered
///
void MainWindow::on_actionCloseAll_triggered()
{
    ui->mdiArea->closeAllSubWindows();
}

///
/// \brief MainWindow::on_actionOpenProject_triggered
///
void MainWindow::on_actionOpenProject_triggered()
{
    QStringList filters;
    filters << tr("Project files (*.msimprj)");
    filters << tr("All files (*)");

    const auto filename = QFileDialog::getOpenFileName(this, QString(), _savePath, filters.join(";;"));
    if(filename.isEmpty()) return;

    _savePath = QFileInfo(filename).absoluteDir().absolutePath();
    loadProject(filename);
}

///
/// \brief MainWindow::on_actionSaveProjectAs_triggered
///
void MainWindow::on_actionSaveProjectAs_triggered()
{
    QStringList filters;
    filters << tr("Project files (*.msimprj)");
    auto filename = QFileDialog::getSaveFileName(this, QString(), _savePath, filters.join(";;"));

    if(filename.isEmpty()) return;

    if(!filename.endsWith(".msimprj", Qt::CaseInsensitive))
        filename.append(".msimprj");

    _savePath = QFileInfo(filename).absoluteDir().absolutePath();
    saveProject(filename);
}

///
/// \brief MainWindow::on_actionCloseProject_triggered
///
void MainWindow::on_actionCloseProject_triggered()
{
    closeProject();
}

///
/// \brief MainWindow::on_actionPrint_triggered
///
void MainWindow::on_actionPrint_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    QPrintDialog dlg(_selectedPrinter.get(), this);
    if(dlg.exec() == QDialog::Accepted)
    {
        frm->print(_selectedPrinter.get());
    }
}

///
/// \brief MainWindow::on_actionPrintSetup_triggered
///
void MainWindow::on_actionPrintSetup_triggered()
{
    DialogPrintSettings dlg(_selectedPrinter.get(), this);
    dlg.exec();
}

///
/// \brief MainWindow::on_actionExit_triggered
///
void MainWindow::on_actionExit_triggered()
{
    close();
}

///
/// \brief MainWindow::on_actionUndo_triggered
///
void MainWindow::on_actionUndo_triggered()
{
    if (auto w = QApplication::focusWidget())
        QMetaObject::invokeMethod(w, "undo");
}

///
/// \brief MainWindow::on_actionRedo_triggered
///
void MainWindow::on_actionRedo_triggered()
{
    if (auto w = QApplication::focusWidget())
        QMetaObject::invokeMethod(w, "redo");
}

///
/// \brief MainWindow::on_actionCut_triggered
///
void MainWindow::on_actionCut_triggered()
{
    if (auto w = QApplication::focusWidget())
        QMetaObject::invokeMethod(w, "cut");
}

///
/// \brief MainWindow::on_actionCopy_triggered
///
void MainWindow::on_actionCopy_triggered()
{
    if (auto w = QApplication::focusWidget())
        QMetaObject::invokeMethod(w, "copy");
}

///
/// \brief MainWindow::on_actionPaste_triggered
///
void MainWindow::on_actionPaste_triggered()
{
    if (auto w = QApplication::focusWidget())
        QMetaObject::invokeMethod(w, "paste");
}

///
/// \brief MainWindow::on_actionSelectAll_triggered
///
void MainWindow::on_actionSelectAll_triggered()
{
    emit selectAll();
}

///
/// \brief MainWindow::on_actionFind_triggered
///
void MainWindow::on_actionFind_triggered()
{
    emit find();
}

///
/// \brief MainWindow::on_actionReplace_triggered
///
void MainWindow::on_actionReplace_triggered()
{
    emit replace();
}

///
/// \brief MainWindow::on_actionPreferences_triggered
///
void MainWindow::on_actionPreferences_triggered()
{
    DialogPreferences dlg(this, this);
    dlg.exec();
}

///
/// \brief MainWindow::applyAutoComplete
/// \param enable
///
void MainWindow::applyAutoComplete(bool enable)
{
    for (auto&& wnd : ui->mdiArea->subWindowList()) {
        if (auto frm = qobject_cast<FormModSim*>(wnd->widget()))
            frm->enableAutoComplete(enable);
    }
}

///
/// \brief MainWindow::applyScriptFont
/// \param font
///
void MainWindow::applyFont(const QFont& font)
{
    for (auto&& wnd : ui->mdiArea->subWindowList()) {
        if (auto frm = qobject_cast<FormModSim*>(wnd->widget()))
            frm->setFont(font);
    }
}

///
/// \brief MainWindow::applyScriptFont
/// \param font
///
void MainWindow::applyScriptFont(const QFont& font)
{
    for (auto&& wnd : ui->mdiArea->subWindowList()) {
        if (auto frm = qobject_cast<FormModSim*>(wnd->widget()))
            frm->setScriptFont(font);
    }
}

///
/// \brief MainWindow::applyColors
/// \param bg
/// \param fg
/// \param status
///
void MainWindow::applyColors(const QColor& bg, const QColor& fg, const QColor& status)
{
    for (auto&& wnd : ui->mdiArea->subWindowList()) {
        if (auto frm = qobject_cast<FormModSim*>(wnd->widget())) {
            frm->setBackgroundColor(bg);
            frm->setForegroundColor(fg);
            frm->setStatusColor(status);
        }
    }
}

///
/// \brief MainWindow::applyCheckForUpdates
/// \param enabled
///
void MainWindow::applyCheckForUpdates(bool enabled)
{
    auto sb = qobject_cast<MainStatusBar*>(statusBar());
    if(sb) sb->setCheckForUpdates(enabled);
}

///
/// \brief MainWindow::applyZoom
/// \param zoomPercent
///
void MainWindow::applyZoom(int zoomPercent)
{
    for (auto&& wnd : ui->mdiArea->subWindowList()) {
        if (auto frm = qobject_cast<FormModSim*>(wnd->widget()))
            frm->setZoomPercent(zoomPercent);
    }
}

///
/// \brief MainWindow::on_connectAction
/// \param type
/// \param port
///
void MainWindow::on_connectAction(ConnectionDetails& cd)
{
    switch(cd.Type)
    {
        case ConnectionType::Tcp:
        {
            DialogSelectServicePort dlg(cd.TcpParams, this);
            if(dlg.exec() == QDialog::Accepted) _mbMultiServer.connectDevice(cd);
        }
        break;

        case ConnectionType::Serial:
        {
            DialogSetupSerialPort dlg(cd.SerialParams, this);
            if(dlg.exec()) _mbMultiServer.connectDevice(cd);
        }
        break;
    }
}

///
/// \brief MainWindow::on_disconnectAction
/// \param type
/// \param port
///
void MainWindow::on_disconnectAction(ConnectionType type, const QString& port)
{
    _mbMultiServer.disconnectDevice(type, port);
}

///
/// \brief MainWindow::on_actionMbDefinitions_triggered
///
void MainWindow::on_actionMbDefinitions_triggered()
{
    DialogModbusDefinitions dlg(_mbMultiServer, this);
    dlg.exec();
}

///
/// \brief MainWindow::on_actionDataDefinition_triggered
///
void MainWindow::on_actionDataDefinition_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    DialogDisplayDefinition dlg(frm->displayDefinition(), this);
    if(dlg.exec() == QDialog::Accepted) {
            frm->setDisplayDefinition(dlg.displayDefinition());
    }
}

///
/// \brief MainWindow::on_actionShowData_triggered
///
void MainWindow::on_actionShowData_triggered()
{
    auto frm = currentMdiChild();
    if(frm) frm->setDisplayMode(DisplayMode::Data);

    updateHelpWidgetState();
}

///
/// \brief MainWindow::on_actionShowTraffic_triggered
///
void MainWindow::on_actionShowTraffic_triggered()
{
    on_actionRawDataLog_triggered(); // open global traffic log window
}

///
/// \brief MainWindow::on_actionShowScript_triggered
/// Opens a new standalone script document.
///
void MainWindow::on_actionShowScript_triggered()
{
    on_actionNewScript_triggered();
}

///
/// \brief MainWindow::on_actionBinary_triggered
///
void MainWindow::on_actionBinary_triggered()
{
    updateDataDisplayMode(DataDisplayMode::Binary);
}

///
/// \brief MainWindow::on_actionUInt16_triggered
///
void MainWindow::on_actionUInt16_triggered()
{
    updateDataDisplayMode(DataDisplayMode::UInt16);
}

///
/// \brief MainWindow::on_actionInt16_triggered
///
void MainWindow::on_actionInt16_triggered()
{
    updateDataDisplayMode(DataDisplayMode::Int16);
}

///
/// \brief MainWindow::on_actionInt32_triggered
///
void MainWindow::on_actionInt32_triggered()
{
    updateDataDisplayMode(DataDisplayMode::Int32);
}

///
/// \brief MainWindow::on_actionSwappedInt32_triggered
///
void MainWindow::on_actionSwappedInt32_triggered()
{
    updateDataDisplayMode(DataDisplayMode::SwappedInt32);
}

///
/// \brief MainWindow::on_actionUInt32_triggered
///
void MainWindow::on_actionUInt32_triggered()
{
    updateDataDisplayMode(DataDisplayMode::UInt32);
}

void MainWindow::on_actionSwappedUInt32_triggered()
{
    updateDataDisplayMode(DataDisplayMode::SwappedUInt32);
}

///
/// \brief MainWindow::on_actionInt64_triggered
///
void MainWindow::on_actionInt64_triggered()
{
    updateDataDisplayMode(DataDisplayMode::Int64);
}

///
/// \brief MainWindow::on_actionSwappedInt64_triggered
///
void MainWindow::on_actionSwappedInt64_triggered()
{
    updateDataDisplayMode(DataDisplayMode::SwappedInt64);
}

///
/// \brief MainWindow::on_actionUInt64_triggered
///
void MainWindow::on_actionUInt64_triggered()
{
    updateDataDisplayMode(DataDisplayMode::UInt64);
}

///
/// \brief MainWindow::on_actionSwappedUInt64_triggered
///
void MainWindow::on_actionSwappedUInt64_triggered()
{
    updateDataDisplayMode(DataDisplayMode::SwappedUInt64);
}

///
/// \brief MainWindow::on_actionHex_triggered
///
void MainWindow::on_actionHex_triggered()
{
    updateDataDisplayMode(DataDisplayMode::Hex);
}

///
/// \brief MainWindow::on_actionAnsi_triggered
///
void MainWindow::on_actionAnsi_triggered()
{
    updateDataDisplayMode(DataDisplayMode::Ansi);
}

///
/// \brief MainWindow::on_actionFloatingPt_triggered
///
void MainWindow::on_actionFloatingPt_triggered()
{
    updateDataDisplayMode(DataDisplayMode::FloatingPt);
}

///
/// \brief MainWindow::on_actionSwappedFP_triggered
///
void MainWindow::on_actionSwappedFP_triggered()
{
    updateDataDisplayMode(DataDisplayMode::SwappedFP);
}

///
/// \brief MainWindow::on_actionSwapBytes_triggered
///
void MainWindow::on_actionSwapBytes_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    switch (frm->byteOrder()) {
    case ByteOrder::Swapped:
        frm->setByteOrder(ByteOrder::Direct);
        break;
    case ByteOrder::Direct:
        frm->setByteOrder(ByteOrder::Swapped);
        break;
    }
}

///
/// \brief MainWindow::on_actionDblFloat_triggered
///
void MainWindow::on_actionDblFloat_triggered()
{
    updateDataDisplayMode(DataDisplayMode::DblFloat);
}

///
/// \brief MainWindow::on_actionSwappedDbl_triggered
///
void MainWindow::on_actionSwappedDbl_triggered()
{
    updateDataDisplayMode(DataDisplayMode::SwappedDbl);
}

///
/// \brief MainWindow::on_actionHexAddresses_triggered
///
void MainWindow::on_actionHexAddresses_triggered()
{
    auto frm = currentMdiChild();
    if(frm) frm->setDisplayHexAddresses(!frm->displayHexAddresses());
}

///
/// \brief MainWindow::on_actionForceCoils_triggered
///
void MainWindow::on_actionForceCoils_triggered()
{
    forceCoils(QModbusDataUnit::Coils);
}

///
/// \brief MainWindow::on_actionForceDiscretes_triggered
///
void MainWindow::on_actionForceDiscretes_triggered()
{
   forceCoils(QModbusDataUnit::DiscreteInputs);
}

///
/// \brief MainWindow::on_actionPresetInputRegs_triggered
///
void MainWindow::on_actionPresetInputRegs_triggered()
{
   presetRegs(QModbusDataUnit::InputRegisters);
}

///
/// \brief MainWindow::on_actionPresetHoldingRegs_triggered
///
void MainWindow::on_actionPresetHoldingRegs_triggered()
{
    presetRegs(QModbusDataUnit::HoldingRegisters);
}

///
/// \brief MainWindow::on_actionMsgParser_triggered
///
void MainWindow::on_actionMsgParser_triggered()
{
    auto frm = currentMdiChild();
    const auto mode = frm ? frm->dataDisplayMode() : DataDisplayMode::Hex;

    auto dlg = new DialogMsgParser(mode, ModbusMessage::Rtu);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    dlg->show();
}

///
/// \brief MainWindow::on_actionRawDataLog_triggered
///
void MainWindow::on_actionRawDataLog_triggered()
{
    if (_trafficLogSubWindow) {
        // already open — bring to front
        ui->mdiArea->setActiveSubWindow(_trafficLogSubWindow);
        return;
    }

    auto trafficWnd = new TrafficLogWindow(_mbMultiServer);
    if (auto frm = currentMdiChild())
        trafficWnd->setDataDisplayMode(frm->dataDisplayMode());
    _trafficLogSubWindow = ui->mdiArea->addSubWindow(trafficWnd);
    _trafficLogSubWindow->setAttribute(Qt::WA_DeleteOnClose);
    _trafficLogSubWindow->setWindowTitle(tr("Traffic"));
    _trafficLogSubWindow->setWindowIcon(QIcon(":/res/actionShowTraffic.png"));

    connect(_trafficLogSubWindow, &QMdiSubWindow::destroyed, this, [this]() {
        _trafficLogSubWindow = nullptr;
    });

    _trafficLogSubWindow->show();
}

///
/// \brief MainWindow::on_actionTextCapture_triggered
///
void MainWindow::on_actionTextCapture_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    auto filename = QFileDialog::getSaveFileName(this, QString(), QString(), "Text files (*.txt)");
    if(!filename.isEmpty())
    {
        if(!filename.endsWith(".txt", Qt::CaseInsensitive)) filename += ".txt";
        frm->startTextCapture(filename);
    }
}

///
/// \brief MainWindow::on_actionCaptureOff_triggered
///
void MainWindow::on_actionCaptureOff_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    frm->stopTextCapture();
}

///
/// \brief MainWindow::on_actionResetCtrs_triggered
///
void MainWindow::on_actionResetCtrs_triggered()
{
    auto frm = currentMdiChild();
    if(!frm)
        return;

    frm->resetCtrs();

    if(_splitDisableInProgress || !isSplitTabbedView())
        return;

    if(auto peer = splitPeer(frm))
        peer->resetCtrs();
}

///
/// \brief MainWindow::on_actionTabbedView_triggered
///
void MainWindow::on_actionTabbedView_triggered()
{
    if(ui->mdiArea->viewMode() == QMdiArea::SubWindowView) {
        setViewMode(QMdiArea::TabbedView);
    }
    else {
        setViewMode(QMdiArea::SubWindowView);
    }
}

///
/// \brief MainWindow::on_actionSplitView_triggered
///
void MainWindow::on_actionSplitView_triggered()
{
    ui->mdiArea->toggleVerticalSplit();
}

///
/// \brief MainWindow::setViewMode
/// \param mode
///
void MainWindow::setViewMode(QMdiArea::ViewMode mode)
{
    ui->mdiArea->setViewMode(mode);
    if(auto tabBar = ui->mdiArea->tabBar()) {
        connect(tabBar, &QTabBar::tabBarDoubleClicked, ui->actionDataDefinition, &QAction::triggered);
    }
}

///
/// \brief MainWindow::updateHelpWidgetState
///
void MainWindow::updateHelpWidgetState()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    switch(frm->displayMode())
    {
        case DisplayMode::Data:
        case DisplayMode::Traffic:
            if(_helpDockWidget->isVisible() &&
                !_helpDockWidget->isFloating())
            {
                _helpDockWidget->setProperty("WasShown", true);
                _helpDockWidget->setVisible(false);
            }
        break;

        case DisplayMode::Script:
            if(!_helpDockWidget->isVisible() &&
                _helpDockWidget->property("WasShown").toBool())
            {
                _helpDockWidget->setVisible(true);
            }
        break;
    }
}

///
/// \brief MainWindow::on_actionToolbar_triggered
///
void MainWindow::on_actionToolbar_triggered()
{
    ui->toolBarMain->setVisible(!ui->toolBarMain->isVisible());
}

///
/// \brief MainWindow::on_actionStatusBar_triggered
///
void MainWindow::on_actionStatusBar_triggered()
{
    statusBar()->setVisible(!statusBar()->isVisible());
}

///
/// \brief MainWindow::on_actionScriptHelp_triggered
///
void MainWindow::on_actionScriptHelp_triggered()
{
    _helpDockWidget->setVisible(!_helpDockWidget->isVisible());
}

///
/// \brief MainWindow::on_actionConsoleOutput_triggered
///
void MainWindow::on_actionConsoleOutput_triggered()
{
    _consoleDockWidget->setVisible(!_consoleDockWidget->isVisible());
}

///
/// \brief MainWindow::on_actionCascade_triggered
///
void MainWindow::on_actionCascade_triggered()
{
    ui->mdiArea->cascadeSubWindows();
}

///
/// \brief MainWindow::on_actionTile_triggered
///
void MainWindow::on_actionTile_triggered()
{
    ui->mdiArea->tileSubWindows();
}

///
/// \brief MainWindow::on_actionWindows_triggered
///
void MainWindow::on_actionWindows_triggered()
{
    DialogWindowsManager dlg(_windowActionList->actionList(), nullptr, this);
    dlg.exec();
}

///
/// \brief MainWindow::on_actionAbout_triggered
///
void MainWindow::on_actionAbout_triggered()
{
    DialogAbout dlg(this);
    dlg.exec();
}

///
/// \brief MainWindow::on_actionNewScript_triggered
///
void MainWindow::on_actionNewScript_triggered()
{
    const QString name = tr("Script%1").arg(++_scriptCounter);
    auto doc = createStandaloneScript(name);
    openScriptEditor(doc);
}

///
/// \brief MainWindow::on_actionImportScript_triggered
///
void MainWindow::on_actionImportScript_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    const auto filename = QFileDialog::getOpenFileName(this, QString(), QString(), tr("JavaScript files (*.js);;All files (*)"));
    if(filename.isEmpty()) return;

    QFile file(filename);
    if(!file.open(QFile::ReadOnly | QFile::Text)) return;

    frm->setScript(QTextStream(&file).readAll());
    frm->setDisplayMode(DisplayMode::Script);
}

///
/// \brief MainWindow::on_actionRunScript_triggered
///
void MainWindow::on_actionRunScript_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    if(isScriptRunningOnSplitPair(frm))
        return;

    frm->runScript();
}

///
/// \brief MainWindow::on_actionStopScript_triggered
///
void MainWindow::on_actionStopScript_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    if(frm->canStopScript()) {
        frm->stopScript();
        return;
    }

    if(auto peer = splitPeer(frm); peer && peer->canStopScript())
        peer->stopScript();
}

///
/// \brief MainWindow::on_searchText
/// \param text
///
void MainWindow::on_searchText(const QString& text)
{
   emit search(text);
}

///
/// \brief MainWindow::on_connectionError
/// \param error
///
void MainWindow::on_connectionError(const QString& error)
{
    QMessageBox::warning(this, windowTitle(), error);
}

///
/// \brief MainWindow::updateMenuWindow
///
void MainWindow::updateMenuWindow()
{
    const auto activeWnd = ui->mdiArea->activeSubWindow();
    for(auto&& wnd : ui->mdiArea->subWindowList())
    {
        wnd->setProperty("isActive", wnd == activeWnd);
    }
    _windowActionList->update();
}

///
/// \brief MainWindow::windowActivate
/// \param wnd
///
void MainWindow::windowActivate(QMdiSubWindow* wnd)
{
    if(wnd) ui->mdiArea->setActiveSubWindow(wnd);
}

///
/// \brief MainWindow::setCodepage
/// \param name
///
void MainWindow::setCodepage(const QString& name)
{
    auto frm = currentMdiChild();
    if(!frm) return;

    frm->setCodepage(name);
}


///
/// \brief MainWindow::updateDisplayMode
/// \param mode
///
void MainWindow::updateDataDisplayMode(DataDisplayMode mode)
{
    auto frm = currentMdiChild();
    if(frm) frm->setDataDisplayMode(mode);

    if (_trafficLogSubWindow) {
        auto trafficWnd = qobject_cast<TrafficLogWindow*>(_trafficLogSubWindow->widget());
        if (trafficWnd) trafficWnd->setDataDisplayMode(mode);
    }
}

///
/// \brief MainWindow::forceCoils
/// \param type
///
void MainWindow::forceCoils(QModbusDataUnit::RegisterType type)
{
    auto frm = currentMdiChild();
    if(!frm) return;

    const auto dd = frm->displayDefinition();
    SetupPresetParams presetParams = { dd.DeviceId, dd.PointAddress, dd.Length, dd.ZeroBasedAddress, dd.AddrSpace, dd.LeadingZeros };

    {
        DialogSetupPresetData dlg(presetParams, type, dd, this);
        if(dlg.exec() != QDialog::Accepted) return;
    }

    ModbusWriteParams params;
    params.DeviceId = presetParams.DeviceId;
    params.Address = presetParams.PointAddress;
    params.ZeroBasedAddress = dd.ZeroBasedAddress;
    params.AddrSpace = dd.AddrSpace;

    if(dd.PointType == type)
    {
        const auto data = _mbMultiServer.data(presetParams.DeviceId, type, presetParams.PointAddress - (dd.ZeroBasedAddress ? 0 : 1), presetParams.Length);
        params.Value = QVariant::fromValue(data.values());
    }

    DialogForceMultipleCoils dlg(params, type, presetParams.Length, dd, this);
    if(dlg.exec() == QDialog::Accepted)
    {
        _mbMultiServer.writeRegister(type, params);
    }
}

///
/// \brief MainWindow::presetRegs
/// \param type
///
void MainWindow::presetRegs(QModbusDataUnit::RegisterType type)
{
    auto frm = currentMdiChild();
    if(!frm) return;

    const auto dd = frm->displayDefinition();
    SetupPresetParams presetParams = { dd.DeviceId, dd.PointAddress, dd.Length, dd.ZeroBasedAddress, dd.AddrSpace, dd.LeadingZeros };

    {
        DialogSetupPresetData dlg(presetParams, type, dd, this);
        if(dlg.exec() != QDialog::Accepted) return;
    }

    ModbusWriteParams params;
    params.DeviceId = presetParams.DeviceId;
    params.Address = presetParams.PointAddress;
    params.DisplayMode = frm->dataDisplayMode();
    params.Order = frm->byteOrder();
    params.Codepage = frm->codepage();
    params.ZeroBasedAddress = dd.ZeroBasedAddress;
    params.AddrSpace = dd.AddrSpace;

    if(dd.PointType == type)
    {
        const auto data = _mbMultiServer.data(presetParams.DeviceId, type, presetParams.PointAddress - (dd.ZeroBasedAddress ? 0 : 1), presetParams.Length);
        params.Value = QVariant::fromValue(data.values());
    }

    DialogForceMultipleRegisters dlg(params, type, presetParams.Length, dd, this);
    if(dlg.exec() == QDialog::Accepted)
    {
        _mbMultiServer.writeRegister(type, params);
    }
}

///
/// \brief MainWindow::createMdiChildOnArea
/// \param id
/// \param area
/// \param addToWindowList
/// \return
///
FormModSim* MainWindow::createMdiChildOnArea(int id, MdiArea* area, bool addToWindowList)
{
    if(!area)
        return nullptr;

    auto frm = new FormModSim(id, _mbMultiServer, _dataSimulator, this);
    frm->enableAutoComplete(AppPreferences::instance().codeAutoComplete());

    auto wnd = area->addSubWindow(frm);
    if(!wnd)
    {
        frm->deleteLater();
        return nullptr;
    }

    wnd->installEventFilter(this);
    wnd->setAttribute(Qt::WA_DeleteOnClose, true);
    setupMdiChild(frm, wnd, addToWindowList);

    return frm;
}

///
/// \brief MainWindow::setupMdiChild
/// \param frm
/// \param wnd
/// \param addToWindowList
///
void MainWindow::setupMdiChild(FormModSim* frm, QMdiSubWindow* wnd, bool addToWindowList)
{
    if(!frm || !wnd)
        return;

    auto updateCodepage = [this](const QString& name)
    {
        _ansiMenu->selectCodepage(name);
    };

    connect(frm, &FormModSim::codepageChanged, this, [updateCodepage](const QString& name)
    {
        updateCodepage(name);
    });

    connect(wnd, &QMdiSubWindow::windowStateChanged, this,
            [this, frm, updateCodepage](Qt::WindowStates, Qt::WindowStates newState)
    {
        switch(newState & ~Qt::WindowMaximized & ~Qt::WindowMinimized)
        {
            case Qt::WindowActive:
                updateHelpWidgetState();
                updateCodepage(frm->codepage());
                frm->connectEditSlots();
            break;

            case Qt::WindowNoState:
                frm->disconnectEditSlots();
            break;
        }
    });

    connect(frm, &FormModSim::pointTypeChanged, this, [frm](QModbusDataUnit::RegisterType type)
    {
        switch(type)
        {
            case QModbusDataUnit::Coils:
            case QModbusDataUnit::DiscreteInputs:
                frm->setProperty("PrevDataDisplayMode", QVariant::fromValue(frm->dataDisplayMode()));
                frm->setDataDisplayMode(DataDisplayMode::Binary);
                break;

            case QModbusDataUnit::HoldingRegisters:
            case QModbusDataUnit::InputRegisters:
            {
                const auto mode = frm->property("PrevDataDisplayMode");
                if(mode.isValid())
                    frm->setDataDisplayMode(mode.value<DataDisplayMode>());
            }
            break;

            default:
                break;
        }
    });

    connect(frm, &FormModSim::definitionChanged, this, [this, frm]()
    {
        syncSplitPeerDisplayDefinition(frm);
    });

    connect(frm, &FormModSim::showed, this, [this, frm]
    {
        // Activate whichever subwindow currently holds this form
        for (auto w : ui->mdiArea->subWindowList()) {
            if (qobject_cast<FormModSim*>(w->widget()) == frm) {
                windowActivate(w);
                break;
            }
        }
    });

    connect(frm, &FormModSim::captureError, this, [this](const QString& error)
    {
        QMessageBox::critical(this, windowTitle(), tr("Capture Error:\r\n%1").arg(error));
    });

    connect(frm, &FormModSim::doubleClicked, this, [this]()
    {
        ui->actionDataDefinition->trigger();
    });

    connect(frm, &FormModSim::helpContextRequested, this, [this](const QString& helpKey)
    {
        _helpDockWidget->setVisible(true);
        if(!helpKey.isEmpty()) {
            _helpWidget->showHelp(helpKey);
        }
    });

    connect(frm, &FormModSim::statisticCtrsReseted, this, [this, frm]()
    {
        if(_splitDisableInProgress || !isSplitTabbedView())
            return;

        if(auto peer = splitPeer(frm))
            peer->resetCtrs();
    });

    connect(frm, &FormModSim::statisticLogStateChanged, this, [this, frm](LogViewState state)
    {
        if(_splitDisableInProgress || !isSplitTabbedView())
            return;

        if(auto peer = splitPeer(frm))
            peer->setLogViewState(state);
    });

    connect(frm, &FormModSim::consoleMessage, this, [this](const QString& source, const QString& text, ConsoleOutput::MessageType type) {
        _consoleDockWidget->setVisible(true);
        _globalConsole->addMessage(text, type, source);
    });

    connect(frm, &FormModSim::scriptRunning, this, [this, frm]()
    {
        frm->setProperty(kSplitScriptRunning, true);
        if(auto peer = splitPeer(frm))
            peer->setProperty(kSplitScriptRunning, true);
        updateSplitPairScriptIcons(frm);
    });

    connect(frm, &FormModSim::scriptStopped, this, [this, frm]()
    {
        frm->setProperty(kSplitScriptRunning, false);
        if(auto peer = splitPeer(frm))
            peer->setProperty(kSplitScriptRunning, false);
        updateSplitPairScriptIcons(frm);
    });

    connect(frm, &FormModSim::closing, this, [this, frm]()
    {
        const int peerId = frm->property(kSplitMirrorPeerId).toInt();
        if(peerId <= 0)
            return;

        if(auto peer = findMdiChild(peerId))
            peer->setProperty(kSplitMirrorPeerId, QVariant());

        frm->setProperty(kSplitMirrorPeerId, QVariant());
    });

    connect(wnd, &QObject::destroyed, this, [this]() {
        resetSplitViewIfEmpty();
    });

    if(addToWindowList) {
        _windowActionList->addWindow(wnd);
        _projectTree->addForm(frm);
    }
}

///
/// \brief MainWindow::cloneMdiChildState
/// \param source
/// \param target
/// \return
///
bool MainWindow::cloneMdiChildState(FormModSim* source, FormModSim* target) const
{
    if(!source || !target)
        return false;

    QByteArray xmlBuffer;
    QBuffer writeBuffer(&xmlBuffer);
    if(!writeBuffer.open(QIODevice::WriteOnly))
        return false;

    QXmlStreamWriter writer(&writeBuffer);
    writer.writeStartDocument();
    writer << source;
    writer.writeEndDocument();
    writeBuffer.close();

    QBuffer readBuffer(&xmlBuffer);
    if(!readBuffer.open(QIODevice::ReadOnly))
        return false;

    QXmlStreamReader reader(&readBuffer);
    if(!reader.readNextStartElement() || reader.name() != QLatin1String("FormModSim"))
        return false;

    reader >> target;
    if(reader.hasError())
        return false;

    target->setFilename(source->filename());
    return true;
}

///
/// \brief MainWindow::findMdiChildInArea
/// \param area
/// \param id
/// \return
///
FormModSim* MainWindow::findMdiChildInArea(MdiArea* area, int id) const
{
    if(!area)
        return nullptr;

    for(auto&& wnd : area->localSubWindowList())
    {
        const auto frm = qobject_cast<FormModSim*>(wnd->widget());
        if(frm && frm->formId() == id)
            return frm;
    }

    return nullptr;
}

///
/// \brief MainWindow::splitPeer
/// \param frm
/// \return
///
FormModSim* MainWindow::splitPeer(FormModSim* frm) const
{
    if(!frm)
        return nullptr;

    const int peerId = frm->property(kSplitMirrorPeerId).toInt();
    if(peerId <= 0)
        return nullptr;

    auto peer = findMdiChild(peerId);
    return (peer && peer != frm) ? peer : nullptr;
}

///
/// \brief MainWindow::isScriptRunningOnSplitPair
/// \param frm
/// \return
///
bool MainWindow::isScriptRunningOnSplitPair(FormModSim* frm) const
{
    if(!frm)
        return false;

    const bool formRunning = frm->canStopScript() || frm->property(kSplitScriptRunning).toBool();
    if(formRunning)
        return true;

    if(auto peer = splitPeer(frm)) {
        const bool peerRunning = peer->canStopScript() || peer->property(kSplitScriptRunning).toBool();
        return peerRunning;
    }

    return false;
}

///
/// \brief MainWindow::updateSplitPairScriptIcons
/// \param frm
///
void MainWindow::updateSplitPairScriptIcons(FormModSim* frm)
{
    if(!frm)
        return;

    auto applyIcon = [this](FormModSim* target, bool running)
    {
        if(!target)
            return;

        auto targetWnd = qobject_cast<QMdiSubWindow*>(target->parentWidget());
        if(!targetWnd)
            return;

        if(running)
            crossFadeWindowIcon(targetWnd, targetWnd->windowIcon(), ui->actionRunScript->icon());
        else
            crossFadeWindowIcon(targetWnd, targetWnd->windowIcon(), windowIcon());
    };

    auto peer = splitPeer(frm);
    const bool periodicMode = frm->scriptSettings().Mode == RunMode::Periodically ||
                              (peer && peer->scriptSettings().Mode == RunMode::Periodically);
    const bool running = periodicMode && isScriptRunningOnSplitPair(frm);

    applyIcon(frm, running);
    applyIcon(peer, running);
}

///
/// \brief MainWindow::splitSecondaryArea
/// \return
///
MdiArea* MainWindow::splitSecondaryArea() const
{
    return ui->mdiArea->secondaryArea();
}

///
/// \brief MainWindow::isSplitTabbedView
/// \return
///
bool MainWindow::isSplitTabbedView() const
{
    return ui->mdiArea->viewMode() == QMdiArea::TabbedView &&
           ui->mdiArea->isSplitView() &&
           splitSecondaryArea() != nullptr;
}

///
/// \brief MainWindow::resetSplitViewIfEmpty
///
void MainWindow::resetSplitViewIfEmpty()
{
    if(_splitDisableInProgress || !isSplitTabbedView())
        return;

    auto secondary = splitSecondaryArea();
    if(!secondary || !secondary->localSubWindowList().isEmpty())
        return;

    ui->mdiArea->toggleVerticalSplit();
}

///
/// \brief MainWindow::ensureSplitMirrorForForm
/// \param frm
///
void MainWindow::ensureSplitMirrorForForm(FormModSim* frm)
{
    if(!frm || !isSplitTabbedView())
        return;

    auto secondary = splitSecondaryArea();
    if(!secondary)
        return;

    MdiArea* ownerArea = nullptr;
    for(auto&& wnd : ui->mdiArea->localSubWindowList()) {
        if(wnd && wnd->widget() == frm) {
            ownerArea = ui->mdiArea->primaryArea();
            break;
        }
    }
    if(!ownerArea) {
        for(auto&& wnd : secondary->localSubWindowList()) {
            if(wnd && wnd->widget() == frm) {
                ownerArea = secondary;
                break;
            }
        }
    }

    if(!ownerArea)
        return;

    const int peerId = frm->property(kSplitMirrorPeerId).toInt();
    MdiArea* targetArea = ownerArea == ui->mdiArea->primaryArea() ? secondary : ui->mdiArea->primaryArea();
    if(peerId > 0) {
        if(auto existingPeer = findMdiChildInArea(targetArea, peerId)) {
            if(auto* doc = frm->scriptDocument()) {
                if(doc->parent() != this)
                    doc->setParent(this);
                existingPeer->setScriptDocument(doc);
            }
            updateSplitPairScriptIcons(frm);
            return;
        }
    }

    int mirrorId = _windowCounter + 1;
    while(findMdiChild(mirrorId))
        ++mirrorId;
    _windowCounter = mirrorId;

    const bool addToWindowList = (targetArea == ui->mdiArea->primaryArea());
    auto mirror = createMdiChildOnArea(mirrorId, targetArea, addToWindowList);
    if(!mirror)
        return;

    cloneMdiChildState(frm, mirror);
    mirror->setStatisticCounters(frm->requestCount(), frm->responseCount());
    mirror->setLogViewState(frm->logViewState());
    if(auto* doc = frm->scriptDocument()) {
        if(doc->parent() != this)
            doc->setParent(this);
        mirror->setScriptDocument(doc);
    }
    frm->setProperty(kSplitMirrorPeerId, mirror->formId());
    mirror->setProperty(kSplitMirrorPeerId, frm->formId());
    mirror->setProperty(kSplitScriptRunning, frm->property(kSplitScriptRunning));
    updateSplitPairScriptIcons(frm);
    mirror->show();
}

///
/// \brief MainWindow::syncSplitPeerDisplayDefinition
/// \param frm
///
void MainWindow::syncSplitPeerDisplayDefinition(FormModSim* frm)
{
    if(!frm || !isSplitTabbedView() || _splitDisplayDefinitionSyncInProgress)
        return;

    const int peerId = frm->property(kSplitMirrorPeerId).toInt();
    if(peerId <= 0)
        return;

    auto peer = findMdiChild(peerId);
    if(!peer || peer == frm)
        return;

    _splitDisplayDefinitionSyncInProgress = true;
    peer->setDisplayDefinition(frm->displayDefinition());
    _splitDisplayDefinitionSyncInProgress = false;
}

///
/// \brief MainWindow::syncSplitPeerState
/// \param frm
///
void MainWindow::syncSplitPeerState(FormModSim* frm)
{
    if(!frm || !isSplitTabbedView())
        return;

    ensureSplitMirrorForForm(frm);

    const int peerId = frm->property(kSplitMirrorPeerId).toInt();
    if(peerId <= 0)
        return;

    auto peer = findMdiChild(peerId);
    if(!peer || peer == frm)
        return;

    cloneMdiChildState(frm, peer);
}

///
/// \brief MainWindow::syncSplitForms
///
void MainWindow::syncSplitForms()
{
    if(!isSplitTabbedView())
        return;

    auto secondary = splitSecondaryArea();
    if(!secondary)
        return;

    QList<FormModSim*> primaryForms;
    for(auto&& wnd : ui->mdiArea->localSubWindowList()) {
        if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
            primaryForms.append(frm);
    }

    QList<FormModSim*> secondaryForms;
    for(auto&& wnd : secondary->localSubWindowList()) {
        if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
            secondaryForms.append(frm);
    }

    for(auto* frm : primaryForms)
        ensureSplitMirrorForForm(frm);

    for(auto* frm : secondaryForms)
        ensureSplitMirrorForForm(frm);
}

///
/// \brief MainWindow::clearSplitMirrorsFromSecondary
///
void MainWindow::clearSplitMirrorsFromSecondary()
{
    auto secondary = splitSecondaryArea();
    if(!secondary)
        return;

    const auto secondaryWindows = secondary->localSubWindowList();
    for(auto&& wnd : secondaryWindows)
    {
        auto frm = qobject_cast<FormModSim*>(wnd->widget());
        if(!frm)
            continue;

        const int peerId = frm->property(kSplitMirrorPeerId).toInt();
        auto peer = findMdiChildInArea(ui->mdiArea->primaryArea(), peerId);
        if(!peer)
            continue;

        peer->setProperty(kSplitMirrorPeerId, QVariant());
        wnd->close();
    }
}

///
/// \brief MainWindow::createMdiChild
/// \param id
/// \return
///
FormModSim* MainWindow::createMdiChild(int id)
{
    while(findMdiChild(id))
        ++id;

    _windowCounter = qMax(_windowCounter, id);
    auto frm = createMdiChildOnArea(id, ui->mdiArea->primaryArea(), true);
    if(frm)
        ensureSplitMirrorForForm(frm);

    return frm;
}

///
/// \brief MainWindow::currentMdiChild
/// \return
///
FormModSim* MainWindow::currentMdiChild() const
{
    auto wnd = ui->mdiArea->currentSubWindow();
    if(!wnd && ui->mdiArea->viewMode() == QMdiArea::TabbedView) {
        // Qt5: d->active may still be null on the first event loop iteration
        // because _q_currentTabChanged is posted via QueuedConnection while
        // awake() fires before posted events are processed. Read the tab bar
        // directly to get the correct subwindow.
        const auto tabBar = ui->mdiArea->tabBar();
        const auto list = ui->mdiArea->subWindowList();
        const auto idx = tabBar ? tabBar->currentIndex() : -1;
        if(idx >= 0 && idx < list.size())
            wnd = list.at(idx);
    }
    return wnd ? qobject_cast<FormModSim*>(wnd->widget()) : nullptr;
}

///
/// \brief MainWindow::findMdiChild
/// \param num
/// \return
///
FormModSim* MainWindow::findMdiChild(int id) const
{
    for(auto&& wnd : ui->mdiArea->subWindowList())
    {
        const auto frm = qobject_cast<FormModSim*>(wnd->widget());
        if(frm && frm->formId() == id) return frm;
    }
    return nullptr;
}

///
/// \brief MainWindow::firstMdiChild
/// \return
///
FormModSim* MainWindow::firstMdiChild() const
{
    for(auto&& wnd : ui->mdiArea->subWindowList())
        return qobject_cast<FormModSim*>(wnd->widget());

    return nullptr;
}

///
/// \brief MainWindow::loadProject
/// \param filename
///
void MainWindow::loadProject(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QFile::ReadOnly))
        return;

    ModbusDefinitions defs;
    QList<ConnectionDetails> conns;
    QMdiArea::ViewMode viewMode = QMdiArea::TabbedView;
    bool splitView = false;
    QString activePrimaryWin;
    QString activeSecWin;
    struct MirrorState { DisplayMode displayMode = DisplayMode::Data; int scriptCursorPos = -1; int scriptScrollPos = -1; };
    QMap<int, MirrorState> mirrorStates;

    QXmlStreamReader xml(&file);
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("OpenModSim")) {
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("AppPreferences")) {
                    AppPreferences::instance().loadXml(xml);
                }
                else if (xml.name() == QLatin1String("ViewSettings")) {
                    const auto attrs = xml.attributes();
                    viewMode = (QMdiArea::ViewMode)qBound(0, attrs.value("ViewMode").toInt(), 1);
                    splitView = attrs.value("SplitView").toInt() != 0;
                    activePrimaryWin = attrs.value("ActivePrimaryWindow").toString();
                    activeSecWin = attrs.value("ActiveSecondaryWindow").toString();
                    xml.skipCurrentElement();
                }
                else if (xml.name() == QLatin1String("SecondaryPanel")) {
                    while(xml.readNextStartElement())
                    {
                        if(xml.name() == QLatin1String("Mirror"))
                        {
                            const auto attrs = xml.attributes();
                            const int primaryId = attrs.value("PrimaryId").toInt();
                            MirrorState ms;
                            ms.displayMode = (DisplayMode)attrs.value("DisplayMode").toInt();
                            if(attrs.hasAttribute("ScriptCursorPos"))
                                ms.scriptCursorPos = attrs.value("ScriptCursorPos").toInt();
                            if(attrs.hasAttribute("ScriptScrollPos"))
                                ms.scriptScrollPos = attrs.value("ScriptScrollPos").toInt();
                            mirrorStates[primaryId] = ms;
                        }
                        xml.skipCurrentElement();
                    }
                }
                else if (xml.name() == QLatin1String("ModbusDefinitions")) {
                    xml >> defs;
                }
                else if (xml.name() == QLatin1String("Connections")) {
                    while (xml.readNextStartElement()) {
                        if (xml.name() == QLatin1String("ConnectionDetails")) {
                            ConnectionDetails cd;
                            xml >> cd;
                            conns.append(cd);
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                }
                else if (xml.name() == QLatin1String("Forms")) {
                    ui->mdiArea->closeAllSubWindows();
                    // Clean up forms that were already closed (hidden)
                    for (auto frm : _closedForms) {
                        _projectTree->removeForm(frm);
                        delete frm;
                    }
                    _closedForms.clear();
                    while (xml.readNextStartElement()) {
                        if (xml.name() == QLatin1String("FormModSim")) {
                            auto frm = createMdiChild(++_windowCounter);
                            if (frm) {
                                xml >> frm;
                                syncSplitPeerState(frm);
                                frm->show();
                            }
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                }
                else if (xml.name() == QLatin1String("Scripts")) {
                    // Remove existing standalone scripts
                    for (auto doc : _standaloneScripts)
                        _projectTree->removeScript(doc);
                    qDeleteAll(_standaloneScripts);
                    _standaloneScripts.clear();

                    while (xml.readNextStartElement()) {
                        if (xml.name() == QLatin1String("ScriptDocument")) {
                            auto doc = new ScriptDocument(QString(), this);
                            xml >> doc;
                            if (doc->name().isEmpty())
                                doc->setName(tr("Script%1").arg(++_scriptCounter));
                            _standaloneScripts.append(doc);
                            _projectTree->addScript(doc);
                            // Auto-run if RunOnStartup
                            if (doc->settings().RunOnStartup)
                                openScriptEditor(doc)->runScript(doc->settings().Mode, doc->settings().Interval);
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                }
                else {
                    xml.skipCurrentElement();
                }
            }
        }
        else {
            xml.skipCurrentElement();
        }
    }

    setViewMode(viewMode);

    auto menu = qobject_cast<MenuConnect*>(ui->actionConnect->menu());
    menu->updateConnectionDetails(conns);

    // setup definitions
    _mbMultiServer.setModbusDefinitions(defs);

    for(auto&& cd : conns)
    {
        if(menu->canConnect(cd))
            _mbMultiServer.connectDevice(cd);
    }

    if(!activePrimaryWin.isEmpty())
        if(auto primary = ui->mdiArea->primaryArea())
            for(auto&& wnd : primary->localSubWindowList())
                if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
                    if(frm->windowTitle() == activePrimaryWin)
                    {
                        primary->setActiveSubWindow(wnd);
                        break;
                    }

    if(splitView)
    {
        ui->mdiArea->setSplitViewEnabled(true);

        for(auto it = mirrorStates.begin(); it != mirrorStates.end(); ++it)
            if(auto frm = findMdiChild(it.key()))
                if(auto mirror = splitPeer(frm))
                {
                    mirror->setDisplayMode(it.value().displayMode);
                    if(it.value().scriptCursorPos >= 0)
                        mirror->setScriptCursorPosition(it.value().scriptCursorPos);
                    if(it.value().scriptScrollPos >= 0)
                        mirror->setScriptScrollPosition(it.value().scriptScrollPos);
                }

        if(!activeSecWin.isEmpty())
            if(auto secondary = splitSecondaryArea())
                for(auto&& wnd : secondary->localSubWindowList())
                    if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
                        if(frm->windowTitle() == activeSecWin)
                        {
                            secondary->setActiveSubWindow(wnd);
                            break;
                        }
    }
}

///
/// \brief MainWindow::saveProject
/// \param filename
///
void MainWindow::saveProject(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QFile::WriteOnly))
        return;

    QXmlStreamWriter w(&file);
    w.setAutoFormatting(true);

    w.writeStartDocument();
    w.writeStartElement("OpenModSim");
    w.writeAttribute("Version", qApp->applicationVersion());

    AppPreferences::instance().saveXml(w);

    w << _mbMultiServer.getModbusDefinitions();

    w.writeStartElement("Connections");
    for(auto&& cd : _mbMultiServer.connections()) {
        w << cd;
    }
    w.writeEndElement(); // Connections

    w.writeStartElement("ViewSettings");
    w.writeAttribute("ViewMode", QString::number(ui->mdiArea->viewMode()));
    w.writeAttribute("SplitView", ui->mdiArea->isSplitView() ? "1" : "0");
    if(auto primary = ui->mdiArea->primaryArea())
        if(auto wnd = primary->activeSubWindow())
            if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
                w.writeAttribute("ActivePrimaryWindow", frm->windowTitle());
    if(isSplitTabbedView())
        if(auto secondary = splitSecondaryArea())
            if(auto wnd = secondary->activeSubWindow())
                if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
                    w.writeAttribute("ActiveSecondaryWindow", frm->windowTitle());
    w.writeEndElement(); // ViewSettings

    w.writeStartElement("Forms");
    for(auto&& wnd : ui->mdiArea->localSubWindowList()) {
        w << qobject_cast<FormModSim*>(wnd->widget());
    }
    // Also save forms that are closed (hidden in project tree)
    for (auto frm : _closedForms) {
        w << frm;
    }
    w.writeEndElement(); // Forms

    w.writeStartElement("Scripts");
    for (auto doc : _standaloneScripts) {
        w << doc;
    }
    w.writeEndElement(); // Scripts

    if(isSplitTabbedView())
    {
        w.writeStartElement("SecondaryPanel");
        for(auto&& wnd : ui->mdiArea->localSubWindowList())
        {
            auto frm = qobject_cast<FormModSim*>(wnd->widget());
            if(!frm) continue;
            if(auto mirror = splitPeer(frm))
            {
                w.writeStartElement("Mirror");
                w.writeAttribute("PrimaryId", QString::number(frm->formId()));
                w.writeAttribute("DisplayMode", QString::number((int)mirror->displayMode()));
                w.writeAttribute("ScriptCursorPos", QString::number(mirror->scriptCursorPosition()));
                w.writeAttribute("ScriptScrollPos", QString::number(mirror->scriptScrollPosition()));
                w.writeEndElement(); // Mirror
            }
        }
        w.writeEndElement(); // SecondaryPanel
    }

    w.writeEndElement(); // OpenModSim
    w.writeEndDocument();
}

///
/// \brief MainWindow::closeMdiChild
/// \param frm
///
void MainWindow::closeMdiChild(FormModSim* frm)
{
    for(auto&& wnd : ui->mdiArea->subWindowList()) {
        const auto f = qobject_cast<FormModSim*>(wnd->widget());
        if(f == frm) wnd->close();
    }
}

///
/// \brief MainWindow::createStandaloneScript
///
ScriptDocument* MainWindow::createStandaloneScript(const QString& name)
{
    auto doc = new ScriptDocument(name, this);
    _standaloneScripts.append(doc);
    _projectTree->addScript(doc);
    return doc;
}

///
/// \brief MainWindow::openScriptEditor
/// Opens (or focuses) the MDI editor window for a standalone script.
///
ScriptEditorWindow* MainWindow::openScriptEditor(ScriptDocument* doc)
{
    // If already open, bring to front
    if (auto existing = findScriptEditor(doc)) {
        for (auto wnd : ui->mdiArea->subWindowList()) {
            if (wnd->widget() == existing) {
                ui->mdiArea->setActiveSubWindow(wnd);
                return existing;
            }
        }
    }

    auto editor = new ScriptEditorWindow(doc, &_mbMultiServer);
    auto wnd = ui->mdiArea->addSubWindow(editor);
    wnd->setAttribute(Qt::WA_DeleteOnClose);
    wnd->setWindowTitle(doc->name());
    wnd->setWindowIcon(QIcon(":/res/actionShowScript.png"));

    connect(doc, &ScriptDocument::nameChanged, wnd, &QMdiSubWindow::setWindowTitle);

    connect(editor, &ScriptEditorWindow::scriptRunning, this, [this, doc]() {
        _projectTree->setScriptRunning(doc, true);
    });
    connect(editor, &ScriptEditorWindow::scriptStopped, this, [this, doc]() {
        _projectTree->setScriptRunning(doc, false);
    });
    connect(editor, &ScriptEditorWindow::consoleMessage, this,
            [this](const QString& source, const QString& text, ConsoleOutput::MessageType type) {
                _consoleDockWidget->setVisible(true);
                _globalConsole->addMessage(text, type, source);
            });

    // Route Find/Replace to the script editor when it's active
    connect(wnd, &QMdiSubWindow::windowStateChanged, this,
            [this, editor](Qt::WindowStates, Qt::WindowStates newState) {
        if (newState & Qt::WindowActive) {
            connect(this, &MainWindow::find,    editor->scriptControl(), &JScriptControl::showFind,    Qt::UniqueConnection);
            connect(this, &MainWindow::replace, editor->scriptControl(), &JScriptControl::showReplace, Qt::UniqueConnection);
        } else {
            disconnect(this, &MainWindow::find,    editor->scriptControl(), &JScriptControl::showFind);
            disconnect(this, &MainWindow::replace, editor->scriptControl(), &JScriptControl::showReplace);
        }
    });

    wnd->show();
    _projectTree->activateScript(doc);
    return editor;
}

///
/// \brief MainWindow::findScriptEditor
///
ScriptEditorWindow* MainWindow::findScriptEditor(ScriptDocument* doc) const
{
    for (auto wnd : ui->mdiArea->subWindowList()) {
        auto editor = qobject_cast<ScriptEditorWindow*>(wnd->widget());
        if (editor && editor->document() == doc)
            return editor;
    }
    return nullptr;
}

///
/// \brief MainWindow::rewrapMdiChild
/// Re-opens a previously "closed" (hidden) FormModSim by creating a new MDI subwindow for it.
///
void MainWindow::rewrapMdiChild(FormModSim* frm)
{
    if (!frm || !_closedForms.contains(frm))
        return;

    auto area = ui->mdiArea->primaryArea();
    if (!area)
        return;

    _closedForms.removeOne(frm);

    frm->setParent(nullptr); // detach from MainWindow before adding to MDI area
    auto wnd = area->addSubWindow(frm);
    if (!wnd) {
        frm->setParent(this);
        frm->hide();
        _closedForms.append(frm);
        return;
    }

    wnd->installEventFilter(this);
    wnd->setAttribute(Qt::WA_DeleteOnClose, true);
    wnd->setWindowTitle(frm->windowTitle());
    wnd->setWindowIcon(frm->windowIcon());

    // Window-specific connections (new wnd each time)
    auto updateCodepage = [this](const QString& name) { _ansiMenu->selectCodepage(name); };
    connect(wnd, &QMdiSubWindow::windowStateChanged, this,
            [this, frm, updateCodepage](Qt::WindowStates, Qt::WindowStates newState)
    {
        switch(newState & ~Qt::WindowMaximized & ~Qt::WindowMinimized)
        {
            case Qt::WindowActive:
                updateHelpWidgetState();
                updateCodepage(frm->codepage());
                frm->connectEditSlots();
            break;
            case Qt::WindowNoState:
                frm->disconnectEditSlots();
            break;
        }
    });

    connect(wnd, &QObject::destroyed, this, [this]() {
        resetSplitViewIfEmpty();
    });

    _windowActionList->addWindow(wnd);
    _projectTree->setFormOpen(frm, true);
    _projectTree->activateForm(frm);

    frm->show();
}

///
/// \brief MainWindow::closeProject
/// Closes all open and hidden forms and scripts, resetting the workspace.
///
void MainWindow::closeProject()
{
    // Close open MDI windows (FormModSim and ScriptEditorWindow)
    ui->mdiArea->closeAllSubWindows();

    // Delete forms that were closed (hidden)
    for (auto frm : _closedForms) {
        _projectTree->removeForm(frm);
        delete frm;
    }
    _closedForms.clear();

    // Delete standalone scripts
    for (auto doc : _standaloneScripts) {
        _projectTree->removeScript(doc);
        delete doc;
    }
    _standaloneScripts.clear();

    _windowCounter = 0;
    _scriptCounter = 0;
}

///
/// \brief MainWindow::deleteForm
/// Deletes a form permanently from the project (closes its tab if open).
///
void MainWindow::deleteForm(FormModSim* frm)
{
    if (!frm) return;

    // Close the MDI subwindow if the form is currently open
    for (auto wnd : ui->mdiArea->subWindowList()) {
        if (qobject_cast<FormModSim*>(wnd->widget()) == frm) {
            wnd->close(); // triggers closing signal → moves frm to _closedForms
            break;
        }
    }

    // Now frm is either in _closedForms (just closed) or was already there
    _closedForms.removeOne(frm);
    _projectTree->removeForm(frm);
    delete frm;
}

///
/// \brief MainWindow::deleteScript
/// Deletes a standalone script permanently from the project.
///
void MainWindow::deleteScript(ScriptDocument* doc)
{
    if (!doc) return;

    // Close the editor window if open
    if (auto editor = findScriptEditor(doc)) {
        for (auto wnd : ui->mdiArea->subWindowList()) {
            if (wnd->widget() == editor) {
                wnd->close();
                break;
            }
        }
    }

    _standaloneScripts.removeOne(doc);
    _projectTree->removeScript(doc);
    delete doc;
}

///
/// \brief checkPathIsWritable
/// \param path
/// \return
///
static bool checkPathIsWritable(const QString& path)
{
    const auto filepath = QString("%1%2%3").arg(path, QDir::separator(), ".test");
    if(!QFile(filepath).open(QFile::WriteOnly)) return false;

    QFile::remove(filepath);
    return true;
}

///
/// \brief canWriteFile
/// \param filePath
/// \return
///
static bool canWriteFile(const QString& filePath)
{
    QFile file(filePath);

    if (file.exists()) {
        return file.open(QIODevice::WriteOnly | QIODevice::Append);
    }

    const QString dirPath = QFileInfo(filePath).absolutePath();
    return checkPathIsWritable(dirPath);
}

///
/// \brief MainWindow::getSettingsFilePath
/// \return
///
static QString getSettingsFilePath()
{
    const QString filename = QString("%1.ini").arg(QFileInfo(qApp->applicationFilePath()).baseName());
    const QString appFilePath = QDir(qApp->applicationDirPath()).filePath(filename);

    if (canWriteFile(appFilePath))
        return appFilePath;

    return QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)).filePath(filename);
}


///
/// \brief MainWindow::loadProfile
/// \param filename
/// \return
///
bool MainWindow::loadProfile(const QString& filename)
{
    _profile = filename.isEmpty() ? getSettingsFilePath() : filename;
    if(!QFile::exists(_profile)) return false;

    QSettings m(_profile, QSettings::IniFormat, this);

    AppPreferences::instance().load(m);

    restoreGeometry(m.value("WindowGeometry").toByteArray());

    const auto viewMode = (QMdiArea::ViewMode)qBound(0, m.value("ViewMode", QMdiArea::TabbedView).toInt(), 1);
    setViewMode(viewMode);

    statusBar()->setVisible(m.value("StatusBar", true).toBool());

    _lang = m.value("Language", translationLang()).toString();
    setLanguage(_lang);

    _savePath = m.value("SavePath").toString();

    ModbusDefinitions defs;
    m >> defs;
    _mbMultiServer.setModbusDefinitions(defs);

    m >> qobject_cast<MenuConnect*>(ui->actionConnect->menu());

    const QStringList groups = m.childGroups();
    for (const QString& g : groups) {
        if (g.startsWith("Form_")) {
            m.beginGroup(g);
            const auto id = m.value("FromId", ++_windowCounter).toInt();
            auto frm = createMdiChild(id);
            m >> frm;
            syncSplitPeerState(frm);
            frm->show();
            m.endGroup();
        }
    }

    if(m.value("SplitView", false).toBool())
    {
        ui->mdiArea->setSplitViewEnabled(true);

        for(const QString& g : groups)
        {
            if(!g.startsWith("Form_")) continue;
            m.beginGroup(g);
            const int formId = m.value("FromId", 0).toInt();
            if(auto frm = findMdiChild(formId))
                if(auto mirror = splitPeer(frm))
                {
                    if(m.contains("MirrorDisplayMode"))
                        mirror->setDisplayMode((DisplayMode)m.value("MirrorDisplayMode", 0).toInt());
                    if(m.contains("MirrorScriptCursorPos"))
                        mirror->setScriptCursorPosition(m.value("MirrorScriptCursorPos").toInt());
                    if(m.contains("MirrorScriptScrollPos"))
                        mirror->setScriptScrollPosition(m.value("MirrorScriptScrollPos").toInt());
                }
            m.endGroup();
        }

        const auto activeSecWin = m.value("ActiveSecondaryWindow").toString();
        if(!activeSecWin.isEmpty())
            if(auto secondary = splitSecondaryArea())
                for(auto&& wnd : secondary->localSubWindowList())
                    if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
                        if(frm->windowTitle() == activeSecWin)
                        {
                            secondary->setActiveSubWindow(wnd);
                            break;
                        }
    }

    // activate window
    const auto activePrimaryTitle = m.value("ActivePrimaryWindow", m.value("ActiveWindow")).toString();
    if(!activePrimaryTitle.isEmpty()) {
        const auto primaryList = ui->mdiArea->primaryArea()
            ? ui->mdiArea->primaryArea()->localSubWindowList()
            : ui->mdiArea->localSubWindowList();
        for(auto&& wnd : primaryList)
        {
            const auto frm = qobject_cast<FormModSim*>(wnd->widget());
            if(frm && frm->windowTitle() == activePrimaryTitle) {
                ui->mdiArea->setActiveSubWindow(wnd);
                break;
            }
        }
    }

    restoreState(m.value("WindowState").toByteArray());

    return true;
}

///
/// \brief MainWindow::saveProfile
///
void MainWindow::saveProfile()
{
    const QString filepath = _profile.isEmpty() ? getSettingsFilePath() : _profile;
    QSettings m(filepath, QSettings::IniFormat, this);

    m.clear();
    m.sync();

    AppPreferences::instance().save(m);

    m.setValue("WindowGeometry", saveGeometry());
    m.setValue("WindowState", saveState());

    const auto frm = currentMdiChild();
    if(frm) m.setValue("ActiveWindow", frm->windowTitle());

    if(auto primary = ui->mdiArea->primaryArea())
        if(auto wnd = primary->activeSubWindow())
            if(auto frmPrimary = qobject_cast<FormModSim*>(wnd->widget()))
                m.setValue("ActivePrimaryWindow", frmPrimary->windowTitle());

    m.setValue("ViewMode", ui->mdiArea->viewMode());
    m.setValue("SplitView", ui->mdiArea->isSplitView());
    m.setValue("StatusBar", statusBar()->isVisible());
    m.setValue("Language", _lang);
    m.setValue("SavePath", _savePath);

    m << _mbMultiServer.getModbusDefinitions();

    m << qobject_cast<MenuConnect*>(ui->actionConnect->menu());

    const auto subWindowList = ui->mdiArea->localSubWindowList();
    for(int i = 0; i < subWindowList.size(); ++i) {
        const auto frm = qobject_cast<FormModSim*>(subWindowList[i]->widget());
        if(frm) {
            m.beginGroup("Form_" + QString::number(i + 1));
            m.setValue("FromId", frm->formId());
            m << frm;
            if(isSplitTabbedView())
                if(auto mirror = splitPeer(frm))
                {
                    m.setValue("MirrorDisplayMode", (int)mirror->displayMode());
                    m.setValue("MirrorScriptCursorPos", mirror->scriptCursorPosition());
                    m.setValue("MirrorScriptScrollPos", mirror->scriptScrollPosition());
                }
            m.endGroup();
        }
    }

    if(isSplitTabbedView())
        if(auto secondary = splitSecondaryArea())
            if(auto wnd = secondary->activeSubWindow())
                if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
                    m.setValue("ActiveSecondaryWindow", frm->windowTitle());
}
