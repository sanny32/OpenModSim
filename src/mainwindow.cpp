#include <QtWidgets>
#include <QBuffer>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QPageSetupDialog>
#include "apppreferences.h"
#include "dialogabout.h"
#include "dialogmsgparser.h"
#include "dialogpreferences.h"
#include "dialogprintsettings.h"
#include "dialogselectserviceport.h"
#include "dialogsetupserialport.h"
#include "dialogsetuppresetdata.h"
#include "dialogforcemultiplecoils.h"
#include "dialogforcemultipleregisters.h"
#include "dialogmodbusdefinitions.h"
#include "mainstatusbar.h"
#include "menuconnect.h"
#include "mdiareaex.h"
#include "formscriptview.h"
#include "formdatamapview.h"
#include "applogger.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

// Forward declaration (defined later in this file)
static QString getSettingsFilePath();
namespace {
constexpr const char* kSplitAutoCloneProperty = "SplitAutoClone";
constexpr const char* kNewFormKindKey = "NewFormKind";
constexpr const char* kRecentProjectsKey = "RecentProjects";
constexpr const char* kLastProjectPathKey = "LastProjectPath";
constexpr int kMaxRecentProjects = 10;

ProjectFormKind newFormKindFromSetting(int value)
{
    switch (static_cast<ProjectFormKind>(value)) {
        case ProjectFormKind::Data:
        case ProjectFormKind::Traffic:
        case ProjectFormKind::Script:
        case ProjectFormKind::DataMap:
            return static_cast<ProjectFormKind>(value);
        default:
            return ProjectFormKind::Data;
    }
}

int newFormKindToSetting(ProjectFormKind kind)
{
    return static_cast<int>(kind);
}

template<typename MdiAreaT, typename Fn>
void forEachTypedForm(MdiAreaT* mdiArea, Fn&& fn)
{
    if(!mdiArea)
        return;

    for (auto* wnd : mdiArea->subWindowList()) {
        if(!wnd)
            continue;

        if (auto* frm = qobject_cast<FormDataView*>(wnd->widget())) {
            fn(frm);
            continue;
        }
        if (auto* frm = qobject_cast<FormTrafficView*>(wnd->widget())) {
            fn(frm);
            continue;
        }
        if (auto* frm = qobject_cast<FormScriptView*>(wnd->widget())) {
            fn(frm);
            continue;
        }
        if (auto* frm = qobject_cast<FormDataMapView*>(wnd->widget())) {
            fn(frm);
            continue;
        }
    }
}

template<typename TDefinitions>
void applySharedDisplayDefaults(TDefinitions& target, const TDefinitions& defaults)
{
    target.LeadingZeros = defaults.LeadingZeros;
    target.DataViewColumnsDistance = defaults.DataViewColumnsDistance;
}

void applySharedDisplayDefaults(TrafficViewDefinitions& target, const TrafficViewDefinitions& defaults)
{
    target.LogViewLimit = defaults.LogViewLimit;
    target.UnitFilter = defaults.UnitFilter;
    target.FunctionCodeFilter = defaults.FunctionCodeFilter;
    target.ExceptionsOnly = defaults.ExceptionsOnly;
    target.Autoscroll = defaults.Autoscroll;
}

void applySharedDisplayDefaults(ScriptViewDefinitions& target, const ScriptViewDefinitions& defaults)
{
    target.ScriptCfg = defaults.ScriptCfg;
}

DataType dataTypeOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->dataType();
    return DataType::Hex;
}

