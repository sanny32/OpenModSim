// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appproject.cpp
/// \brief Implements the appproject functionality.
///

#include <QtWidgets>

#include "apppreferences.h"
#include "appproject.h"
#include "projectformxml.h"
#include "projectformmetadata.h"
#include "projectformmanager.h"
#include "projectserializer.h"
#include "projectsplitcontroller.h"
#include "mainwindow.h"
#include "controls/mdiareaex.h"
#include "controls/projecttreewidget.h"
#include "themedicons.h"
#include "formdataview.h"
#include "apptrace.h"

namespace {
///
/// \brief dataMapIcon
/// \param deleteLocked
/// \return
///
QIcon dataMapIcon(bool deleteLocked)
{
    return deleteLocked ? themedIcon(QStringLiteral("omodsim/data-locked"))
                        : themedIcon(QStringLiteral("omodsim/show-data-map"));
}

///
/// \brief panelName
/// \param mdi
/// \param area
/// \return
///
QString panelName(const MdiAreaEx* mdi, const MdiArea* area)
{
    if (!mdi || !area)
        return QStringLiteral("null");
    if (area == mdi->primaryArea())
        return QStringLiteral("primary");
    if (area == mdi->secondaryArea())
        return QStringLiteral("secondary");
    return QStringLiteral("unknown");
}

///
/// \brief mdiExState
/// \param mdi
/// \return
///
QString mdiExState(const MdiAreaEx* mdi)
{
    if (!mdi)
        return QStringLiteral("mdiEx=null");

    return QStringLiteral("%1 split=%2 activePanel=%3 primary={%4} secondary={%5}")
        .arg(AppTrace::objectTag(mdi))
        .arg(mdi->isSplitView())
        .arg(panelName(mdi, mdi->activePanel()))
        .arg(AppTrace::mdiAreaState(mdi->primaryArea()))
        .arg(AppTrace::mdiAreaState(mdi->secondaryArea()));
}

}

///
/// \brief AppProject::AppProject
///
AppProject::AppProject(MdiAreaEx* mdiArea,
                       ModbusMultiServer& mbServer,
                       DataSimulator* dataSimulator,
                       ProjectTreeWidget* projectTree,
                       MainWindow* mainWindow,
                       QObject* parent)
    : QObject(parent)
    , _mdiArea(mdiArea)
    , _mbServer(mbServer)
    , _dataSimulator(dataSimulator)
    , _projectTree(projectTree)
    , _mainWindow(mainWindow)
    , _formManager(new ProjectFormManager(mdiArea, mbServer, dataSimulator,
                                          projectTree, mainWindow, this))
    , _splitController(new ProjectSplitController(mdiArea, _formManager, this))
{
    Q_ASSERT(_dataSimulator != nullptr);

    AppTrace::log("AppProject::AppProject",
                  QStringLiteral("constructed state=%1").arg(mdiExState(_mdiArea)));
    connect(_mdiArea, &MdiAreaEx::subWindowActivated, this, [this](QMdiSubWindow* wnd) {
        AppTrace::log("AppProject::onMdiSubWindowActivated",
                      QStringLiteral("wnd=%1 state=%2")
                          .arg(AppTrace::subWindowTag(wnd))
                          .arg(mdiExState(_mdiArea)));
    });
    connect(&_mbServer, &ModbusMultiServer::definitionsChanged,
            this, &AppProject::syncAutoRequestMap);
    connect(_formManager, &ProjectFormManager::modified, this, &AppProject::modified);
    connect(_formManager, &ProjectFormManager::formOpened, this, &AppProject::formOpened);
    connect(_formManager, &ProjectFormManager::formClosed, this, &AppProject::formClosed);
    connect(_formManager, &ProjectFormManager::formDeleted, this, &AppProject::formDeleted);
    connect(_formManager, &ProjectFormManager::helpStateUpdateRequested,
            this, &AppProject::helpStateUpdateRequested);
    connect(_formManager, &ProjectFormManager::helpRequested, this, &AppProject::helpRequested);
    connect(_formManager, &ProjectFormManager::consoleMessage, this, &AppProject::consoleMessage);
    connect(_formManager, &ProjectFormManager::outputConsoleRequested,
            this, &AppProject::outputConsoleRequested);
    connect(_formManager, &ProjectFormManager::formActivationRequested,
            this, &AppProject::formActivationRequested);
    connect(_formManager, &ProjectFormManager::splitFormStateChanged,
            this, &AppProject::updateSplitPairScriptIcons);
    connect(_formManager, &ProjectFormManager::splitMayBeEmpty,
            this, &AppProject::resetSplitViewIfEmpty);
}

