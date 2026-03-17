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
#include "dialogselectserviceport.h"
#include "dialogsetupserialport.h"
#include "dialogsetuppresetdata.h"
#include "dialogforcemultiplecoils.h"
#include "dialogforcemultipleregisters.h"
#include "dialogmodbusdefinitions.h"
#include "mainstatusbar.h"
#include "menuconnect.h"
#include "controls/mdiareaex.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

// Forward declaration (defined later in this file)
static QString getSettingsFilePath();
namespace {
constexpr const char* kSplitAutoCloneProperty = "SplitAutoClone";
constexpr const char* kNewFormKindKey = "NewFormKind";

ProjectFormKind newFormKindFromSetting(int value)
{
    switch (static_cast<ProjectFormKind>(value)) {
        case ProjectFormKind::Data:
        case ProjectFormKind::Traffic:
        case ProjectFormKind::Script:
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
    }
}

ProjectFormKind projectFormKindFromWidget(QWidget* widget, bool* ok = nullptr)
{
    if (qobject_cast<FormDataView*>(widget)) {
        if(ok) *ok = true;
        return ProjectFormKind::Data;
    }
    if (qobject_cast<FormTrafficView*>(widget)) {
        if(ok) *ok = true;
        return ProjectFormKind::Traffic;
    }
    if (qobject_cast<FormScriptView*>(widget)) {
        if(ok) *ok = true;
        return ProjectFormKind::Script;
    }

    if(ok) *ok = false;
    return ProjectFormKind::Data;
}

template<typename TDefinitions>
void applySharedDisplayDefaults(TDefinitions& target, const TDefinitions& defaults)
{
    target.ZeroBasedAddress = defaults.ZeroBasedAddress;
    target.HexAddress = defaults.HexAddress;
    target.LeadingZeros = defaults.LeadingZeros;
    target.DataViewColumnsDistance = defaults.DataViewColumnsDistance;
    target.AutoscrollLog = defaults.AutoscrollLog;
    target.VerboseLogging = defaults.VerboseLogging;
    target.LogViewLimit = defaults.LogViewLimit;
}

void applySharedDisplayDefaults(TrafficViewDefinitions& target, const TrafficViewDefinitions& defaults)
{
    applySharedDisplayDefaults<TrafficViewDefinitions>(target, defaults);
    target.ScriptCfg = defaults.ScriptCfg;
}

void applySharedDisplayDefaults(ScriptViewDefinitions& target, const ScriptViewDefinitions& defaults)
{
    applySharedDisplayDefaults<ScriptViewDefinitions>(target, defaults);
    target.ScriptCfg = defaults.ScriptCfg;
}

DisplayMode displayModeOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->displayMode();
    if (qobject_cast<FormTrafficView*>(widget)) return DisplayMode::Traffic;
    if (qobject_cast<FormScriptView*>(widget)) return DisplayMode::Script;
    return DisplayMode::Data;
}

DataDisplayMode dataDisplayModeOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->dataDisplayMode();
    return DataDisplayMode::Hex;
}

void setDataDisplayModeOnForm(QWidget* widget, DataDisplayMode mode)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setDataDisplayMode(mode);
}

ByteOrder byteOrderOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->byteOrder();
    return ByteOrder::Direct;
}

void setByteOrderOnForm(QWidget* widget, ByteOrder order)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setByteOrder(order);
}

CaptureMode captureModeOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->captureMode();
    return CaptureMode::Off;
}

void startTextCaptureOnForm(QWidget* widget, const QString& file)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->startTextCapture(file);
}

void stopTextCaptureOnForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->stopTextCapture();
}

void resetCtrsOnForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->resetCtrs();
}

bool displayHexAddressesOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->displayHexAddresses();
    return false;
}

void setDisplayHexAddressesOnForm(QWidget* widget, bool on)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setDisplayHexAddresses(on);
}

void printOnForm(QWidget* widget, QPrinter* printer)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->print(printer);
}

void setCodepageOnForm(QWidget* widget, const QString& name)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setCodepage(name);
}

QColor statusColorOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->statusColor();
    return QColor();
}

QColor backgroundColorOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->backgroundColor();
    if (auto* frm = qobject_cast<FormTrafficView*>(widget)) return frm->backgroundColor();
    if (auto* frm = qobject_cast<FormScriptView*>(widget)) return frm->backgroundColor();
    return QColor();
}