void printOnForm(QWidget* widget, QPrinter* printer)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->print(printer);
    else if (auto* frm = qobject_cast<FormTrafficView*>(widget)) frm->print(printer);
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->print(printer);
    else if (auto* frm = qobject_cast<FormDataMapView*>(widget)) frm->print(printer);
}

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

    ui->actionNew->setMenu(ui->menuNew);
    if (auto* newButton = qobject_cast<QToolButton*>(ui->toolBarMain->widgetForAction(ui->actionNew))) {
        newButton->setPopupMode(QToolButton::MenuButtonPopup);
    }
    _openRecentMenu = new QMenu(tr("Open Recent"), this);
    _clearRecentAction = _openRecentMenu->addAction(tr("Clear List"));
    connect(_clearRecentAction, &QAction::triggered, this, &MainWindow::clearRecentProjects);
    ui->menuFile->insertMenu(ui->actionSaveProject, _openRecentMenu);
    ui->menuFile->insertSeparator(ui->actionSaveProject);

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
    setupGlobalViewToolbar();

    const auto defaultPrinter = QPrinterInfo::defaultPrinter();
    if(!defaultPrinter.isNull())
        _selectedPrinter = QSharedPointer<QPrinter>(new QPrinter(defaultPrinter));

    _projectTree = new ProjectTreeWidget(this);
    _projectDockWidget = new QDockWidget(tr("Project"), this);
    _projectDockWidget->setObjectName("projectDockWidget");
    _projectDockWidget->setWidget(_projectTree);
    _projectDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, _projectDockWidget);

    _project = new AppProject(ui->mdiArea, _mbMultiServer, _dataSimulator,
                               _projectTree, this, this);

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

    connect(_projectTree, &ProjectTreeWidget::formActivated, this, [this](ProjectFormRef ref) {
        if(ref.widget)
            _project->openFormOnActivePanel(ref.widget);
    });
    connect(_projectTree, &ProjectTreeWidget::formDeleteRequested, this, [this](ProjectFormRef ref) {
        if(!ref.widget)
            return;
        _project->deleteForm(ref.widget);
    });
    connect(_projectTree, &ProjectTreeWidget::formRenamed, this, [this](ProjectFormRef) { markModified(); });
    connect(_projectTree, &ProjectTreeWidget::formRunScriptRequested, this, [this](ProjectFormRef ref) {
        if (auto* script = qobject_cast<FormScriptView*>(ref.widget))
            script->runScript();
    });
    connect(_projectTree, &ProjectTreeWidget::formStopScriptRequested, this, [this](ProjectFormRef ref) {
        if (auto* script = qobject_cast<FormScriptView*>(ref.widget))
            script->stopScript();
    });
    connect(_projectTree, &ProjectTreeWidget::runAllScriptsRequested, this, &MainWindow::runAllScripts);
    connect(_projectTree, &ProjectTreeWidget::stopAllScriptsRequested, this, &MainWindow::stopAllScripts);
    connect(_projectTree, &ProjectTreeWidget::deleteAllFormsRequested, this, &MainWindow::deleteAllForms);
    connect(_projectTree, &ProjectTreeWidget::formCreateRequested, this, [this](ProjectFormType type) {
        createNewForm(static_cast<ProjectFormKind>(type));
    });
    _outputPanel = new OutputPanel(this);
    _outputPanel->jsConsole()->setMaxLines(AppPreferences::instance().consoleMaxLines());
    _consoleDockWidget = new QDockWidget(tr("Output"), this);
    _consoleDockWidget->setObjectName("consoleDockWidget");
    _consoleDockWidget->setWidget(_outputPanel);
    _consoleDockWidget->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, _consoleDockWidget);
    _consoleDockWidget->setVisible(false);

    connect(_outputPanel, &OutputPanel::collapse, this, [this]() {
        _consoleDockWidget->setVisible(false);
    });

    ui->mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    connect(ui->mdiArea, &MdiAreaEx::subWindowActivated, this, &MainWindow::updateMenuWindow);
    connect(ui->mdiArea, &MdiAreaEx::subWindowActivated, this, [this](QMdiSubWindow* wnd) {
        if(wnd)
            markModified();

        QMdiSubWindow* stableWnd = ui->mdiArea->activeSubWindow();
        if(!stableWnd)
            stableWnd = ui->mdiArea->currentSubWindow();
        if(stableWnd)
            _projectTree->activateForm(stableWnd->widget());
        syncGlobalViewControls();
    });
    connect(ui->mdiArea, &MdiAreaEx::tabsReordered, this, &MainWindow::markModified);
    connect(ui->mdiArea, &MdiAreaEx::tabContextMenuRequested, this, &MainWindow::on_tabContextMenuRequested);
    connect(ui->mdiArea, &MdiAreaEx::moveTabToOtherPanelRequested, this, &MainWindow::on_moveTabToOtherPanelRequested);
    connect(ui->mdiArea, &MdiAreaEx::splitViewAboutToDisable, this, [this]() {
        _project->removeSplitAutoClonesFromSecondary();
    });
    connect(ui->mdiArea, &MdiAreaEx::splitViewToggled, this, [this](bool enabled) {
        if(enabled)
            _project->duplicatePrimaryTabsToSecondary();
    });
    connect(&_mbMultiServer, &ModbusMultiServer::connectionError, this, &MainWindow::on_connectionError);
    AppLogger::setupModbusMultiServerLogging(_mbMultiServer, this);
    AppLogger::setupDataSimulatorLogging(*_dataSimulator, this);
    AppLogger::setupAppProjectLogging(*_project, this);

    // View / window trivial action wiring
    connect(ui->actionExit,           &QAction::triggered, this, [this]{ close(); });
    connect(ui->actionCascade,        &QAction::triggered, this, [this]{ ui->mdiArea->cascadeSubWindows(); });
    connect(ui->actionTile,           &QAction::triggered, this, [this]{ ui->mdiArea->tileSubWindows(); });
    connect(ui->actionSplitView,      &QAction::triggered, this, [this]{ ui->mdiArea->toggleVerticalSplit(); });
    connect(ui->actionToolbar,        &QAction::triggered, this, [this]{ ui->toolBarMain->setVisible(!ui->toolBarMain->isVisible()); });
    connect(ui->actionStatusBar,      &QAction::triggered, this, [this]{ statusBar()->setVisible(!statusBar()->isVisible()); });
    connect(ui->actionProjectTree,    &QAction::triggered, this, [this]{ _projectDockWidget->setVisible(!_projectDockWidget->isVisible()); });
    connect(ui->actionScriptHelp,     &QAction::triggered, this, [this]{ _helpDockWidget->setVisible(!_helpDockWidget->isVisible()); });
    connect(ui->actionOutputWindow,   &QAction::triggered, this, [this]{ _consoleDockWidget->setVisible(!_consoleDockWidget->isVisible()); });

    // Edit command wiring
    connect(ui->actionUndo,      &QAction::triggered, this, [this]{ if (auto* w = QApplication::focusWidget()) QMetaObject::invokeMethod(w, "undo"); });
    connect(ui->actionRedo,      &QAction::triggered, this, [this]{ if (auto* w = QApplication::focusWidget()) QMetaObject::invokeMethod(w, "redo"); });
    connect(ui->actionCut,       &QAction::triggered, this, [this]{ if (auto* w = QApplication::focusWidget()) QMetaObject::invokeMethod(w, "cut"); });
    connect(ui->actionCopy,      &QAction::triggered, this, [this]{ if (auto* w = QApplication::focusWidget()) QMetaObject::invokeMethod(w, "copy"); });
    connect(ui->actionPaste,     &QAction::triggered, this, [this]{ if (auto* w = QApplication::focusWidget()) QMetaObject::invokeMethod(w, "paste"); });
    connect(ui->actionSelectAll, &QAction::triggered, this, [this]{ emit selectAll(); });
    connect(ui->actionFind,      &QAction::triggered, this, [this]{ emit find(); });
    connect(ui->actionReplace,   &QAction::triggered, this, [this]{ emit replace(); });

    // New form kind wiring
    connect(ui->actionNewDataView,        &QAction::triggered, this, [this]{ activateNewFormKind(ProjectFormKind::Data,        ui->actionNewDataView); });
    connect(ui->actionNewTrafficView,     &QAction::triggered, this, [this]{ activateNewFormKind(ProjectFormKind::Traffic,     ui->actionNewTrafficView); });
    connect(ui->actionNewDataMapView,     &QAction::triggered, this, [this]{ activateNewFormKind(ProjectFormKind::DataMap,     ui->actionNewDataMapView); });
    connect(ui->actionNewScript,          &QAction::triggered, this, [this]{ activateNewFormKind(ProjectFormKind::Script,      ui->actionNewScript); });

    loadAppSettings(profile);
    rebuildRecentProjectsMenu();

    const bool canRestoreLastProject = !_lastProjectPath.isEmpty() && QFile::exists(_lastProjectPath);
    if(canRestoreLastProject) {
        loadProject(_lastProjectPath);
        addRecentProject(_lastProjectPath);
    } else {
        ui->actionNew->trigger();
    }
}