///
/// \brief AppProject::~AppProject
///
AppProject::~AppProject()
{
    AppTrace::log("AppProject::~AppProject",
                  QStringLiteral("destroyed state=%1").arg(mdiExState(_mdiArea)));
    if (_mdiArea)
        _formManager->clear();
}

///
/// \brief AppProject::allProjectForms
///
QList<QWidget*> AppProject::allProjectForms() const
{
    return _formManager->forms();
}

QList<QWidget*> AppProject::forms(ProjectFormKind kind) const
{
    return _formManager->forms(kind);
}

QList<FormScriptView*> AppProject::scriptForms() const
{
    return _formManager->scriptForms();
}

///
/// \brief AppProject::findAutoRequestMap
///
FormDataMapView* AppProject::findAutoRequestMap() const
{
    for (auto* widget : allProjectForms()) {
        auto* map = qobject_cast<FormDataMapView*>(widget);
        if (map && map->isAutoRequestMap())
            return map;
    }

    return nullptr;
}

///
/// \brief AppProject::ensureAutoRequestMap
///
FormDataMapView* AppProject::ensureAutoRequestMap()
{
    if (auto* map = findAutoRequestMap()) {
        map->setAutoRequestMap(true);
        if (containsClosedForm(map))
            rewrapMdiChild(map);
        return map;
    }

    auto* widget = createMdiChildOnArea(ProjectFormKind::DataMap,
                                        _mdiArea->primaryArea() ? _mdiArea->primaryArea() : activeCreateArea(),
                                        true);
    auto* map = qobject_cast<FormDataMapView*>(widget);
    if (!map)
        return nullptr;

    map->setAutoRequestMap(true);
    map->setWindowTitle(QStringLiteral("AutoMap"));
    return map;
}

///
/// \brief AppProject::syncAutoRequestMap
///
void AppProject::syncAutoRequestMap(const ModbusDefinitions& defs)
{
    if (defs.AutoAddRegistersOnRequest) {
        auto* existingMap = findAutoRequestMap();
        const bool wasClosed = existingMap && containsClosedForm(existingMap);
        const bool shouldRevealMap = !existingMap || wasClosed;

        if (auto* map = ensureAutoRequestMap()) {
            map->setAutoAddOnRequest(true);
            map->setProperty(ProjectFormMetadata::DeleteLocked, true);
            map->setWindowIcon(dataMapIcon(true));
            _projectTree->updateFormTitle(map);
            if (shouldRevealMap) {
                openFormOnActivePanel(map);
                _projectTree->activateForm(map);
            }
        }
        return;
    }

    if (auto* map = findAutoRequestMap()) {
        map->setAutoAddOnRequest(false);
        map->setProperty(ProjectFormMetadata::DeleteLocked, false);
        map->setWindowIcon(dataMapIcon(false));
        _projectTree->updateFormTitle(map);
    }
}

///
/// \brief AppProject::containsClosedForm
///
bool AppProject::containsClosedForm(QWidget* frm) const
{
    return _formManager->isClosed(frm);
}

///
/// \brief AppProject::isFormClosed
///
bool AppProject::isFormClosed(QWidget* frm) const
{
    return _formManager->isClosed(frm);
}

