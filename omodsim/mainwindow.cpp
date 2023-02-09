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
    ,_windowCounter(0)
{
    ui->setupUi(this);

    setWindowTitle(APP_NAME);
    setUnifiedTitleAndToolBarOnMac(true);
    setStatusBar(new MainStatusBar(_mbMultiServer, this));

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

    _recentFileActionList = new RecentFileActionList(ui->menuFile, ui->actionRecentFile);
    connect(_recentFileActionList, &RecentFileActionList::triggered, this, &MainWindow::openFile);

    _windowActionList = new WindowActionList(ui->menuWindow, ui->actionWindows);
    connect(_windowActionList, &WindowActionList::triggered, this, &MainWindow::windowActivate);

    auto dispatcher = QAbstractEventDispatcher::instance();
    connect(dispatcher, &QAbstractEventDispatcher::awake, this, &MainWindow::on_awake);

    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::updateMenuWindow);
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
    ui->actionBinary->setEnabled(frm != nullptr);
    ui->actionUnsignedDecimal->setEnabled(frm != nullptr);
    ui->actionInteger->setEnabled(frm != nullptr);
    ui->actionHex->setEnabled(frm != nullptr);
    ui->actionFloatingPt->setEnabled(frm != nullptr);
    ui->actionSwappedFP->setEnabled(frm != nullptr);
    ui->actionDblFloat->setEnabled(frm != nullptr);
    ui->actionSwappedDbl->setEnabled(frm != nullptr);
    ui->actionResetCtrs->setEnabled(frm != nullptr);
    ui->actionToolbar->setChecked(ui->toolBarMain->isVisible());
    ui->actionStatusBar->setChecked(statusBar()->isVisible());
    ui->actionDisplayBar->setChecked(ui->toolBarDisplay->isVisible());

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

        ui->actionHexAddresses->setChecked(frm->displayHexAddresses());

        const auto dm = frm->displayMode();
        ui->actionShowData->setChecked(dm == DisplayMode::Data);
        ui->actionShowTraffic->setChecked(dm == DisplayMode::Traffic);
        ui->actionPrint->setEnabled(_selectedPrinter != nullptr && dm == DisplayMode::Data);
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
        frm->setDisplayMode(cur->displayMode());
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
/// \brief MainWindow::createMdiChild
/// \param id
/// \return
///
FormModSim* MainWindow::createMdiChild(int id)
{
    auto frm = new FormModSim(id, _mbMultiServer, this);
    auto wnd = ui->mdiArea->addSubWindow(frm);
    wnd->installEventFilter(this);
    wnd->setAttribute(Qt::WA_DeleteOnClose, true);

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

    if(ver != QVersionNumber(1, 0))
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
    s << QVersionNumber(1, 0);

    // form
    s << frm;

    addRecentFile(frm->filename());
}

///
/// \brief MainWindow::loadSettings
///
void MainWindow::loadSettings()
{
    const auto filepath = QString("%1%2%3.ini").arg(
                qApp->applicationDirPath(), QDir::separator(),
                QFileInfo(qApp->applicationFilePath()).baseName());

    if(!QFile::exists(filepath)) return;

    QSettings m(filepath, QSettings::IniFormat, this);

    const auto toolbarArea = (Qt::ToolBarArea)qBound(0, m.value("DisplayBarArea").toInt(), 0xf);
    const auto toolbarBreal = m.value("DisplayBarBreak").toBool();
    if(toolbarBreal) addToolBarBreak(toolbarArea);
    addToolBar(toolbarArea, ui->toolBarDisplay);

    m >> firstMdiChild();
}

///
/// \brief MainWindow::saveSettings
///
void MainWindow::saveSettings()
{
    const auto frm = firstMdiChild();
    const auto filepath = QString("%1%2%3.ini").arg(
                qApp->applicationDirPath(), QDir::separator(),
                QFileInfo(qApp->applicationFilePath()).baseName());
    QSettings m(filepath, QSettings::IniFormat, this);

    m.setValue("DisplayBarArea", toolBarArea(ui->toolBarDisplay));
    m.setValue("DisplayBarBreak", toolBarBreak(ui->toolBarDisplay));

    m << frm;
}
