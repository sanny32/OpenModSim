#include <QtWidgets>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QPageSetupDialog>
#include "dialogabout.h"
#include "dialogwindowsmanager.h"
#include "dialogprintsettings.h"
#include "dialogdisplaydefinition.h"
#include "dialogselectserviceport.h"
#include "dialogsetupserialport.h"
#include "dialogsetuppresetdata.h"
#include "dialogscriptsettings.h"
#include "dialogforcemultiplecoils.h"
#include "dialogforcemultipleregisters.h"
#include "runmodecombobox.h"
#include "mainstatusbar.h"
#include "menuconnect.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

///
/// \brief MainWindow::MainWindow
/// \param parent
///
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,_lang("en")
    ,_icoBigEndian(":/res/actionBigEndian.png")
    ,_icoLittleEndian(":/res/actionLittleEndian.png")
    ,_windowCounter(0)
    ,_dataSimulator(new DataSimulator(this))
{
    ui->setupUi(this);

    setWindowTitle(APP_NAME);
    setUnifiedTitleAndToolBarOnMac(true);
    setStatusBar(new MainStatusBar(_mbMultiServer, this));

    auto menuByteOrder = new QMenu(this);
    menuByteOrder->addAction(ui->actionLittleEndian);
    menuByteOrder->addAction(ui->actionBigEndian);
    ui->actionByteOrder->setMenu(menuByteOrder);
    ui->actionByteOrder->setIcon(_icoLittleEndian);
    qobject_cast<QToolButton*>(ui->toolBarDisplay->widgetForAction(ui->actionByteOrder))->setPopupMode(QToolButton::InstantPopup);

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

    auto comboBoxRunMode = new RunModeComboBox(this);
    connect(comboBoxRunMode, &RunModeComboBox::runModeChanged, this, &MainWindow::on_runModeChanged);

    _actionRunMode = new QWidgetAction(this);
    _actionRunMode->setDefaultWidget(comboBoxRunMode);
    ui->toolBarScript->insertAction(ui->actionRunScript, _actionRunMode);
    qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionRunScript))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionStopScript))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    /*auto labelJS = new QLabel(this);
    labelJS->setPixmap(QPixmap(":/res/jslogo.svg"));
    labelJS->setContentsMargins(2, 0, 6, 0);
    ui->toolBarScript->insertWidget(_actionRunMode, labelJS);*/

    const auto defaultPrinter = QPrinterInfo::defaultPrinter();
    if(!defaultPrinter.isNull())
        _selectedPrinter = QSharedPointer<QPrinter>(new QPrinter(defaultPrinter));

    _recentFileActionList = new RecentFileActionList(ui->menuFile, ui->actionRecentFile);
    connect(_recentFileActionList, &RecentFileActionList::triggered, this, &MainWindow::openFile);

    _windowActionList = new WindowActionList(ui->menuWindow, ui->actionWindows);
    connect(_windowActionList, &WindowActionList::triggered, this, &MainWindow::windowActivate);

    auto dispatcher = QAbstractEventDispatcher::instance();
    connect(dispatcher, &QAbstractEventDispatcher::awake, this, &MainWindow::on_awake);

    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::updateMenuWindow);
    connect(_dataSimulator.get(), &DataSimulator::dataSimulated, this, &MainWindow::on_dataSimulated);
    connect(&_mbMultiServer, &ModbusMultiServer::connectionError, this, &MainWindow::on_connectionError);

    ui->actionNew->trigger();
    loadSettings();
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

        if(_qtTranslator.load(QString("%1/translations/qt_%2").arg(qApp->applicationDirPath(), lang)))
            qApp->installTranslator(&_qtTranslator);
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
    saveSettings();

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
bool MainWindow::eventFilter(QObject * obj, QEvent * e)
{
    switch (e->type())
    {
        case QEvent::Close:
            _windowActionList->removeWindow(qobject_cast<QMdiSubWindow*>(obj));
        break;
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
    ui->menuConfig->setEnabled(frm != nullptr);

    ui->actionSave->setEnabled(frm != nullptr);
    ui->actionSaveAs->setEnabled(frm != nullptr);
    ui->actionPrintSetup->setEnabled(_selectedPrinter != nullptr);
    ui->actionPrint->setEnabled(_selectedPrinter != nullptr && frm != nullptr);
    ui->actionRecentFile->setEnabled(!_recentFileActionList->isEmpty());

    ui->actionDataDefinition->setEnabled(frm != nullptr);
    ui->actionShowData->setEnabled(frm != nullptr);
    ui->actionShowTraffic->setEnabled(frm != nullptr);
    ui->actionShowScript->setEnabled(frm != nullptr);
    ui->actionBinary->setEnabled(frm != nullptr);
    ui->actionUnsignedDecimal->setEnabled(frm != nullptr);
    ui->actionInteger->setEnabled(frm != nullptr);
    ui->actionHex->setEnabled(frm != nullptr);
    ui->actionFloatingPt->setEnabled(frm != nullptr);
    ui->actionSwappedFP->setEnabled(frm != nullptr);
    ui->actionDblFloat->setEnabled(frm != nullptr);
    ui->actionSwappedDbl->setEnabled(frm != nullptr);

    ui->actionRunScript->setEnabled(frm != nullptr);
    ui->actionStopScript->setEnabled(frm != nullptr);
    ui->actionScriptSettings->setEnabled(frm != nullptr);

    const auto isConnected = _mbMultiServer.isConnected();
    ui->actionForceCoils->setEnabled(isConnected);
    ui->actionForceDiscretes->setEnabled(isConnected);
    ui->actionPresetInputRegs->setEnabled(isConnected);
    ui->actionPresetHoldingRegs->setEnabled(isConnected);

    ui->actionToolbar->setChecked(ui->toolBarMain->isVisible());
    ui->actionStatusBar->setChecked(statusBar()->isVisible());
    ui->actionDisplayBar->setChecked(ui->toolBarDisplay->isVisible());
    ui->actionEnglish->setChecked(_lang == "en");
    ui->actionRussian->setChecked(_lang == "ru");

    if(frm != nullptr)
    {
        const auto ddm = frm->dataDisplayMode();
        ui->actionBinary->setChecked(ddm == DataDisplayMode::Binary);
        ui->actionUnsignedDecimal->setChecked(ddm == DataDisplayMode::Decimal);
        ui->actionInteger->setChecked(ddm == DataDisplayMode::Integer);
        ui->actionHex->setChecked(ddm == DataDisplayMode::Hex);
        ui->actionFloatingPt->setChecked(ddm == DataDisplayMode::FloatingPt);
        ui->actionSwappedFP->setChecked(ddm == DataDisplayMode::SwappedFP);
        ui->actionDblFloat->setChecked(ddm == DataDisplayMode::DblFloat);
        ui->actionSwappedDbl->setChecked(ddm == DataDisplayMode::SwappedDbl);

        const auto byteOrder = frm->byteOrder();
        ui->actionLittleEndian->setChecked(byteOrder == ByteOrder::LittleEndian);
        ui->actionBigEndian->setChecked(byteOrder == ByteOrder::BigEndian);

        ui->actionHexAddresses->setChecked(frm->displayHexAddresses());

        const auto dm = frm->displayMode();
        ui->actionShowData->setChecked(dm == DisplayMode::Data);
        ui->actionShowTraffic->setChecked(dm == DisplayMode::Traffic);
        ui->actionShowScript->setChecked(dm == DisplayMode::Script);
        ui->actionPrint->setEnabled(_selectedPrinter != nullptr && dm == DisplayMode::Data);

        ui->actionRunScript->setEnabled(frm->canRunScript());
        ui->actionStopScript->setEnabled(frm->canStopScript());
    }
}

///
/// \brief MainWindow::on_actionNew_triggered
///
void MainWindow::on_actionNew_triggered()
{
    const auto cur = currentMdiChild();
    auto frm = createMdiChild(++_windowCounter);

    if(cur) {
        frm->setByteOrder(cur->byteOrder());
        frm->setDataDisplayMode(cur->dataDisplayMode());

        frm->setFont(cur->font());
        frm->setStatusColor(cur->statusColor());
        frm->setBackgroundColor(cur->backgroundColor());
        frm->setForegroundColor(cur->foregroundColor());
    }

    frm->show();
}

///
/// \brief MainWindow::on_actionOpen_triggered
///
void MainWindow::on_actionOpen_triggered()
{
    const auto filename = QFileDialog::getOpenFileName(this, QString(), QString(), tr("All files (*)"));
    if(filename.isEmpty()) return;

    openFile(filename);
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
/// \brief MainWindow::on_actionSave_triggered
///
void MainWindow::on_actionSave_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    if(frm->filename().isEmpty())
        ui->actionSaveAs->trigger();
    else
        saveMdiChild(frm);
}

///
/// \brief MainWindow::on_actionSaveAs_triggered
///
void MainWindow::on_actionSaveAs_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    const auto filename = QFileDialog::getSaveFileName(this, QString(), frm->windowTitle(), tr("All files (*)"));
    if(filename.isEmpty()) return;

    frm->setFilename(filename);

    saveMdiChild(frm);
}