///
/// \brief Returns the canonical forms currently parked outside the MDI area.
/// \return Closed project forms.
///
const QList<QWidget*>& AppProject::closedForms() const
{
    return _formManager->closedForms();
}

///
/// \brief AppProject::closeProject
/// Closes all open and hidden forms, resetting the workspace.
///
void AppProject::closeProject()
{
    _dataSimulator->stopSimulations();
    _mbServer.closeConnections();
    _formManager->clear();
    _mbServer.clearAddressSpace();
    _mbServer.clearDescriptions();
    _mbServer.clearTimestamps();
    _mbServer.clearConfiguredValues();
    _projectComments.clear();

    if (!_projectFilename.isEmpty()) {
        emit projectClosed(_projectFilename);
        _projectFilename.clear();
    }
}

///
/// \brief AppProject::markFormClosed
/// Called when a primary form's subwindow is being closed - reparents the form so it survives.
///
void AppProject::markFormClosed(QWidget* frm)
{
    _formManager->park(frm);
}

///
/// \brief AppProject::nextFormDisplayNumber
/// Returns the next sequential display number for the given form kind.
///
int AppProject::nextFormDisplayNumber(ProjectFormKind kind)
{
    return _formManager->nextDisplayNumber(kind);
}

///
/// \brief AppProject::createMdiChild
/// \return
///
QWidget* AppProject::createMdiChild(ProjectFormKind kind)
{
    auto* frm = createMdiChildOnArea(kind, activeCreateArea(), true);
    if (frm)
        emit formCreated(frm);
    return frm;
}

///
/// \brief AppProject::createMdiChildOnArea
/// \param kind
/// \param area
/// \param addToWindowList
/// \return
///
QWidget* AppProject::createMdiChildOnArea(ProjectFormKind kind, MdiArea* area, bool addToWindowList)
{
    return _formManager->create(kind, area, addToWindowList);
}

///
/// \brief AppProject::rewrapMdiChild
/// Re-opens a previously "closed" (hidden) QWidget by creating a new MDI subwindow for it.
///
void AppProject::rewrapMdiChild(QWidget* frm)
{
    _formManager->reopen(frm, activeCreateArea());
}

///
/// \brief AppProject::closeMdiChild
/// \param frm
///
void AppProject::closeMdiChild(QWidget* frm)
{
    _formManager->close(frm);
}

///
/// \brief AppProject::deleteForm
/// Deletes a form permanently from the project (closes its tab if open).
///
void AppProject::deleteForm(QWidget* frm)
{
    _formManager->remove(frm);
}

///
/// \brief AppProject::currentMdiChild
/// \return
///
QWidget* AppProject::currentMdiChild() const
{
    return _formManager->currentForm();
}

///
/// \brief AppProject::currentDataMdiChild
///
FormDataView* AppProject::currentDataMdiChild() const
{
    return qobject_cast<FormDataView*>(currentMdiChild());
}

///
/// \brief AppProject::currentTrafficMdiChild
///
FormTrafficView* AppProject::currentTrafficMdiChild() const
{
    return qobject_cast<FormTrafficView*>(currentMdiChild());
}

///
/// \brief AppProject::currentScriptMdiChild
///
FormScriptView* AppProject::currentScriptMdiChild() const
{
    return qobject_cast<FormScriptView*>(currentMdiChild());
}

///
/// \brief AppProject::currentDataMapMdiChild
///
FormDataMapView* AppProject::currentDataMapMdiChild() const
{
    return qobject_cast<FormDataMapView*>(currentMdiChild());
}

///
/// \brief AppProject::findMdiChild
/// \param id
/// \return
///
QWidget* AppProject::findMdiChild(QUuid id) const
{
    return _formManager->find(id);
}

///
/// \brief AppProject::findMdiChildInArea
/// \param area
/// \param id
/// \return
///
QWidget* AppProject::findMdiChildInArea(MdiArea* area, QUuid id) const
{
    return _formManager->findInArea(area, id);
}