///
/// \brief MainWindow::~MainWindow
///
MainWindow::~MainWindow()
{
    // Important: destroy forms while _mbMultiServer is still alive.
    // FormDataView::~FormDataView() calls removeDeviceId/removeUnitMap.
    if(_project)
        _project->destroyContentForShutdown();

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
/// \brief MainWindow::showEvent
/// \param event
///
void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    _project->restoreActiveWindows();
}

///
/// \brief MainWindow::runAllScripts
///
void MainWindow::runAllScripts()
{
    for (auto* script : _project->scriptForms()) {
        if (script && script->canRunScript())
            script->runScript();
    }
}

///
/// \brief MainWindow::stopAllScripts
///
void MainWindow::stopAllScripts()
{
    for (auto* script : _project->scriptForms()) {
        if (script && script->canStopScript())
            script->stopScript();
    }
}

///
/// \brief MainWindow::deleteAllForms
/// \param type
///
void MainWindow::deleteAllForms(ProjectFormType type)
{
    const auto forms = _project->forms(static_cast<ProjectFormKind>(type));
    for (auto* form : forms) {
        if (form && !form->property("DeleteLocked").toBool())
            _project->deleteForm(form);
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

        _projectDockWidget->setWindowTitle(tr("Project"));
        _helpDockWidget->setWindowTitle(tr("Script Help"));
        _consoleDockWidget->setWindowTitle(tr("Output"));
        _openRecentMenu->setTitle(tr("Open Recent"));

        rebuildRecentProjectsMenu();
        updateProjectWindowTitle();
    }

    QMainWindow::changeEvent(event);
}

///
/// \brief MainWindow::closeEvent
/// \param event
///
void MainWindow::closeEvent(QCloseEvent *event)
{
    const bool shouldAskToSave = hasProjectContext() && (_isModified || _projectFilePath.isEmpty());
    if(shouldAskToSave) {
        if(!confirmSaveOnClose()) {
            event->ignore();
            return;
        }
    }

    saveAppSettings();

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
                auto* frm = wnd->widget();
                if (frm && !frm->property(kSplitAutoCloneProperty).toBool()) {
                    // Primary form: reparent before subwindow is destroyed so frm survives
                    _project->markFormClosed(frm);
                    markModified();
                }
            }
        break;
        case QEvent::Move:
            if(auto wnd = qobject_cast<const QMdiSubWindow*>(obj))
            {
                auto* widget = wnd->widget();
                if(!widget || wnd->isMinimized() || wnd->isMaximized())
                    break;

                if (auto* frm = qobject_cast<FormTrafficView*>(widget))
                    frm->setProperty("ParentGeometry", wnd->geometry());
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
    auto* frm = currentForm();
    const bool hasProject = hasProjectContext();

    const bool tabbedView    = ui->mdiArea->viewMode() == QMdiArea::TabbedView;
    const bool subWindowView = ui->mdiArea->viewMode() == QMdiArea::SubWindowView;

    bool isData = false, isScript = false, canPrint = false;
    if (frm) {
        if (auto* f = qobject_cast<FormDataView*>(frm)) {
            isData   = true;
            canPrint = _selectedPrinter != nullptr;
        } else if (auto* f = qobject_cast<FormTrafficView*>(frm)) {
            canPrint = _selectedPrinter != nullptr && !f->isLogEmpty();
        } else if (auto* f = qobject_cast<FormScriptView*>(frm)) {
            isScript = true;
            canPrint = _selectedPrinter != nullptr && !f->script().isEmpty();
        } else if (auto* f = qobject_cast<FormDataMapView*>(frm)) {
            canPrint = _selectedPrinter != nullptr && !f->isEmpty();
        }
    }

    ui->menuSetup->setEnabled(hasProject);
    ui->menuWindow->setEnabled(frm != nullptr);
    ui->actionPrintSetup->setEnabled(hasProject && _selectedPrinter != nullptr && frm != nullptr);
    ui->actionPrint->setEnabled(canPrint);

    ui->actionUndo->setEnabled(isScript);
    ui->actionRedo->setEnabled(isScript);
    ui->actionCut->setEnabled(isScript);
    ui->actionCopy->setEnabled(isScript);
    ui->actionPaste->setEnabled(isScript);
    ui->actionSelectAll->setEnabled(isScript);
    ui->actionFind->setEnabled(isScript || isData);
    ui->actionReplace->setEnabled(isScript);

    ui->actionTabbedView->setChecked(tabbedView);
    ui->actionSplitView->setVisible(tabbedView);
    ui->actionSplitView->setEnabled(hasProject);
    ui->actionSplitView->setChecked(ui->mdiArea->isSplitView());
    ui->actionToolbar->setChecked(ui->toolBarMain->isVisible());
    ui->actionStatusBar->setChecked(statusBar()->isVisible());
    ui->actionScriptHelp->setChecked(_helpDockWidget->isVisible());
    ui->actionScriptHelp->setVisible(isScript);
    ui->actionOutputWindow->setChecked(_consoleDockWidget->isVisible());
    ui->actionProjectTree->setChecked(_projectDockWidget->isVisible());

    ui->actionTile->setEnabled(subWindowView);
    ui->actionCascade->setEnabled(subWindowView);
    updateMainToolbarState();
}

///
/// \brief MainWindow::on_actionNew_triggered
///
void MainWindow::on_actionNew_triggered()
{
    createNewForm(_newFormKind);
}