///
/// \brief MainWindow::on_actionSaveTestConfig_triggered
///
void MainWindow::on_actionSaveTestConfig_triggered()
{
    const auto filename = QFileDialog::getSaveFileName(this, QString(), QString(), tr("All files (*)"));
    if(filename.isEmpty()) return;

    saveConfig(filename);
}

///
/// \brief MainWindow::on_actionRestoreTestConfig_triggered
///
void MainWindow::on_actionRestoreTestConfig_triggered()
{
    const auto filename = QFileDialog::getOpenFileName(this, QString(), QString(), tr("All files (*)"));
    if(filename.isEmpty()) return;

    loadConfig(filename);
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
            DialogSelectServicePort dlg(cd.TcpParams.ServicePort, this);
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
/// \brief MainWindow::on_actionDataDefinition_triggered
///
void MainWindow::on_actionDataDefinition_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    DialogDisplayDefinition dlg(frm);
    dlg.exec();
}

///
/// \brief MainWindow::on_actionShowData_triggered
///
void MainWindow::on_actionShowData_triggered()
{
    auto frm = currentMdiChild();
    if(frm) frm->setDisplayMode(DisplayMode::Data);
}

///
/// \brief MainWindow::on_actionShowTraffic_triggered
///
void MainWindow::on_actionShowTraffic_triggered()
{
    auto frm = currentMdiChild();
    if(frm) frm->setDisplayMode(DisplayMode::Traffic);
}