QColor foregroundColorOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->foregroundColor();
    if (auto* frm = qobject_cast<FormTrafficView*>(widget)) return frm->foregroundColor();
    if (auto* frm = qobject_cast<FormScriptView*>(widget)) return frm->foregroundColor();
    return QColor();
}

void setStatusColorOnForm(QWidget* widget, const QColor& clr)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setStatusColor(clr);
}

void setBackgroundColorOnForm(QWidget* widget, const QColor& clr)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setBackgroundColor(clr);
    else if (auto* frm = qobject_cast<FormTrafficView*>(widget)) frm->setBackgroundColor(clr);
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->setBackgroundColor(clr);
}

void setForegroundColorOnForm(QWidget* widget, const QColor& clr)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setForegroundColor(clr);
    else if (auto* frm = qobject_cast<FormTrafficView*>(widget)) frm->setForegroundColor(clr);
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->setForegroundColor(clr);
}

void setScriptFontOnForm(QWidget* widget, const QFont& font)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setScriptFont(font);
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->setFont(font);
}

void setZoomPercentOnForm(QWidget* widget, int zoomPercent)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setZoomPercent(zoomPercent);
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->setZoomPercent(zoomPercent);
}

DataViewDefinitions toDataViewDefinitions(const FormDisplayDefinition& definition)
{
    return std::visit([](const auto& value) -> DataViewDefinitions {
        return toDataViewDefinitions(value);
    }, definition);
}

TrafficViewDefinitions toTrafficViewDefinitions(const FormDisplayDefinition& definition)
{
    return std::visit([](const auto& value) -> TrafficViewDefinitions {
        return toTrafficViewDefinitions(value);
    }, definition);
}

ScriptViewDefinitions toScriptViewDefinitions(const FormDisplayDefinition& definition)
{
    return std::visit([](const auto& value) -> ScriptViewDefinitions {
        return toScriptViewDefinitions(value);
    }, definition);
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

    ui->actionNew->setIcon(QIcon(":/res/icon-new-form.svg"));
    ui->actionNew->setMenu(ui->menuNew);
    if (auto* newButton = qobject_cast<QToolButton*>(ui->toolBarMain->widgetForAction(ui->actionNew))) {
        newButton->setPopupMode(QToolButton::MenuButtonPopup);
    }

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

    _projectTree = new ProjectTreeWidget(this);
    _projectDockWidget = new QDockWidget(tr("Project"), this);
    _projectDockWidget->setObjectName("projectDockWidget");
    _projectDockWidget->setWidget(_projectTree);
    _projectDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, _projectDockWidget);

    _project = new AppProject(ui->mdiArea, _mbMultiServer, _dataSimulator,
                               _projectTree, _windowActionList, this, this);

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
        auto* frm = ref.widget;
        if(!frm)
            return;
        // If the form is currently open in a tab, activate it
        for (auto wnd : ui->mdiArea->subWindowList()) {
            if (wnd->widget() == frm) {
                ui->mdiArea->setActiveSubWindow(wnd);
                return;
            }
        }
        // Form tab was closed but form is still alive — re-open it
        if (_project->isFormClosed(frm))
            _project->rewrapMdiChild(frm);
    });
    connect(_projectTree, &ProjectTreeWidget::formDeleteRequested, this, [this](ProjectFormRef ref) {
        if(!ref.widget)
            return;
        _project->deleteForm(ref.widget);
    });
    connect(_projectTree, &ProjectTreeWidget::formRenamed, this, [this](ProjectFormRef) {
        _windowActionList->update();
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
        _project->removeSplitAutoClonesFromSecondary();
    });
    connect(ui->mdiArea, &MdiAreaEx::splitViewToggled, this, [this](bool enabled) {
        if(enabled)
            _project->duplicatePrimaryTabsToSecondary();
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
            _newFormKind = newFormKindFromSetting(
                m.value(kNewFormKindKey, newFormKindToSetting(ProjectFormKind::Data)).toInt());
        }
    }
}