///
/// \brief MainWindow::createNewForm
/// \param kind
///
QWidget* MainWindow::createNewForm(ProjectFormKind kind)
{
    auto* frm = _project->createMdiChild(kind);
    if(!frm)
        return nullptr;

    markModified();

    const auto& prefs = AppPreferences::instance();

    switch(kind)
    {
        case ProjectFormKind::Data: {
            if (auto* dataFrm = qobject_cast<FormDataView*>(frm)) {
                dataFrm->setFont(prefs.font());
                dataFrm->setBackgroundColor(prefs.backgroundColor());
                dataFrm->setForegroundColor(prefs.foregroundColor());
                dataFrm->setAddressColor(prefs.addressColor());
                dataFrm->setCommentColor(prefs.commentColor());
                dataFrm->setZoomPercent(prefs.fontZoom());

                auto dd = dataFrm->displayDefinition();
                applySharedDisplayDefaults(dd, prefs.dataViewDefinitions());
                dataFrm->setDisplayDefinition(dd);
            }
            break;
        }
        case ProjectFormKind::Traffic: {
            if (auto* trafficFrm = qobject_cast<FormTrafficView*>(frm)) {
                trafficFrm->setFont(prefs.font());
                trafficFrm->setBackgroundColor(prefs.backgroundColor());
                trafficFrm->setForegroundColor(prefs.foregroundColor());

                auto dd = trafficFrm->displayDefinition();
                applySharedDisplayDefaults(dd, prefs.trafficViewDefinitions());
                trafficFrm->setDisplayDefinition(dd);
            }
            break;
        }
        case ProjectFormKind::Script: {
            if (auto* scriptFrm = qobject_cast<FormScriptView*>(frm)) {
                scriptFrm->setBackgroundColor(prefs.backgroundColor());
                scriptFrm->setForegroundColor(prefs.foregroundColor());
                scriptFrm->setFont(prefs.scriptFont());
                scriptFrm->setZoomPercent(prefs.fontZoom());

                auto dd = scriptFrm->definitions();
                applySharedDisplayDefaults(dd, prefs.scriptViewDefinitions());
                scriptFrm->setDefinitions(dd);
            }
            break;
        }
        case ProjectFormKind::DataMap:
            break;
    }

    applyGlobalViewStateToForm(frm);
    frm->show();
    updateMainToolbarState();
    return frm;
}

///
/// \brief MainWindow::activateNewFormKind
/// \param kind
/// \param sourceAction
///
void MainWindow::activateNewFormKind(ProjectFormKind kind, QAction* sourceAction)
{
    _newFormKind = kind;
    ui->actionNew->setIcon(sourceAction->icon());
    createNewForm(_newFormKind);
}

///
/// \brief MainWindow::restoreNewFormKindIcon
///
void MainWindow::restoreNewFormKindIcon()
{
    switch(_newFormKind) {
        case ProjectFormKind::Traffic:     ui->actionNew->setIcon(ui->actionNewTrafficView->icon());     break;
        case ProjectFormKind::Script:      ui->actionNew->setIcon(ui->actionNewScript->icon());          break;
        case ProjectFormKind::DataMap: ui->actionNew->setIcon(ui->actionNewDataMapView->icon()); break;
        default:                           ui->actionNew->setIcon(ui->actionNewDataView->icon());        break;
    }
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
    filters << tr("Project files (*.omp)");
    filters << tr("All files (*)");

    const auto filename = QFileDialog::getOpenFileName(this, QString(), _project->savePath(), filters.join(";;"));
    if(filename.isEmpty()) return;

    _project->setSavePath(QFileInfo(filename).absoluteDir().absolutePath());
    loadProject(filename);
    addRecentProject(QFileInfo(filename).absoluteFilePath());
}

///
/// \brief MainWindow::on_actionSaveProject_triggered
///
void MainWindow::on_actionSaveProject_triggered()
{
    if(_projectFilePath.isEmpty()) {
        on_actionSaveProjectAs_triggered();
        return;
    }

    saveProject(_projectFilePath);
    addRecentProject(_projectFilePath);
}

///
/// \brief MainWindow::on_actionSaveProjectAs_triggered
///
void MainWindow::on_actionSaveProjectAs_triggered()
{
    QStringList filters;
    filters << tr("Project files (*.omp)");
    auto filename = QFileDialog::getSaveFileName(this, QString(), _project->savePath(), filters.join(";;"));

    if(filename.isEmpty()) return;

    if(!filename.endsWith(".omp", Qt::CaseInsensitive))
        filename.append(".omp");

    _project->setSavePath(QFileInfo(filename).absoluteDir().absolutePath());
    saveProject(filename);
    addRecentProject(QFileInfo(filename).absoluteFilePath());
}

///
/// \brief MainWindow::on_actionCloseProject_triggered
///
void MainWindow::on_actionCloseProject_triggered()
{
    const bool shouldAskToSave = hasProjectContext() && (_isModified || _projectFilePath.isEmpty());
    if(shouldAskToSave) {
        if(!confirmSaveOnClose())
            return;
    }

    _project->closeProject();
    _projectFilePath.clear();
    _isModified = false;
    updateProjectWindowTitle();
    updateMainToolbarState();
}