///
/// \brief MainWindow::on_actionShowScript_triggered
///
void MainWindow::on_actionShowScript_triggered()
{
    auto frm = currentMdiChild();
    if(frm) frm->setDisplayMode(DisplayMode::Script);
}

///
/// \brief MainWindow::on_actionBinary_triggered
///
void MainWindow::on_actionBinary_triggered()
{
    updateDataDisplayMode(DataDisplayMode::Binary);
}

///
/// \brief MainWindow::on_actionUnsignedDecimal_triggered
///
void MainWindow::on_actionUnsignedDecimal_triggered()
{
    updateDataDisplayMode(DataDisplayMode::Decimal);
}

///
/// \brief MainWindow::on_actionInteger_triggered
///
void MainWindow::on_actionInteger_triggered()
{
    updateDataDisplayMode(DataDisplayMode::Integer);
}

///
/// \brief MainWindow::on_actionHex_triggered
///
void MainWindow::on_actionHex_triggered()
{
    updateDataDisplayMode(DataDisplayMode::Hex);
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
/// \brief MainWindow::on_actionLittleEndian_triggered
///
void MainWindow::on_actionLittleEndian_triggered()
{
    auto frm = currentMdiChild();
    if(frm) frm->setByteOrder(ByteOrder::LittleEndian);
}

///
/// \brief MainWindow::on_actionBigEndian_triggered
///
void MainWindow::on_actionBigEndian_triggered()
{
    auto frm = currentMdiChild();
    if(frm) frm->setByteOrder(ByteOrder::BigEndian);
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
    ui->statusbar->setVisible(!ui->statusbar->isVisible());
}

///
/// \brief MainWindow::on_actionDisplayBar_triggered
///
void MainWindow::on_actionDisplayBar_triggered()
{
    ui->toolBarDisplay->setVisible(!ui->toolBarDisplay->isVisible());
}

///
/// \brief MainWindow::on_actionBackground_triggered
///
void MainWindow::on_actionBackground_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    QColorDialog dlg(frm->backgroundColor(), this);
    if(dlg.exec() == QDialog::Accepted)
    {
        frm->setBackgroundColor(dlg.currentColor());
    }
}

///
/// \brief MainWindow::on_actionForeground_triggered
///
void MainWindow::on_actionForeground_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    QColorDialog dlg(frm->foregroundColor(), this);
    if(dlg.exec() == QDialog::Accepted)
    {
        frm->setForegroundColor(dlg.currentColor());
    }
}

///
/// \brief MainWindow::on_actionStatus_triggered
///
void MainWindow::on_actionStatus_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    QColorDialog dlg(frm->statusColor(), this);
    if(dlg.exec() == QDialog::Accepted)
    {
        frm->setStatusColor(dlg.currentColor());
    }
}