///
/// \brief MainWindow::~MainWindow
///
MainWindow::~MainWindow()
{
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
        m.setValue(kNewFormKindKey, newFormKindToSetting(_newFormKind));
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
                auto* frm = wnd->widget();
                if (frm && !frm->property(kSplitAutoCloneProperty).toBool()) {
                    // Primary form: reparent before subwindow is destroyed so frm survives
                    _project->markFormClosed(frm);
                }
            }
        break;
        case QEvent::Move:
            if(auto wnd = qobject_cast<const QMdiSubWindow*>(obj))
            {
                auto* widget = wnd->widget();
                if(!widget || wnd->isMinimized() || wnd->isMaximized())
                    break;

                if (auto* frm = qobject_cast<FormDataView*>(widget))
                    frm->setParentGeometry(wnd->geometry());
                else if (auto* frm = qobject_cast<FormTrafficView*>(widget))
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
    auto* dataFrm = currentDataForm();
    auto* trafficFrm = currentTrafficForm();
    auto* scriptFrm = currentScriptForm();
    auto* dataLikeFrm = currentDataOrTrafficForm();

    ui->menuSetup->setEnabled(frm != nullptr);
    ui->menuWindow->setEnabled(frm != nullptr);

    ui->actionPrintSetup->setEnabled(_selectedPrinter != nullptr && dataLikeFrm != nullptr);
    ui->actionPrint->setEnabled(_selectedPrinter != nullptr && dataLikeFrm != nullptr && displayModeOfForm(dataLikeFrm) == DisplayMode::Data);

    ui->actionUndo->setEnabled(scriptFrm != nullptr);
    ui->actionRedo->setEnabled(scriptFrm != nullptr);
    ui->actionCut->setEnabled(scriptFrm != nullptr);
    ui->actionCopy->setEnabled(scriptFrm != nullptr);
    ui->actionPaste->setEnabled(scriptFrm != nullptr);
    ui->actionSelectAll->setEnabled(scriptFrm != nullptr);
    const bool isScriptActive = (scriptFrm != nullptr);
    ui->actionFind->setEnabled(isScriptActive);
    ui->actionReplace->setEnabled(isScriptActive);

    ui->actionShowData->setEnabled(true);
    ui->actionShowTraffic->setEnabled(true);
    ui->actionShowScript->setEnabled(true);
    ui->actionBinary->setEnabled(dataLikeFrm != nullptr);
    ui->actionUInt16->setEnabled(dataLikeFrm != nullptr);
    ui->actionInt16->setEnabled(dataLikeFrm != nullptr);
    ui->actionInt32->setEnabled(dataLikeFrm != nullptr);
    ui->actionSwappedInt32->setEnabled(dataLikeFrm != nullptr);
    ui->actionUInt32->setEnabled(dataLikeFrm != nullptr);
    ui->actionSwappedUInt32->setEnabled(dataLikeFrm != nullptr);
    ui->actionInt64->setEnabled(dataLikeFrm != nullptr);
    ui->actionSwappedInt64->setEnabled(dataLikeFrm != nullptr);
    ui->actionUInt64->setEnabled(dataLikeFrm != nullptr);
    ui->actionSwappedUInt64->setEnabled(dataLikeFrm != nullptr);
    ui->actionHex->setEnabled(dataLikeFrm != nullptr);
    ui->actionAnsi->setEnabled(dataLikeFrm != nullptr);
    ui->actionHex->setEnabled(dataLikeFrm != nullptr);
    ui->actionFloatingPt->setEnabled(dataLikeFrm != nullptr);
    ui->actionSwappedFP->setEnabled(dataLikeFrm != nullptr);
    ui->actionDblFloat->setEnabled(dataLikeFrm != nullptr);
    ui->actionSwappedDbl->setEnabled(dataLikeFrm != nullptr);
    ui->actionSwapBytes->setEnabled(dataLikeFrm != nullptr);

    ui->actionRawDataLog->setChecked(false);

    ui->actionTextCapture->setEnabled(dataLikeFrm && captureModeOfForm(dataLikeFrm) == CaptureMode::Off);
    ui->actionCaptureOff->setEnabled(dataLikeFrm && captureModeOfForm(dataLikeFrm) == CaptureMode::TextCapture);

    const bool scriptRunning = scriptFrm && _project->isScriptRunningOnSplitPair(scriptFrm);
    ui->actionImportScript->setEnabled(scriptFrm != nullptr);
    ui->actionRunScript->setEnabled(scriptFrm && !scriptRunning && scriptFrm->canRunScript());
    ui->actionStopScript->setEnabled(scriptRunning);

    ui->actionTabbedView->setChecked(ui->mdiArea->viewMode() == QMdiArea::TabbedView);
    ui->actionSplitView->setVisible(ui->mdiArea->viewMode() == QMdiArea::TabbedView);
    ui->actionSplitView->setChecked(ui->mdiArea->isSplitView());
    ui->actionToolbar->setChecked(ui->toolBarMain->isVisible());
    ui->actionStatusBar->setChecked(statusBar()->isVisible());
    ui->actionScriptHelp->setChecked(_helpDockWidget->isVisible());
    const bool formInScriptMode = (scriptFrm != nullptr);
    ui->actionScriptHelp->setVisible(formInScriptMode);
    ui->actionConsoleOutput->setVisible(true);
    ui->actionConsoleOutput->setChecked(_consoleDockWidget->isVisible());

    ui->actionTile->setEnabled(ui->mdiArea->viewMode() == QMdiArea::SubWindowView);
    ui->actionCascade->setEnabled(ui->mdiArea->viewMode() == QMdiArea::SubWindowView);

    if(dataLikeFrm != nullptr)
    {
        const auto dd = dataFrm ? dataFrm->displayDefinition()
                                : toDataViewDefinitions(trafficFrm->displayDefinition());
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

        const auto ddm = dataDisplayModeOfForm(dataLikeFrm);
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

        const auto byteOrder = byteOrderOfForm(dataLikeFrm);
        ui->actionSwapBytes->setChecked(byteOrder == ByteOrder::Swapped);

        ui->actionHexAddresses->setChecked(displayHexAddressesOfForm(dataLikeFrm));
    }

    ui->actionShowData->setChecked(dataFrm != nullptr);
    ui->actionShowTraffic->setChecked(trafficFrm != nullptr);
    ui->actionShowScript->setChecked(scriptFrm != nullptr);

    if(frm)
        _projectTree->activateForm(frm);
}