///
/// \brief MainWindow::on_actionPrint_triggered
///
void MainWindow::on_actionPrint_triggered()
{
    auto* frm = currentForm();
    if(!frm) return;

    QPrintDialog dlg(_selectedPrinter.get(), this);
    if(dlg.exec() == QDialog::Accepted)
    {
        printOnForm(frm, _selectedPrinter.get());
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
    forEachTypedForm(ui->mdiArea, [enable](auto* frm) {
        if (auto* script = qobject_cast<FormScriptView*>(frm))
            script->enableAutoComplete(enable);
    });
}

///
/// \brief MainWindow::applyConsoleMaxLines
/// \param n
///
void MainWindow::applyConsoleMaxLines(int n)
{
    _outputPanel->jsConsole()->setMaxLines(n);
}

///
/// \brief MainWindow::applyScriptFont
/// \param font
///
void MainWindow::applyFont(const QFont& font)
{
    forEachTypedForm(ui->mdiArea, [&font](auto* frm) {
        if (!qobject_cast<FormDataMapView*>(frm))
            frm->setFont(font);
    });
}

///
/// \brief MainWindow::applyScriptFont
/// \param font
///
void MainWindow::applyScriptFont(const QFont& font)
{
    forEachTypedForm(ui->mdiArea, [&font](auto* frm) {
        if (auto* script = qobject_cast<FormScriptView*>(frm))
            script->setFont(font);
    });
}

///
/// \brief MainWindow::applyColors
/// \param bg
/// \param fg
/// \param status
///
void MainWindow::applyColors(const QColor& bg, const QColor& fg, const QColor& addr, const QColor& comment)
{
    forEachTypedForm(ui->mdiArea, [&bg, &fg, &addr, &comment](auto* frm) {
        if (auto* data = qobject_cast<FormDataView*>(frm)) {
            data->setBackgroundColor(bg);
            data->setForegroundColor(fg);
            data->setAddressColor(addr);
            data->setCommentColor(comment);
        } else if (auto* traffic = qobject_cast<FormTrafficView*>(frm)) {
            traffic->setBackgroundColor(bg);
            traffic->setForegroundColor(fg);
        } else if (auto* script = qobject_cast<FormScriptView*>(frm)) {
            script->setBackgroundColor(bg);
            script->setForegroundColor(fg);
        }
    });
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
    forEachTypedForm(ui->mdiArea, [zoomPercent](auto* frm) {
        if (auto* data = qobject_cast<FormDataView*>(frm))
            data->setZoomPercent(zoomPercent);
        else if (auto* script = qobject_cast<FormScriptView*>(frm))
            script->setZoomPercent(zoomPercent);
    });
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
    auto* frm = currentDataOrTrafficForm();
    const auto type = frm ? dataTypeOfForm(frm) : DataType::Hex;

    auto dlg = new DialogMsgParser(type, ModbusMessage::Rtu);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    dlg->show();
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
/// \brief MainWindow::setViewMode
/// \param mode
///
void MainWindow::setViewMode(QMdiArea::ViewMode mode)
{
    ui->mdiArea->setViewMode(mode);
}

///
/// \brief MainWindow::updateHelpWidgetState
///
void MainWindow::updateHelpWidgetState()
{
    auto frm = _project->currentMdiChild();
    if(!frm) return;
    if (qobject_cast<FormScriptView*>(frm)) {
        if(!_helpDockWidget->isVisible() &&
            _helpDockWidget->property("WasShown").toBool())
        {
            _helpDockWidget->setVisible(true);
        }
        return;
    }

    if(_helpDockWidget->isVisible() &&
        !_helpDockWidget->isFloating())
    {
        _helpDockWidget->setProperty("WasShown", true);
        _helpDockWidget->setVisible(false);
    }
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
/// \brief MainWindow::on_actionImportScript_triggered
///
void MainWindow::on_actionImportScript_triggered()
{
    const auto filename = QFileDialog::getOpenFileName(this, QString(), QString(), tr("JavaScript files (*.js);;All files (*)"));
    if(filename.isEmpty()) return;

    QFile file(filename);
    if(!file.open(QFile::ReadOnly | QFile::Text)) return;

    const auto script = QTextStream(&file).readAll();

    if(auto* frm = qobject_cast<FormScriptView*>(createNewForm(ProjectFormKind::Script))) {
        frm->setScript(script);
        frm->setFormName(QFileInfo(filename).completeBaseName());
        _projectTree->updateFormTitle(frm);
    }
}

///
/// \brief MainWindow::on_connectionError
/// \param error
///
void MainWindow::on_connectionError(const QString& error)
{
    AppLogger::logConnectionError(error);
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
}

///
/// \brief MainWindow::on_tabContextMenuRequested
/// \param subWnd
/// \param sourcePanel
/// \param globalPos
///
void MainWindow::on_tabContextMenuRequested(QMdiSubWindow* subWnd, MdiArea* /*sourcePanel*/, QPoint globalPos)
{
    auto* frm = subWnd ? subWnd->widget() : nullptr;
    if(!frm || !_project)
        return;

    QMenu menu(this);
    if(_project->canMoveFormToOtherPanel(frm))
    {
        auto* act = menu.addAction(tr("Move to Other Panel"));
        connect(act, &QAction::triggered, this, [this, frm]() {
            _project->moveFormToOtherPanel(frm);
            markModified();
        });
    }
    if(!menu.isEmpty())
        menu.exec(globalPos);
}

///
/// \brief MainWindow::on_moveTabToOtherPanelRequested
/// \param subWnd
///
void MainWindow::on_moveTabToOtherPanelRequested(QMdiSubWindow* subWnd, QPoint globalDropPos)
{
    auto* frm = subWnd ? subWnd->widget() : nullptr;
    if(!frm || !_project)
        return;

    _project->moveFormToOtherPanel(frm, globalDropPos);
    markModified();
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
/// \brief MainWindow::markModified
///
void MainWindow::markModified()
{
    _isModified = true;
    updateProjectWindowTitle();
}

///
/// \brief MainWindow::currentForm
///
QWidget* MainWindow::currentForm() const
{
    return _project->currentMdiChild();
}

///
/// \brief MainWindow::currentDataForm
///
FormDataView* MainWindow::currentDataForm() const
{
    return _project->currentDataMdiChild();
}

///
/// \brief MainWindow::currentTrafficForm
///
FormTrafficView* MainWindow::currentTrafficForm() const
{
    return _project->currentTrafficMdiChild();
}

///
/// \brief MainWindow::currentScriptForm
///
FormScriptView* MainWindow::currentScriptForm() const
{
    return _project->currentScriptMdiChild();
}

FormDataMapView* MainWindow::currentDataMapForm() const
{
    return _project->currentDataMapMdiChild();
}

///
/// \brief MainWindow::currentDataOrTrafficForm
///
QWidget* MainWindow::currentDataOrTrafficForm() const
{
    if (auto* data = currentDataForm())
        return data;
    return currentTrafficForm();
}

///
/// \brief MainWindow::prepareWriteParams
/// \param type
/// \param outFrm
/// \param outDd
/// \param outPreset
/// \param outParams
/// \return
///
bool MainWindow::prepareWriteParams(QModbusDataUnit::RegisterType type,
                                    FormDataView*& outFrm,
                                    DataViewDefinitions& outDd,
                                    SetupPresetParams& outPreset,
                                    ModbusWriteParams& outParams)
{
    outFrm = currentDataForm();
    outDd = outFrm ? outFrm->displayDefinition() : AppPreferences::instance().dataViewDefinitions();
    const auto& prefs = AppPreferences::instance();
    const bool zeroBasedAddress = outFrm ? outFrm->zeroBasedAddress() : prefs.globalZeroBasedAddress();
    const auto addrSpace = _mbMultiServer.getModbusDefinitions().AddrSpace;

    outPreset = SetupPresetParams{
        outDd.DeviceId,
        outDd.PointAddress,
        outDd.Length,
        zeroBasedAddress,
        addrSpace,
        outDd.LeadingZeros
    };
    if(outFrm) {
        _presetParams[type] = outPreset;
    } else if(_presetParams.contains(type)) {
        outPreset = _presetParams.value(type);
        if (outPreset.ZeroBasedAddress != zeroBasedAddress) {
            int adjustedAddress = static_cast<int>(outPreset.PointAddress);
            if (zeroBasedAddress) {
                adjustedAddress = qMax(0, adjustedAddress - 1);
            } else if (adjustedAddress < std::numeric_limits<quint16>::max()) {
                adjustedAddress += 1;
            }
            outPreset.PointAddress = static_cast<quint16>(adjustedAddress);
            outPreset.ZeroBasedAddress = zeroBasedAddress;
        }
        outPreset.AddrSpace = addrSpace;
    }

    {
        DialogSetupPresetData dlg(outPreset, type, this);
        if(dlg.exec() != QDialog::Accepted) return false;
    }
    _presetParams[type] = outPreset;

    outParams.DeviceId         = outPreset.DeviceId;
    outParams.Address          = outPreset.PointAddress;
    outParams.ZeroBasedAddress = outPreset.ZeroBasedAddress;
    outParams.AddrSpace        = outPreset.AddrSpace;

    if(outDd.PointType == type)
    {
        const auto data = _mbMultiServer.data(outPreset.DeviceId, type,
            outPreset.PointAddress - (outPreset.ZeroBasedAddress ? 0 : 1),
            outPreset.Length);
        outParams.Value = QVariant::fromValue(data.values());
    }

    return true;
}

///
/// \brief MainWindow::forceCoils
/// \param type
///
void MainWindow::forceCoils(QModbusDataUnit::RegisterType type)
{
    FormDataView* frm; DataViewDefinitions dd; SetupPresetParams preset; ModbusWriteParams params;
    if(!prepareWriteParams(type, frm, dd, preset, params)) return;

    const bool displayHexAddresses = frm ? frm->displayHexAddresses() : AppPreferences::instance().globalHexView();
    DialogForceMultipleCoils dlg(params, type, preset.Length, displayHexAddresses, this);
    if(dlg.exec() == QDialog::Accepted) {
        _mbMultiServer.writeRegister(type, params);
    }
}

///
/// \brief MainWindow::presetRegs
/// \param type
///
void MainWindow::presetRegs(QModbusDataUnit::RegisterType type)
{
    FormDataView* frm; DataViewDefinitions dd; SetupPresetParams preset; ModbusWriteParams params;
    if(!prepareWriteParams(type, frm, dd, preset, params)) return;

    params.DataMode = frm ? frm->dataType()      : DataType::Hex;
    params.RegOrder = frm ? frm->registerOrder() : RegisterOrder::MSRF;
    params.Order    = frm ? frm->byteOrder()     : ByteOrder::Direct;
    params.Codepage = frm ? frm->codepage()      : QString();

    const bool displayHexAddresses = frm ? frm->displayHexAddresses() : AppPreferences::instance().globalHexView();
    DialogForceMultipleRegisters dlg(params, type, preset.Length, displayHexAddresses, this);
    if(dlg.exec() == QDialog::Accepted) {
        _mbMultiServer.writeRegister(type, params);
    }
}

///
/// \brief MainWindow::loadProject
/// \param filename
///
void MainWindow::loadProject(const QString& filename)
{
    if (hasProjectContext()) {
        _project->closeProject();
        _projectFilePath.clear();
        _isModified = false;
        updateProjectWindowTitle();
    }

    AppLogger::clear();

    _project->loadProject(filename);
    applyGlobalAddressBase(AppPreferences::instance().globalZeroBasedAddress(), false);
    applyGlobalHexView(AppPreferences::instance().globalHexView(), false);
    syncGlobalViewControls();
    _projectFilePath = QFileInfo(filename).absoluteFilePath();
    _lastProjectPath = _projectFilePath;
    _project->setSavePath(QFileInfo(filename).absoluteDir().absolutePath());
    _isModified = false;
    updateProjectWindowTitle();
    updateMainToolbarState();
}

///
/// \brief MainWindow::saveProject
/// \param filename
///
void MainWindow::saveProject(const QString& filename)
{
    _project->saveProject(filename);
    _projectFilePath = QFileInfo(filename).absoluteFilePath();
    _lastProjectPath = _projectFilePath;
    _project->setSavePath(QFileInfo(filename).absoluteDir().absolutePath());
    _isModified = false;
    updateProjectWindowTitle();
}

///
/// \brief MainWindow::updateProjectWindowTitle
///
void MainWindow::updateProjectWindowTitle()
{
    const QString modifiedMark = _isModified ? "*" : "";

    if(_projectFilePath.isEmpty()) {
        setWindowTitle(modifiedMark + APP_NAME);
        return;
    }

    const QString projectName = QFileInfo(_projectFilePath).completeBaseName();
    if(projectName.isEmpty()) {
        setWindowTitle(modifiedMark + APP_NAME);
        return;
    }

    setWindowTitle(QString("%1%2 - %3").arg(modifiedMark, APP_NAME, projectName));
}

///
/// \brief MainWindow::appendConsoleMessage
///
void MainWindow::appendConsoleMessage(const QString& source, const QString& text, ConsoleOutput::MessageType type)
{
    _outputPanel->jsConsole()->addMessage(text, type, source);
}

///
/// \brief MainWindow::showOutputConsole
///
void MainWindow::showOutputConsole()
{
    if (AppPreferences::instance().autoShowConsoleOutput()) {
        _consoleDockWidget->setVisible(true);
        _outputPanel->switchToJsConsole();
    }
}

///
/// \brief MainWindow::showHelpContext
///
void MainWindow::showHelpContext(const QString& helpKey)
{
    _helpDockWidget->setVisible(true);
    if (!helpKey.isEmpty())
        _helpWidget->showHelp(helpKey);
}

///
/// \brief MainWindow::applyConnections
///
void MainWindow::applyConnections(const ModbusDefinitions& defs, const QList<ConnectionDetails>& conns)
{
    _mbMultiServer.setModbusDefinitions(defs);
    auto menu = qobject_cast<MenuConnect*>(ui->actionConnect->menu());
    menu->updateConnectionDetails(conns);
    for (auto&& cd : conns) {
        if (menu->canConnect(cd))
            _mbMultiServer.connectDevice(cd);
    }
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
/// \brief MainWindow::loadAppSettings
/// \param filename
/// \return
///
bool MainWindow::loadAppSettings(const QString& filename)
{
    _profile = filename.isEmpty() ? getSettingsFilePath() : filename;
    if(!QFile::exists(_profile))
        return false;

    QSettings m(_profile, QSettings::IniFormat, this);

    AppPreferences::instance().load(m);
    _newFormKind = newFormKindFromSetting(
        m.value(kNewFormKindKey, newFormKindToSetting(ProjectFormKind::Data)).toInt());
    restoreNewFormKindIcon();

    restoreGeometry(m.value("WindowGeometry").toByteArray());
    restoreState(m.value("WindowState").toByteArray());

    statusBar()->setVisible(m.value("StatusBar", true).toBool());
    _lang = m.value("Language", translationLang()).toString();
    setLanguage(_lang);

    _project->setSavePath(m.value("SavePath").toString());
    _recentProjects = m.value(kRecentProjectsKey).toStringList();
    _recentProjects.removeAll(QString());
    _recentProjects.removeDuplicates();
    _lastProjectPath = m.value(kLastProjectPathKey).toString();
    syncGlobalViewControls();

    return true;
}

///
/// \brief MainWindow::saveAppSettings
///
void MainWindow::saveAppSettings()
{
    const QString filepath = _profile.isEmpty() ? getSettingsFilePath() : _profile;
    QSettings m(filepath, QSettings::IniFormat, this);

    m.clear();
    m.sync();

    AppPreferences::instance().save(m);
    m.setValue("WindowGeometry", saveGeometry());
    m.setValue("WindowState", saveState());
    m.setValue("StatusBar", statusBar()->isVisible());
    m.setValue("Language", _lang);
    m.setValue("SavePath", _project->savePath());
    m.setValue(kNewFormKindKey, newFormKindToSetting(_newFormKind));
    m.setValue(kRecentProjectsKey, _recentProjects);
    m.setValue(kLastProjectPathKey, _lastProjectPath);
}

///
/// \brief MainWindow::setupGlobalViewToolbar
///
void MainWindow::setupGlobalViewToolbar()
{
    _globalAddressBaseCombo = new QComboBox(ui->toolBarMain);
    _globalAddressBaseCombo->addItem(tr("1-based"), false);
    _globalAddressBaseCombo->addItem(tr("0-based"), true);
    _globalAddressBaseCombo->setMinimumWidth(84);

    auto* label = new QLabel(tr("Address Base"), ui->toolBarMain);
    _globalAddressBaseWidget = new QWidget(ui->toolBarMain);
    auto* layout = new QHBoxLayout(_globalAddressBaseWidget);
    layout->setContentsMargins(9, 0, 9, 0);
    layout->setSpacing(6);
    layout->addWidget(label);
    layout->addWidget(_globalAddressBaseCombo);

    QAction* insertBefore = ui->actionMbDefinitions;
    const auto actions = ui->toolBarMain->actions();
    const int defsIndex = actions.indexOf(ui->actionMbDefinitions);
    if (defsIndex > 0 && actions.at(defsIndex - 1)->isSeparator())
        insertBefore = actions.at(defsIndex - 1);

    ui->toolBarMain->insertSeparator(insertBefore);
    ui->toolBarMain->insertWidget(insertBefore, _globalAddressBaseWidget);
    ui->toolBarMain->insertAction(insertBefore, ui->actionHexView);

    connect(_globalAddressBaseCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!_globalAddressBaseCombo)
            return;
        applyGlobalAddressBase(_globalAddressBaseCombo->itemData(index).toBool());
    });
    connect(ui->actionHexView, &QAction::toggled, this, [this](bool checked) {
        applyGlobalHexView(checked);
    });
}

///
/// \brief MainWindow::syncGlobalViewControls
///
void MainWindow::syncGlobalViewControls()
{
    const auto& prefs = AppPreferences::instance();

    if (_globalAddressBaseCombo) {
        const QSignalBlocker blocker(_globalAddressBaseCombo);
        const int index = _globalAddressBaseCombo->findData(prefs.globalZeroBasedAddress());
        _globalAddressBaseCombo->setCurrentIndex(index >= 0 ? index : 0);
    }

    const QSignalBlocker blocker(ui->actionHexView);
    ui->actionHexView->setChecked(prefs.globalHexView());
}

///
/// \brief MainWindow::applyGlobalViewStateToForm
/// \param frm
///
void MainWindow::applyGlobalViewStateToForm(QWidget* frm)
{
    if (!frm)
        return;

    const auto& prefs = AppPreferences::instance();
    if (auto* data = qobject_cast<FormDataView*>(frm)) {
        data->setZeroBasedAddress(prefs.globalZeroBasedAddress());
        data->setDisplayHexAddresses(prefs.globalHexView());
    } else if (auto* traffic = qobject_cast<FormTrafficView*>(frm)) {
        traffic->setHexView(prefs.globalHexView());
    } else if (auto* map = qobject_cast<FormDataMapView*>(frm)) {
        map->setZeroBasedAddress(prefs.globalZeroBasedAddress());
        map->setHexView(prefs.globalHexView());
    }
}

///
/// \brief MainWindow::updateMainToolbarState
///
void MainWindow::updateMainToolbarState()
{
    const bool hasProject = hasProjectContext();

    ui->actionNew->setEnabled(true);
    ui->actionOpenProject->setEnabled(true);
    ui->actionSaveProject->setEnabled(hasProject);
    ui->actionPrint->setEnabled(hasProject && ui->actionPrint->isEnabled());
    ui->actionPrintSetup->setEnabled(hasProject && ui->actionPrintSetup->isEnabled());
    ui->actionUndo->setEnabled(hasProject && ui->actionUndo->isEnabled());
    ui->actionRedo->setEnabled(hasProject && ui->actionRedo->isEnabled());
    ui->actionConnect->setEnabled(hasProject);
    ui->actionDisconnect->setEnabled(hasProject);
    ui->actionHexView->setEnabled(hasProject);
    ui->actionMbDefinitions->setEnabled(hasProject);
    ui->actionForceCoils->setEnabled(hasProject);
    ui->actionForceDiscretes->setEnabled(hasProject);
    ui->actionPresetInputRegs->setEnabled(hasProject);
    ui->actionPresetHoldingRegs->setEnabled(hasProject);
    ui->actionCloseProject->setEnabled(hasProject);

    if (_globalAddressBaseWidget)
        _globalAddressBaseWidget->setEnabled(hasProject);
}

///
/// \brief MainWindow::applyGlobalAddressBase
/// \param zeroBased
/// \param persist
///
void MainWindow::applyGlobalAddressBase(bool zeroBased, bool persist)
{
    auto& prefs = AppPreferences::instance();
    if (persist)
        prefs.setGlobalZeroBasedAddress(zeroBased);

    for (auto* frm : _project->forms(ProjectFormKind::Data))
        if (auto* data = qobject_cast<FormDataView*>(frm))
            data->setZeroBasedAddress(zeroBased);

    for (auto* frm : _project->forms(ProjectFormKind::DataMap))
        if (auto* map = qobject_cast<FormDataMapView*>(frm))
            map->setZeroBasedAddress(zeroBased);

    forEachTypedForm(ui->mdiArea, [zeroBased](auto* frm) {
        if (!frm || !frm->property(kSplitAutoCloneProperty).toBool())
            return;

        if (auto* data = qobject_cast<FormDataView*>(frm))
            data->setZeroBasedAddress(zeroBased);
        else if (auto* map = qobject_cast<FormDataMapView*>(frm))
            map->setZeroBasedAddress(zeroBased);
    });

    syncGlobalViewControls();
}

///
/// \brief MainWindow::applyGlobalHexView
/// \param enabled
/// \param persist
///
void MainWindow::applyGlobalHexView(bool enabled, bool persist)
{
    auto& prefs = AppPreferences::instance();
    if (persist)
        prefs.setGlobalHexView(enabled);

    for (auto* frm : _project->forms(ProjectFormKind::Data))
        if (auto* data = qobject_cast<FormDataView*>(frm))
            data->setDisplayHexAddresses(enabled);

    for (auto* frm : _project->forms(ProjectFormKind::Traffic))
        if (auto* traffic = qobject_cast<FormTrafficView*>(frm))
            traffic->setHexView(enabled);

    for (auto* frm : _project->forms(ProjectFormKind::DataMap))
        if (auto* map = qobject_cast<FormDataMapView*>(frm))
            map->setHexView(enabled);

    forEachTypedForm(ui->mdiArea, [enabled](auto* frm) {
        if (!frm || !frm->property(kSplitAutoCloneProperty).toBool())
            return;

        if (auto* data = qobject_cast<FormDataView*>(frm))
            data->setDisplayHexAddresses(enabled);
        else if (auto* traffic = qobject_cast<FormTrafficView*>(frm))
            traffic->setHexView(enabled);
        else if (auto* map = qobject_cast<FormDataMapView*>(frm))
            map->setHexView(enabled);
    });

    syncGlobalViewControls();
}


///
/// \brief MainWindow::confirmSaveOnClose
///
bool MainWindow::confirmSaveOnClose()
{
    const auto button = QMessageBox::question(this,
                                              tr("Save Project"),
                                              tr("Save project before closing?"),
                                              QMessageBox::Save | QMessageBox::No | QMessageBox::Cancel,
                                              QMessageBox::Save);
    if(button == QMessageBox::Cancel)
        return false;

    if(button != QMessageBox::Save)
        return true;

    if(!_projectFilePath.isEmpty()) {
        saveProject(_projectFilePath);
        addRecentProject(_projectFilePath);
        return true;
    }

    const QString before = _projectFilePath;
    on_actionSaveProjectAs_triggered();
    return before != _projectFilePath && !_projectFilePath.isEmpty();
}

///
/// \brief MainWindow::hasProjectContext
///
bool MainWindow::hasProjectContext() const
{
    return !_projectFilePath.isEmpty()
        || _project->firstMdiChild() != nullptr
        || !_project->closedForms().isEmpty();
}

///
/// \brief MainWindow::addRecentProject
///
void MainWindow::addRecentProject(const QString& filePath)
{
    if(filePath.isEmpty())
        return;

    const QString absolutePath = QFileInfo(filePath).absoluteFilePath();
    if(absolutePath.isEmpty())
        return;

    for (int i = _recentProjects.size() - 1; i >= 0; --i) {
        if (_recentProjects[i].compare(absolutePath, Qt::CaseInsensitive) == 0)
            _recentProjects.removeAt(i);
    }
    _recentProjects.prepend(absolutePath);
    while (_recentProjects.size() > kMaxRecentProjects)
        _recentProjects.removeLast();

    rebuildRecentProjectsMenu();
}

///
/// \brief MainWindow::rebuildRecentProjectsMenu
///
void MainWindow::rebuildRecentProjectsMenu()
{
    if(!_openRecentMenu)
        return;

    _openRecentMenu->clear();
    QStringList existing;
    existing.reserve(_recentProjects.size());
    for (const auto& path : _recentProjects) {
        if(path.isEmpty())
            continue;
        if(!QFile::exists(path))
            continue;
        existing.append(path);
    }
    _recentProjects = existing;

    if(_recentProjects.isEmpty()) {
        auto* empty = _openRecentMenu->addAction(tr("No Recent Projects"));
        empty->setEnabled(false);
    } else {
        for(const auto& path : _recentProjects) {
            auto* action = _openRecentMenu->addAction(path);
            connect(action, &QAction::triggered, this, [this, path]() {
                openRecentProject(path);
            });
        }
    }

    _openRecentMenu->addSeparator();
    _clearRecentAction = _openRecentMenu->addAction(tr("Clear List"));
    _clearRecentAction->setEnabled(!_recentProjects.isEmpty());
    connect(_clearRecentAction, &QAction::triggered, this, &MainWindow::clearRecentProjects);
}

///
/// \brief MainWindow::clearRecentProjects
///
void MainWindow::clearRecentProjects()
{
    _recentProjects.clear();
    rebuildRecentProjectsMenu();
}

///
/// \brief MainWindow::openRecentProject
///
void MainWindow::openRecentProject(const QString& filePath)
{
    if(!QFile::exists(filePath)) {
        _recentProjects.removeAll(filePath);
        rebuildRecentProjectsMenu();
        return;
    }

    loadProject(filePath);
    addRecentProject(filePath);
}