///
/// \brief MainWindow::on_actionFont_triggered
///
void MainWindow::on_actionFont_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    QFontDialog dlg(frm->font(), this);
    if(dlg.exec() == QDialog::Accepted)
    {
        frm->setFont(dlg.currentFont());
    }
}

///
/// \brief MainWindow::on_actionEnglish_triggered
///
void MainWindow::on_actionEnglish_triggered()
{
    setLanguage("en");
}

///
/// \brief MainWindow::on_actionRussian_triggered
///
void MainWindow::on_actionRussian_triggered()
{
   setLanguage("ru");
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
    DialogWindowsManager dlg(_windowActionList->actionList(), ui->actionSave, this);
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
/// \brief MainWindow::on_actionRunScript_triggered
///
void MainWindow::on_actionRunScript_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    frm->runScript();
}

///
/// \brief MainWindow::on_actionStopScript_triggered
///
void MainWindow::on_actionStopScript_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    frm->stopScript();
}

///
/// \brief MainWindow::on_actionScriptSettings_triggered
///
void MainWindow::on_actionScriptSettings_triggered()
{
    auto frm = currentMdiChild();
    if(!frm) return;

    auto ss = frm->scriptSettings();
    DialogScriptSettings dlg(ss, this);

    if(dlg.exec() == QDialog::Accepted)
        frm->setScriptSettings(ss);
}