///
/// \brief MainWindow::on_actionNew_triggered
///
void MainWindow::on_actionNew_triggered()
{
    createNewForm(_newFormKind);
}

///
/// \brief MainWindow::on_actionNewDataView_triggered
///
void MainWindow::on_actionNewDataView_triggered()
{
    _newFormKind = ProjectFormKind::Data;
    createNewForm(_newFormKind);
}

///
/// \brief MainWindow::on_actionNewTrafficView_triggered
///
void MainWindow::on_actionNewTrafficView_triggered()
{
    _newFormKind = ProjectFormKind::Traffic;
    createNewForm(_newFormKind);
}

///
/// \brief MainWindow::createNewForm
/// \param kind
///
void MainWindow::createNewForm(ProjectFormKind kind)
{
    const auto cur = _project->currentMdiChild();
    _project->setWindowCounter(_project->windowCounter() + 1);
    auto* frm = _project->createMdiChild(_project->windowCounter(), kind);
    if(!frm)
        return;

    const auto& prefs = AppPreferences::instance();

    if(cur) {
        setByteOrderOnForm(frm, byteOrderOfForm(cur));
        setDataDisplayModeOnForm(frm, dataDisplayModeOfForm(cur));
        frm->setFont(cur->font());
        setStatusColorOnForm(frm, statusColorOfForm(cur));
        setBackgroundColorOnForm(frm, backgroundColorOfForm(cur));
        setForegroundColorOnForm(frm, foregroundColorOfForm(cur));
    }
    else {
        frm->setFont(prefs.font());
        setBackgroundColorOnForm(frm, prefs.backgroundColor());
        setForegroundColorOnForm(frm, prefs.foregroundColor());
        setStatusColorOnForm(frm, prefs.statusColor());
    }
    setScriptFontOnForm(frm, prefs.scriptFont());
    setZoomPercentOnForm(frm, prefs.fontZoom());

    switch(kind)
    {
        case ProjectFormKind::Data: {
            if (auto* dataFrm = qobject_cast<FormDataView*>(frm)) {
                auto dd = dataFrm->displayDefinition();
                applySharedDisplayDefaults(dd, prefs.dataViewDefinitions());
                dataFrm->setDisplayDefinition(dd);
            }
            break;
        }
        case ProjectFormKind::Traffic: {
            if (auto* trafficFrm = qobject_cast<FormTrafficView*>(frm)) {
                auto dd = trafficFrm->displayDefinition();
                applySharedDisplayDefaults(dd, prefs.trafficViewDefinitions());
                trafficFrm->setDisplayDefinition(dd);
            }
            break;
        }
        case ProjectFormKind::Script: {
            if (auto* scriptFrm = qobject_cast<FormScriptView*>(frm)) {
                auto dd = scriptFrm->displayDefinition();
                applySharedDisplayDefaults(dd, prefs.scriptViewDefinitions());
                scriptFrm->setDisplayDefinition(dd);
            }
            break;
        }
    }

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

    const auto filename = QFileDialog::getOpenFileName(this, QString(), _project->savePath(), filters.join(";;"));
    if(filename.isEmpty()) return;

    _project->setSavePath(QFileInfo(filename).absoluteDir().absolutePath());
    loadProject(filename);
}