///
/// \brief AppProject::resolveFormForActiveArea
/// \param primaryForm
/// \return The clone of primaryForm in the secondary area if the secondary area is active,
///         otherwise primaryForm itself.
///
QWidget* AppProject::resolveFormForActiveArea(QWidget* primaryForm) const
{
    return _splitController->resolveForActiveArea(primaryForm);
}

///
/// \brief AppProject::createCloneOnArea
/// Creates a visual clone of \a source on \a area. Returns the clone widget, or nullptr on failure.
///
QWidget* AppProject::createCloneOnArea(QWidget* source, MdiArea* area)
{
    return _splitController->createClone(source, area);
}

///
/// \brief AppProject::openFormOnActivePanel
/// Activates \a frm (or its split clone) on the active panel.
/// If the form is not present there, opens it on that panel.
///
void AppProject::openFormOnActivePanel(QWidget* frm)
{
    _splitController->openOnActivePanel(frm);
}

///
/// \brief AppProject::firstMdiChild
/// \return
///
QWidget* AppProject::firstMdiChild() const
{
    return _formManager->firstForm();
}

///
/// \brief AppProject::cloneMdiChildState
/// \param source
/// \param target
/// \return
///
bool AppProject::cloneMdiChildState(QWidget* source, QWidget* target) const
{
    return _splitController->cloneState(source, target);
}

///
/// \brief AppProject::activeCreateArea
/// \return
///
MdiArea* AppProject::activeCreateArea() const
{
    return _splitController->activeCreateArea();
}

///
/// \brief AppProject::areaOfForm
/// \param frm
/// \return
///
MdiArea* AppProject::areaOfForm(QWidget* frm) const
{
    return _splitController->areaOfForm(frm);
}

///
/// \brief AppProject::secondaryArea
/// \return
///
MdiArea* AppProject::secondaryArea() const
{
    return _splitController->secondaryArea();
}

///
/// \brief AppProject::isSplitTabbedView
/// \return
///
bool AppProject::isSplitTabbedView() const
{
    return _splitController->isSplitTabbedView();
}

///
/// \brief AppProject::canMoveFormToOtherPanel
/// Returns true when \a frm can be moved to the opposite panel.
/// Auto-clones are excluded; split view must be active.
///
bool AppProject::canMoveFormToOtherPanel(QWidget* frm) const
{
    return _splitController->canMoveToOtherPanel(frm);
}

///
/// \brief AppProject::moveFormToOtherPanel
/// Moves \a frm from its current panel to the opposite panel.
/// If an auto-clone of this form already exists in the target panel it is
/// deleted first, so there is never a duplicate.
///
void AppProject::moveFormToOtherPanel(QWidget* frm, QPoint globalDropPos)
{
    _splitController->moveToOtherPanel(frm, globalDropPos);
}

///
/// \brief AppProject::resetSplitViewIfEmpty
///
void AppProject::resetSplitViewIfEmpty()
{
    _splitController->resetIfEmpty();
}

///
/// \brief AppProject::isScriptRunningOnSplitPair
/// \param frm
/// \return
///
bool AppProject::isScriptRunningOnSplitPair(QWidget* frm) const
{
    return _splitController->isScriptPairRunning(frm);
}

///
/// \brief AppProject::updateSplitPairScriptIcons
/// \param frm
///
void AppProject::updateSplitPairScriptIcons(QWidget* frm)
{
    _splitController->updateScriptPairIcons(frm);
}

///
/// \brief AppProject::duplicatePrimaryTabsToSecondary
/// Opens a clone of the currently active primary tab on the secondary panel.
/// Reuses an existing auto-clone on primary (leftover from a previous split) if available.
/// \return 1 if a window was placed on secondary, 0 otherwise
///
int AppProject::duplicatePrimaryTabsToSecondary()
{
    return _splitController->duplicatePrimaryTab();
}