///
/// \brief MainWindow::on_runModeChanged
/// \param mode
///
void MainWindow::on_runModeChanged(RunMode mode)
{
    auto frm = currentMdiChild();
    if(!frm) return;

    auto ss = frm->scriptSettings();

    ss.Mode = mode;
    frm->setScriptSettings(ss);
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
/// \brief MainWindow::on_dataSimulated
/// \param mode
/// \param type
/// \param addr
/// \param value
///
void MainWindow::on_dataSimulated(DataDisplayMode mode, QModbusDataUnit::RegisterType type, quint16 addr, QVariant value)
{
    auto frm = currentMdiChild();
    const auto order = (frm)? frm->byteOrder() : ByteOrder::LittleEndian;

    _mbMultiServer.writeRegister(type, { addr, value, mode, order });
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
/// \brief MainWindow::openFile
/// \param filename
///
void MainWindow::openFile(const QString& filename)
{
    auto frm = loadMdiChild(filename);
    if(frm)
    {
        frm->show();
    }
    else
    {
        QString message = !QFileInfo::exists(filename) ?
                    QString("%1 was not found").arg(filename) :
                    QString("Failed to open %1").arg(filename);

        _recentFileActionList->removeRecentFile(filename);
        QMessageBox::warning(this, windowTitle(), message);
    }
}

///
/// \brief MainWindow::addRecentFile
/// \param filename
///
void MainWindow::addRecentFile(const QString& filename)
{
    _recentFileActionList->addRecentFile(filename);
}

///
/// \brief MainWindow::updateDisplayMode
/// \param mode
///
void MainWindow::updateDataDisplayMode(DataDisplayMode mode)
{
    auto frm = currentMdiChild();
    if(frm) frm->setDataDisplayMode(mode);
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
    SetupPresetParams presetParams = { dd.PointAddress, dd.Length };

    {
        DialogSetupPresetData dlg(presetParams, type, this);
        if(dlg.exec() != QDialog::Accepted) return;
    }

    ModbusWriteParams params;
    params.Address = presetParams.PointAddress;

    if(dd.PointType == type)
    {
        const auto data = _mbMultiServer.data(type, presetParams.PointAddress - 1, presetParams.Length);
        params.Value = QVariant::fromValue(data.values());
    }

    DialogForceMultipleCoils dlg(params, presetParams.Length, this);
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
    SetupPresetParams presetParams = { dd.PointAddress, dd.Length };

    {
        DialogSetupPresetData dlg(presetParams, type, this);
        if(dlg.exec() != QDialog::Accepted) return;
    }

    ModbusWriteParams params;
    params.Address = presetParams.PointAddress;
    params.DisplayMode = frm->dataDisplayMode();
    params.Order = frm->byteOrder();

    if(dd.PointType == type)
    {
        const auto data = _mbMultiServer.data(type, presetParams.PointAddress - 1, presetParams.Length);
        params.Value = QVariant::fromValue(data.values());
    }

    DialogForceMultipleRegisters dlg(params, presetParams.Length, this);
    if(dlg.exec() == QDialog::Accepted)
    {
        _mbMultiServer.writeRegister(type, params);
    }
}

///
/// \brief MainWindow::createMdiChild
/// \param id
/// \return
///
FormModSim* MainWindow::createMdiChild(int id)
{
    auto frm = new FormModSim(id, _mbMultiServer, _dataSimulator, this);
    auto wnd = ui->mdiArea->addSubWindow(frm);
    wnd->installEventFilter(this);
    wnd->setAttribute(Qt::WA_DeleteOnClose, true);

    auto updateIcons = [this](ByteOrder order)
    {
        switch(order){
        case ByteOrder::BigEndian: ui->actionByteOrder->setIcon(_icoBigEndian); break;
        case ByteOrder::LittleEndian: ui->actionByteOrder->setIcon(_icoLittleEndian); break;
        }
    };

    auto updateRunMode = [this](RunMode mode)
    {
        auto comboBox = qobject_cast<RunModeComboBox*>(ui->toolBarScript->widgetForAction(_actionRunMode));
        comboBox->setCurrentRunMode(mode);
    };

    connect(wnd, &QMdiSubWindow::windowStateChanged, this, [frm, updateIcons, updateRunMode](Qt::WindowStates, Qt::WindowStates newState)
    {
        if(newState == Qt::WindowActive)
        {
            updateIcons(frm->byteOrder());
            updateRunMode(frm->scriptSettings().Mode);
        }
    });

    connect(frm, &FormModSim::byteOrderChanged, this, [updateIcons](ByteOrder order)
    {
        updateIcons(order);
    });

    connect(frm, &FormModSim::scriptSettingsChanged, this, [updateRunMode](const ScriptSettings& ss)
    {
        updateRunMode(ss.Mode);
    });

    connect(frm, &FormModSim::showed, this, [this, wnd]
    {
        windowActivate(wnd);
    });

    _windowActionList->addWindow(wnd);

    return frm;
}

///
/// \brief MainWindow::currentMdiChild
/// \return
///
FormModSim* MainWindow::currentMdiChild() const
{
    const auto wnd = ui->mdiArea->currentSubWindow();
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
/// \brief MainWindow::loadConfig
/// \param filename
///
void MainWindow::loadConfig(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QFile::ReadOnly))
        return;

    QDataStream s(&file);
    s.setByteOrder(QDataStream::BigEndian);
    s.setVersion(QDataStream::Version::Qt_5_0);

    quint8 magic = 0;
    s >> magic;

    if(magic != 0x35)
        return;

    QVersionNumber ver;
    s >> ver;

    if(ver != QVersionNumber(1, 0))
        return;

    QStringList listFilename;
    s >> listFilename;

    QList<ConnectionDetails> conns;
    s >> conns;

    if(s.status() != QDataStream::Ok)
        return;

    ui->mdiArea->closeAllSubWindows();
    for(auto&& filename: listFilename)
    {
        if(!filename.isEmpty())
            openFile(filename);
    }

    auto menu = qobject_cast<MenuConnect*>(ui->actionConnect->menu());
    menu->updateConnectionDetails(conns);

    for(auto&& cd : conns)
    {
        if(menu->canConnect(cd))
            _mbMultiServer.connectDevice(cd);
    }
}

///
/// \brief MainWindow::saveConfig
/// \param filename
///
void MainWindow::saveConfig(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QFile::WriteOnly))
        return;

    QStringList listFilename;
    const auto activeWnd = ui->mdiArea->currentSubWindow();
    for(auto&& wnd : ui->mdiArea->subWindowList())
    {
        windowActivate(wnd);
        ui->actionSave->trigger();

        const auto frm = qobject_cast<FormModSim*>(wnd->widget());
        const auto filename = frm->filename();
        if(!filename.isEmpty()) listFilename.push_back(filename);
    }
    windowActivate(activeWnd);

    QDataStream s(&file);
    s.setByteOrder(QDataStream::BigEndian);
    s.setVersion(QDataStream::Version::Qt_5_0);

    // magic number
    s << (quint8)0x35;

    // version number
    s << QVersionNumber(1, 0);

    // list of files
    s << listFilename;

    // connections
    s << _mbMultiServer.connections();
}