///
/// \brief MainWindow::on_actionSaveProjectAs_triggered
///
void MainWindow::on_actionSaveProjectAs_triggered()
{
    QStringList filters;
    filters << tr("Project files (*.msimprj)");
    auto filename = QFileDialog::getSaveFileName(this, QString(), _project->savePath(), filters.join(";;"));

    if(filename.isEmpty()) return;

    if(!filename.endsWith(".msimprj", Qt::CaseInsensitive))
        filename.append(".msimprj");

    _project->setSavePath(QFileInfo(filename).absoluteDir().absolutePath());
    saveProject(filename);
}

///
/// \brief MainWindow::on_actionCloseProject_triggered
///
void MainWindow::on_actionCloseProject_triggered()
{
    _project->closeProject();
}

///
/// \brief MainWindow::on_actionPrint_triggered
///
void MainWindow::on_actionPrint_triggered()
{
    auto* frm = currentDataOrTrafficForm();
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
    forEachTypedForm(ui->mdiArea, [enable](auto* frm) {
        if (auto* data = qobject_cast<FormDataView*>(frm))
            data->enableAutoComplete(enable);
        else if (auto* script = qobject_cast<FormScriptView*>(frm))
            script->enableAutoComplete(enable);
    });
}

///
/// \brief MainWindow::applyScriptFont
/// \param font
///
void MainWindow::applyFont(const QFont& font)
{
    forEachTypedForm(ui->mdiArea, [&font](auto* frm) { frm->setFont(font); });
}

///
/// \brief MainWindow::applyScriptFont
/// \param font
///
void MainWindow::applyScriptFont(const QFont& font)
{
    forEachTypedForm(ui->mdiArea, [&font](auto* frm) {
        if (auto* data = qobject_cast<FormDataView*>(frm))
            data->setScriptFont(font);
        else if (auto* script = qobject_cast<FormScriptView*>(frm))
            script->setFont(font);
    });
}

///
/// \brief MainWindow::applyColors
/// \param bg
/// \param fg
/// \param status
///
void MainWindow::applyColors(const QColor& bg, const QColor& fg, const QColor& status)
{
    forEachTypedForm(ui->mdiArea, [&bg, &fg, &status](auto* frm) {
        frm->setBackgroundColor(bg);
        frm->setForegroundColor(fg);
        if (auto* data = qobject_cast<FormDataView*>(frm))
            data->setStatusColor(status);
    });
}

///
/// \brief MainWindow::applyDataViewDefaults
/// \param dd
///
void MainWindow::applyDataViewDefaults(const DataViewDefinitions& dd)
{
    for (auto&& wnd : ui->mdiArea->subWindowList()) {
        if (auto* frm = qobject_cast<FormDataView*>(wnd->widget())) {
            auto cur = frm->displayDefinition();
            applySharedDisplayDefaults(cur, dd);
            frm->setDisplayDefinition(cur);
        }
    }
}

///
/// \brief MainWindow::applyTrafficViewDefaults
/// \param dd
///
void MainWindow::applyTrafficViewDefaults(const TrafficViewDefinitions& dd)
{
    for (auto&& wnd : ui->mdiArea->subWindowList()) {
        if (auto* frm = qobject_cast<FormTrafficView*>(wnd->widget())) {
            auto cur = frm->displayDefinition();
            applySharedDisplayDefaults(cur, dd);
            frm->setDisplayDefinition(cur);
        }
    }
}