///
/// \brief AppProject::removeSplitAutoClonesFromSecondary
/// Removes transient split clones from secondary panel before split merge.
///
void AppProject::removeSplitAutoClonesFromSecondary()
{
    _splitController->removeSecondaryClones();
}

///
/// \brief Loads or merges a validated project document.
/// \param filename Project file path.
/// \return Load status and error text.
///
ProjectLoadResult AppProject::loadProject(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QFile::ReadOnly)) {
        const ProjectLoadResult status{false, file.errorString()};
        emit projectLoadFailed(QFileInfo(filename).absoluteFilePath(), status.Error);
        return status;
    }

    const auto replace = _projectFilename.isEmpty();
    ProjectSerializer serializer(*this, *_formManager, *_splitController,
                                 _mbServer, _dataSimulator, _mdiArea, _mainWindow);
    const auto result = serializer.load(file, replace);
    if (!result.Status.Success) {
        emit projectLoadFailed(QFileInfo(filename).absoluteFilePath(), result.Status.Error);
        return result.Status;
    }

    if (replace) {
        _projectComments = result.Comments;
        setSavePath(QFileInfo(filename).absoluteDir().absolutePath());
        _projectFilename = QFileInfo(filename).absoluteFilePath();
        emit projectOpened(_projectFilename);
    }

    if (!replace)
        return result.Status;

    _mainWindow->applyConnections(result.Definitions, result.Connections);
    syncAutoRequestMap(_mbServer.getModbusDefinitions());

    auto& prefs = AppPreferences::instance();
    if (result.HasGlobalZeroBasedAddress)
        prefs.setGlobalAddressBase(result.GlobalZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    if (result.HasGlobalHexView)
        prefs.setGlobalHexView(result.GlobalHexView);

    if(auto* primary = _mdiArea->primaryArea())
        applyTabOrderToArea(primary, result.PrimaryTabOrder);
    if(result.SplitView)
        if(auto* secondary = secondaryArea())
            applyTabOrderToArea(secondary, result.SecondaryTabOrder);

    _splitController->setPendingActivation(result.ActivePrimaryWindow,
                                           result.SplitView ? result.ActiveSecondaryWindow : QString(),
                                           result.ActivePanel);

    if(_mdiArea->isVisible())
        restoreActiveWindows();

    return result.Status;
}

///
/// \brief Validates a project file without changing application state.
/// \param filename Project file path.
/// \return Validation status and error text.
///
ProjectLoadResult AppProject::validateProject(const QString& filename) const
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly))
        return {false, file.errorString()};
    return ProjectSerializer::validate(file);
}

///
/// \brief AppProject::restoreActiveWindows
/// Restores the active sub-window selection after a project load.
/// Called immediately if the MDI area is already visible, or deferred via
/// MainWindow::showEvent if the window has not been shown yet.
///
void AppProject::restoreActiveWindows()
{
    _splitController->restoreActiveWindows();
}

///
/// \brief AppProject::saveProject
/// \param filename
/// \return
///
bool AppProject::saveProject(const QString& filename)
{
    const QString absoluteFilename = QFileInfo(filename).absoluteFilePath();

    QFile file(filename);
    if(!file.open(QFile::WriteOnly)) {
        emit projectSaveFailed(absoluteFilename, file.errorString());
        return false;
    }

    setSavePath(QFileInfo(filename).absoluteDir().absolutePath());
    _projectFilename = absoluteFilename;

    ProjectSerializer serializer(*this, *_formManager, *_splitController,
                                 _mbServer, _dataSimulator, _mdiArea, _mainWindow);
    if (!serializer.save(file, _projectComments)) {
        emit projectSaveFailed(_projectFilename, QObject::tr("Failed to write project XML."));
        return false;
    }

    emit projectSaved(_projectFilename);
    return true;
}

///
/// \brief AppProject::destroyContentForShutdown
///
void AppProject::destroyContentForShutdown()
{
    closeProject();
}