///
/// \brief MainWindow::loadMdiChild
/// \param filename
/// \return
///
FormModSim* MainWindow::loadMdiChild(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QFile::ReadOnly))
        return nullptr;

    QDataStream s(&file);
    s.setByteOrder(QDataStream::BigEndian);
    s.setVersion(QDataStream::Version::Qt_5_0);

    quint8 magic = 0;
    s >> magic;

    if(magic != 0x34)
        return nullptr;

    QVersionNumber ver;
    s >> ver;

    if(ver > FormModSim::VERSION)
        return nullptr;

    int formId;
    s >> formId;

    if(s.status() != QDataStream::Ok)
        return nullptr;

    bool created = false;
    auto frm = findMdiChild(formId);
    if(!frm)
    {
        created = true;
        frm = createMdiChild(formId);
    }

    if(!frm)
        return nullptr;

    frm->setProperty("Version", QVariant::fromValue(ver));
    s >> frm;

    if(s.status() != QDataStream::Ok)
    {
        if(created) frm->close();
        return nullptr;
    }

    frm->setFilename(filename);
    addRecentFile(filename);
    _windowCounter = qMax(frm->formId(), _windowCounter);

    return frm;
}

///
/// \brief MainWindow::saveMdiChild
/// \param frm
///
void MainWindow::saveMdiChild(FormModSim* frm)
{
    if(!frm) return;

    QFile file(frm->filename());
    if(!file.open(QFile::WriteOnly))
        return;

    QDataStream s(&file);
    s.setByteOrder(QDataStream::BigEndian);
    s.setVersion(QDataStream::Version::Qt_5_0);

    // magic number
    s << (quint8)0x34;

    // version number
    s << FormModSim::VERSION;

    // form
    s << frm;

    addRecentFile(frm->filename());
}

///
/// \brief MainWindow::loadSettings
///
void MainWindow::loadSettings()
{
    const auto filename = QString("%1.ini").arg(QFileInfo(qApp->applicationFilePath()).baseName());
    auto filepath = QString("%1%2%3").arg(qApp->applicationDirPath(), QDir::separator(), filename);

    if(!QFile::exists(filepath))
        filepath = QString("%1%2%3").arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
                                         QDir::separator(), filename);

    if(!QFile::exists(filepath)) return;

    QSettings m(filepath, QSettings::IniFormat, this);

    const auto displaybarArea = (Qt::ToolBarArea)qBound(0, m.value("DisplayBarArea", 0x4).toInt(), 0xf);
    const auto displaybarBreak = m.value("DisplayBarBreak").toBool();
    if(displaybarBreak) addToolBarBreak(displaybarArea);
    addToolBar(displaybarArea, ui->toolBarDisplay);

    const auto scriptbarArea = (Qt::ToolBarArea)qBound(0, m.value("ScriptBarArea", 0x4).toInt(), 0xf);
    const auto scriptbarBreak = m.value("ScriptBarBreak").toBool();
    if(scriptbarBreak) addToolBarBreak(scriptbarArea);
    addToolBar(scriptbarArea, ui->toolBarScript);

    _lang = m.value("Language", "en").toString();
    setLanguage(_lang);

    m >> firstMdiChild();
}

///
/// \brief checkPathIsWritable
/// \param path
/// \return
///
bool checkPathIsWritable(const QString& path)
{
    const auto filepath = QString("%1%2%3").arg(path, QDir::separator(), ".test");
    if(!QFile(filepath).open(QFile::WriteOnly)) return false;

    QFile::remove(filepath);
    return true;
}

///
/// \brief MainWindow::saveSettings
///
void MainWindow::saveSettings()
{
    const auto filename = QString("%1.ini").arg(QFileInfo(qApp->applicationFilePath()).baseName());
    auto filepath = QString("%1%2%3").arg(qApp->applicationDirPath(), QDir::separator(), filename);

    if(!QFileInfo(qApp->applicationDirPath()).isWritable() || !checkPathIsWritable(qApp->applicationDirPath()))
        filepath = QString("%1%2%3").arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
                                         QDir::separator(), filename);

    QSettings m(filepath, QSettings::IniFormat, this);

    m.setValue("DisplayBarArea", toolBarArea(ui->toolBarDisplay));
    m.setValue("DisplayBarBreak", toolBarBreak(ui->toolBarDisplay));
    m.setValue("ScriptBarArea", toolBarArea(ui->toolBarScript));
    m.setValue("ScriptBarBreak", toolBarBreak(ui->toolBarScript));
    m.setValue("Language", _lang);

    m << firstMdiChild();
}