///
/// \brief MainWindow::applyScriptViewDefaults
/// \param dd
///
void MainWindow::applyScriptViewDefaults(const ScriptViewDefinitions& dd)
{
    for (auto&& wnd : ui->mdiArea->subWindowList()) {
        if (auto* frm = qobject_cast<FormScriptView*>(wnd->widget())) {
            auto cur = frm->displayDefinition();
            applySharedDisplayDefaults(cur, dd);
            frm->setDisplayDefinition(cur);
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
/// \brief MainWindow::on_actionShowData_triggered
///
void MainWindow::on_actionShowData_triggered()
{
    _project->setWindowCounter(_project->windowCounter() + 1);
    if (auto frm = _project->createDataMdiChild(_project->windowCounter())) {
        frm->show();
    }
}

///
/// \brief MainWindow::on_actionShowTraffic_triggered
///
void MainWindow::on_actionShowTraffic_triggered()
{
    _project->setWindowCounter(_project->windowCounter() + 1);
    if (auto frm = _project->createTrafficMdiChild(_project->windowCounter())) {
        frm->show();
    }
}

///
/// \brief MainWindow::on_actionShowScript_triggered
/// Opens a new standalone script document.
///
void MainWindow::on_actionShowScript_triggered()
{
    _project->setWindowCounter(_project->windowCounter() + 1);
    if (auto frm = _project->createScriptMdiChild(_project->windowCounter())) {
        frm->show();
    }
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
    auto* frm = currentDataOrTrafficForm();
    if(!frm) return;

    switch (byteOrderOfForm(frm)) {
    case ByteOrder::Swapped:
        setByteOrderOnForm(frm, ByteOrder::Direct);
        break;
    case ByteOrder::Direct:
        setByteOrderOnForm(frm, ByteOrder::Swapped);
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
    auto* frm = currentDataOrTrafficForm();
    if(frm) setDisplayHexAddressesOnForm(frm, !displayHexAddressesOfForm(frm));
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
    const auto mode = frm ? dataDisplayModeOfForm(frm) : DataDisplayMode::Hex;

    auto dlg = new DialogMsgParser(mode, ModbusMessage::Rtu);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    dlg->show();
}

///
/// \brief MainWindow::on_actionRawDataLog_triggered
///
void MainWindow::on_actionRawDataLog_triggered()
{
    on_actionShowTraffic_triggered();
}

///
/// \brief MainWindow::on_actionTextCapture_triggered
///
void MainWindow::on_actionTextCapture_triggered()
{
    auto* frm = currentDataOrTrafficForm();
    if(!frm) return;

    auto filename = QFileDialog::getSaveFileName(this, QString(), QString(), "Text files (*.txt)");
    if(!filename.isEmpty())
    {
        if(!filename.endsWith(".txt", Qt::CaseInsensitive)) filename += ".txt";
        startTextCaptureOnForm(frm, filename);
    }
}

///
/// \brief MainWindow::on_actionCaptureOff_triggered
///
void MainWindow::on_actionCaptureOff_triggered()
{
    auto* frm = currentDataOrTrafficForm();
    if(!frm) return;

    stopTextCaptureOnForm(frm);
}

///
/// \brief MainWindow::on_actionResetCtrs_triggered
///
void MainWindow::on_actionResetCtrs_triggered()
{
    auto* frm = currentDataOrTrafficForm();
    if(!frm)
        return;

    resetCtrsOnForm(frm);
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
}

///
/// \brief MainWindow::updateHelpWidgetState
///
void MainWindow::updateHelpWidgetState()
{
    auto frm = _project->currentMdiChild();
    if(!frm) return;

    switch(displayModeOfForm(frm))
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
    _newFormKind = ProjectFormKind::Script;
    createNewForm(_newFormKind);
}

///
/// \brief MainWindow::on_actionImportScript_triggered
///
void MainWindow::on_actionImportScript_triggered()
{
    auto* frm = currentScriptForm();
    if(!frm) return;

    const auto filename = QFileDialog::getOpenFileName(this, QString(), QString(), tr("JavaScript files (*.js);;All files (*)"));
    if(filename.isEmpty()) return;

    QFile file(filename);
    if(!file.open(QFile::ReadOnly | QFile::Text)) return;

    frm->setScript(QTextStream(&file).readAll());
}

///
/// \brief MainWindow::on_actionRunScript_triggered
///
void MainWindow::on_actionRunScript_triggered()
{
    auto* frm = currentScriptForm();
    if(!frm) return;

    if(_project->isScriptRunningOnSplitPair(frm))
        return;

    frm->runScript();
}

///
/// \brief MainWindow::on_actionStopScript_triggered
///
void MainWindow::on_actionStopScript_triggered()
{
    auto* frm = currentScriptForm();
    if(!frm) return;

    if(frm->canStopScript())
        frm->stopScript();
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

QWidget* MainWindow::currentForm() const
{
    return _project->currentMdiChild();
}

FormDataView* MainWindow::currentDataForm() const
{
    return _project->currentDataMdiChild();
}

FormTrafficView* MainWindow::currentTrafficForm() const
{
    return _project->currentTrafficMdiChild();
}

FormScriptView* MainWindow::currentScriptForm() const
{
    return _project->currentScriptMdiChild();
}

QWidget* MainWindow::currentDataOrTrafficForm() const
{
    if (auto* data = currentDataForm())
        return data;
    return currentTrafficForm();
}

///
/// \brief MainWindow::setCodepage
/// \param name
///
void MainWindow::setCodepage(const QString& name)
{
    auto* frm = currentForm();
    if(!frm) return;

    setCodepageOnForm(frm, name);
}


///
/// \brief MainWindow::updateDisplayMode
/// \param mode
///
void MainWindow::updateDataDisplayMode(DataDisplayMode mode)
{
    auto* frm = currentDataOrTrafficForm();
    if(frm) setDataDisplayModeOnForm(frm, mode);
}

///
/// \brief MainWindow::forceCoils
/// \param type
///
void MainWindow::forceCoils(QModbusDataUnit::RegisterType type)
{
    auto* frm = currentDataForm();
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
    auto* frm = currentDataForm();
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
/// \brief MainWindow::loadProject
/// \param filename
///
void MainWindow::loadProject(const QString& filename)
{
    _project->loadProject(filename);
}

///
/// \brief MainWindow::saveProject
/// \param filename
///
void MainWindow::saveProject(const QString& filename)
{
    _project->saveProject(filename);
}

///
/// \brief MainWindow::selectAnsiCodepage
/// \param name
///
void MainWindow::selectAnsiCodepage(const QString& name)
{
    _ansiMenu->selectCodepage(name);
}

///
/// \brief MainWindow::showConsoleMessage
///
void MainWindow::showConsoleMessage(const QString& source, const QString& text, ConsoleOutput::MessageType type)
{
    _consoleDockWidget->setVisible(true);
    _globalConsole->addMessage(text, type, source);
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
/// \brief MainWindow::runScriptIcon
///
QIcon MainWindow::runScriptIcon() const
{
    return ui->actionRunScript->icon();
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
    _newFormKind = newFormKindFromSetting(
        m.value(kNewFormKindKey, newFormKindToSetting(ProjectFormKind::Data)).toInt());

    restoreGeometry(m.value("WindowGeometry").toByteArray());

    const auto viewMode = (QMdiArea::ViewMode)qBound(0, m.value("ViewMode", QMdiArea::TabbedView).toInt(), 1);
    setViewMode(viewMode);
    const bool splitView = m.value("SplitView", false).toBool();
    if(ui->mdiArea->viewMode() == QMdiArea::TabbedView && ui->mdiArea->isSplitView() != splitView)
        ui->mdiArea->setSplitViewEnabled(splitView);

    statusBar()->setVisible(m.value("StatusBar", true).toBool());

    _lang = m.value("Language", translationLang()).toString();
    setLanguage(_lang);

    _project->setSavePath(m.value("SavePath").toString());

    ModbusDefinitions defs;
    m >> defs;
    _mbMultiServer.setModbusDefinitions(defs);

    m >> qobject_cast<MenuConnect*>(ui->actionConnect->menu());

    const QStringList groups = m.childGroups();
    for (const QString& g : groups) {
        if (g.startsWith("Form_")) {
            m.beginGroup(g);
            const int id = m.value("FromId", _project->windowCounter() + 1).toInt();
            _project->setWindowCounter(qMax(_project->windowCounter(), id));
            const auto kind = static_cast<ProjectFormKind>(m.value("FormKind", static_cast<int>(ProjectFormKind::Data)).toInt());
            MdiArea* targetArea = ui->mdiArea->primaryArea();
            const auto panel = m.value("SplitPanel", "L").toString();
            if(splitView && panel.compare("R", Qt::CaseInsensitive) == 0)
                if(auto secondary = _project->splitSecondaryArea())
                    targetArea = secondary;

            switch (kind) {
                case ProjectFormKind::Data:
                    if (auto* frm = _project->createDataMdiChildOnArea(id, targetArea, true)) {
                        frm->loadSettings(m);
                        frm->show();
                    }
                    break;
                case ProjectFormKind::Traffic:
                    if (auto* frm = _project->createTrafficMdiChildOnArea(id, targetArea, true)) {
                        frm->loadSettings(m);
                        frm->show();
                    }
                    break;
                case ProjectFormKind::Script:
                    if (auto* frm = _project->createScriptMdiChildOnArea(id, targetArea, true)) {
                        frm->loadSettings(m);
                        frm->show();
                    }
                    break;
            }
            m.endGroup();
        }
    }

    if(splitView)
    {
        const auto activeSecWin = m.value("ActiveSecondaryWindow").toString();
        if(!activeSecWin.isEmpty())
            if(auto secondary = _project->splitSecondaryArea())
                for(auto&& wnd : secondary->localSubWindowList())
                    if(wnd && wnd->widget() && wnd->widget()->windowTitle() == activeSecWin)
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
            auto* frm = wnd ? wnd->widget() : nullptr;
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

    const auto frm = _project->currentMdiChild();
    if(frm) m.setValue("ActiveWindow", frm->windowTitle());

    if(auto primary = ui->mdiArea->primaryArea())
        if(auto wnd = primary->activeSubWindow())
            if(auto frmPrimary = wnd->widget())
                m.setValue("ActivePrimaryWindow", frmPrimary->windowTitle());

    m.setValue("ViewMode", ui->mdiArea->viewMode());
    m.setValue("SplitView", ui->mdiArea->isSplitView());
    m.setValue("StatusBar", statusBar()->isVisible());
    m.setValue("Language", _lang);
    m.setValue("SavePath", _project->savePath());
    m.setValue(kNewFormKindKey, newFormKindToSetting(_newFormKind));

    m << _mbMultiServer.getModbusDefinitions();

    m << qobject_cast<MenuConnect*>(ui->actionConnect->menu());

    auto* secondary = _project->splitSecondaryArea();
    const auto subWindowList = ui->mdiArea->subWindowList();
    int groupIndex = 0;
    for(int i = 0; i < subWindowList.size(); ++i) {
        auto* widget = subWindowList[i] ? subWindowList[i]->widget() : nullptr;
        if(!widget || widget->property(kSplitAutoCloneProperty).toBool())
            continue;

        bool okKind = false;
        const auto formKind = projectFormKindFromWidget(widget, &okKind);
        if(!okKind)
            continue;

        ++groupIndex;
        m.beginGroup("Form_" + QString::number(groupIndex));
        m.setValue("FormKind", static_cast<int>(formKind));
        const bool onRight = secondary && secondary->localSubWindowList().contains(subWindowList[i]);
        m.setValue("SplitPanel", onRight ? "R" : "L");

        if (auto* dataFrm = qobject_cast<FormDataView*>(widget)) {
            m.setValue("FromId", dataFrm->formId());
            dataFrm->saveSettings(m);
        } else if (auto* trafficFrm = qobject_cast<FormTrafficView*>(widget)) {
            m.setValue("FromId", trafficFrm->formId());
            trafficFrm->saveSettings(m);
        } else if (auto* scriptFrm = qobject_cast<FormScriptView*>(widget)) {
            m.setValue("FromId", scriptFrm->formId());
            scriptFrm->saveSettings(m);
        }

        m.endGroup();
    }

    if(_project->isSplitTabbedView())
        if(auto secondary = _project->splitSecondaryArea())
            if(auto wnd = secondary->activeSubWindow())
                if(auto frm = wnd->widget())
                    m.setValue("ActiveSecondaryWindow", frm->windowTitle());
}

