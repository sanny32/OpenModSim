#include <QtWidgets>
#include <QBuffer>
#include <QUuid>
#include <QSet>
#include <QTextDocument>
#include "apppreferences.h"
#include "appproject.h"
#include "mainwindow.h"
#include "controls/mdiareaex.h"
#include "controls/mditabbar.h"
#include "controls/projecttreewidget.h"
#include "formdataview.h"
#include "formtrafficview.h"
#include "formscriptview.h"
#include "uiutils.h"
#include "apptrace.h"

namespace {
constexpr const char* kFormPanelProperty = "SplitPanel";
constexpr const char* kFormClosedProperty = "Closed";
constexpr const char* kPanelLeft = "L";
constexpr const char* kPanelRight = "R";
constexpr const char* kSplitOriginIdProperty = "SplitOriginId";
constexpr const char* kSplitAutoCloneProperty = "SplitAutoClone";
constexpr const char* kFormIdProperty = "FormId";
constexpr const char* kDeleteLockedProperty = "DeleteLocked";

QUuid formIdOf(QWidget* widget);

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

QString formTag(const QWidget* frm)
{
    if (!frm)
        return QStringLiteral("null");
    return QStringLiteral("%1 id=%2")
        .arg(AppTrace::widgetTag(frm))
        .arg(formIdOf(const_cast<QWidget*>(frm)).toString(QUuid::WithoutBraces));
}

void traceMdiActivationRequest(const char* scope, const MdiAreaEx* mdi, QMdiSubWindow* wnd, const QString& reason)
{
    if (!AppTrace::isEnabled())
        return;

    AppTrace::log(scope,
                  QStringLiteral("reason=\"%1\" request=%2 state=%3")
                      .arg(reason)
                      .arg(AppTrace::subWindowTag(wnd))
                      .arg(mdiExState(mdi)));
}

ProjectFormType toProjectFormType(ProjectFormKind kind)
{
    switch (kind)
    {
        case ProjectFormKind::Data:        return ProjectFormType::Data;
        case ProjectFormKind::Traffic:     return ProjectFormType::Traffic;
        case ProjectFormKind::Script:      return ProjectFormType::Script;
        case ProjectFormKind::DataMap: return ProjectFormType::DataMap;
    }
    return ProjectFormType::Data;
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
    if (qobject_cast<FormDataMapView*>(widget)) {
        if(ok) *ok = true;
        return ProjectFormKind::DataMap;
    }

    if(ok) *ok = false;
    return ProjectFormKind::Data;
}

QUuid formIdOf(QWidget* widget)
{
    if (!widget)
        return {};

    return widget->property(kFormIdProperty).toUuid();
}

void enableAutoCompleteOnForm(QWidget* widget, bool enable)
{
    if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->enableAutoComplete(enable);
}

QString codepageOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->codepage();
    return QString();
}

void connectEditSlotsOnForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->connectEditSlots();
    else if (auto* frm = qobject_cast<FormTrafficView*>(widget)) frm->connectEditSlots();
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->connectEditSlots();
}

void disconnectEditSlotsOnForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->disconnectEditSlots();
    else if (auto* frm = qobject_cast<FormTrafficView*>(widget)) frm->disconnectEditSlots();
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->disconnectEditSlots();
}

DataType dataTypeOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->dataType();
    return DataType::Hex;
}

void setDataTypeOnForm(QWidget* widget, DataType type)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setDataType(type);
}

ScriptSettings scriptSettingsOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormScriptView*>(widget)) return frm->scriptSettings();
    return ScriptSettings();
}

bool canStopScriptOnForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormScriptView*>(widget)) return frm->canStopScript();
    return false;
}

void saveXmlOfForm(QWidget* widget, QXmlStreamWriter& w)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->saveXml(w);
    else if (auto* frm = qobject_cast<FormTrafficView*>(widget)) frm->saveXml(w);
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->saveXml(w);
    else if (auto* frm = qobject_cast<FormDataMapView*>(widget)) frm->saveXml(w);
}

void loadXmlOfForm(QWidget* widget, QXmlStreamReader& r)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) {
        frm->loadXml(r);
        const AppPreferences& prefs = AppPreferences::instance();
        frm->setFont(prefs.font());
        frm->setZoomPercent(prefs.fontZoom());
        frm->setForegroundColor(prefs.foregroundColor());
        frm->setBackgroundColor(prefs.backgroundColor());
        frm->setStatusColor(prefs.statusColor());
        frm->setAddressColor(prefs.addressColor());
        frm->setCommentColor(prefs.commentColor());
    }
    else if (auto* frm = qobject_cast<FormTrafficView*>(widget)) frm->loadXml(r);
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->loadXml(r);
    else if (auto* frm = qobject_cast<FormDataMapView*>(widget)) frm->loadXml(r);
    else r.skipCurrentElement();
}

QStringList tabTitlesForArea(const MdiArea* area)
{
    QStringList titles;
    if(!area)
        return titles;

    const auto* tabBar = qobject_cast<const MdiTabBar*>(area->tabBar());
    if(!tabBar)
        return titles;

    titles.reserve(tabBar->count());
    for(int i = 0; i < tabBar->count(); ++i) {
        const auto* wnd = tabBar->subWindowAt(i);
        const auto* widget = wnd ? wnd->widget() : nullptr;
        if(widget)
            titles << widget->windowTitle();
    }
    return titles;
}

void applyTabOrderToArea(MdiArea* area, const QStringList& orderedTitles)
{
    if(!area || orderedTitles.isEmpty())
        return;

    auto* tabBar = qobject_cast<MdiTabBar*>(area->tabBar());
    if(!tabBar)
        return;

    for(int targetIndex = 0; targetIndex < orderedTitles.size(); ++targetIndex) {
        const QString& wantedTitle = orderedTitles.at(targetIndex);
        int currentIndex = -1;
        for(int i = 0; i < tabBar->count(); ++i) {
            auto* wnd = tabBar->subWindowAt(i);
            auto* widget = wnd ? wnd->widget() : nullptr;
            if(widget && widget->windowTitle() == wantedTitle) {
                currentIndex = i;
                break;
            }
        }

        if(currentIndex >= 0 && currentIndex != targetIndex)
            tabBar->moveTab(currentIndex, targetIndex);
    }
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
}

///
/// \brief AppProject::~AppProject
///
AppProject::~AppProject()
{
    AppTrace::log("AppProject::~AppProject",
                  QStringLiteral("destroyed state=%1").arg(mdiExState(_mdiArea)));
}

///
/// \brief AppProject::allProjectForms
///
QList<QWidget*> AppProject::allProjectForms() const
{
    QList<QWidget*> result;
    for (auto* wnd : _mdiArea->subWindowList()) {
        auto* widget = wnd ? wnd->widget() : nullptr;
        if (!widget || widget->property(kSplitAutoCloneProperty).toBool())
            continue;
        if (!result.contains(widget))
            result.append(widget);
    }

    for (auto* frm : _closedForms) {
        if (frm && !result.contains(frm))
            result.append(frm);
    }

    return result;
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
        if (auto* map = ensureAutoRequestMap()) {
            map->setAutoAddOnRequest(true);
            map->setProperty(kDeleteLockedProperty, true);
            openFormOnActivePanel(map);
            _projectTree->activateForm(map);
        }
        return;
    }

    if (auto* map = findAutoRequestMap()) {
        map->setAutoAddOnRequest(false);
        map->setProperty(kDeleteLockedProperty, false);
    }
}

///
/// \brief AppProject::addClosedForm
///
void AppProject::addClosedForm(QWidget* frm)
{
    if(frm && !_closedForms.contains(frm))
        _closedForms.append(frm);
}

///
/// \brief AppProject::removeClosedForm
///
void AppProject::removeClosedForm(QWidget* frm)
{
    _closedForms.removeOne(frm);
}

///
/// \brief AppProject::containsClosedForm
///
bool AppProject::containsClosedForm(QWidget* frm) const
{
    return frm && _closedForms.contains(frm);
}

///
/// \brief AppProject::isFormClosed
///
bool AppProject::isFormClosed(QWidget* frm) const
{
    return containsClosedForm(qobject_cast<QWidget*>(frm));
}

///
/// \brief AppProject::closeProject
/// Closes all open and hidden forms, resetting the workspace.
///
void AppProject::closeProject()
{
    // Close open MDI windows (QWidget windows)
    _mdiArea->closeAllSubWindows();

    // Delete forms that were closed (hidden)
    const auto closed = _closedForms;
    for (auto&& frm : closed) {
        _projectTree->removeForm(frm);
        delete frm;
    }
    _closedForms.clear();
    _mbServer.clearDescriptions();
    _mbServer.clearTimestamps();
    _dataCounter        = 0;
    _trafficCounter     = 0;
    _scriptCounter      = 0;
    _DataMapCounter = 0;
}

///
/// \brief AppProject::markFormClosed
/// Called when a primary form's subwindow is being closed - reparents the form so it survives.
///
void AppProject::markFormClosed(QWidget* frm)
{
    // Remove the QMdiSubWindow's event filter from frm before reparenting.
    // QMdiSubWindow installs this filter via setWidget(). After reparenting frm,
    // the subwindow is DeferredDeleted via WA_DeleteOnClose. Without this call,
    // frm's extraData->eventFilters retains a dangling pointer to the deleted
    // QMdiSubWindow. When frm is later explicitly deleted, QEvent::Destroy is sent
    // through sendThroughObjectEventFilters(), dereferencing that dangling pointer.
    if (auto wnd = qobject_cast<QMdiSubWindow*>(frm->parentWidget()))
        frm->removeEventFilter(wnd);

    frm->setParent(_mainWindow);
    frm->hide();
    addClosedForm(frm);
    _projectTree->setFormOpen(frm, false);
}

///
/// \brief AppProject::nextFormDisplayNumber
/// Returns the next sequential display number for the given form kind.
///
int AppProject::nextFormDisplayNumber(ProjectFormKind kind)
{
    switch(kind)
    {
        case ProjectFormKind::Data:        return ++_dataCounter;
        case ProjectFormKind::Traffic:     return ++_trafficCounter;
        case ProjectFormKind::Script:      return ++_scriptCounter;
        case ProjectFormKind::DataMap: return ++_DataMapCounter;
    }
    return 0;
}

///
/// \brief AppProject::createMdiChild
/// \return
///
QWidget* AppProject::createMdiChild(ProjectFormKind kind)
{
    return createMdiChildOnArea(kind, activeCreateArea(), true);
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
    if(!area)
        return nullptr;

    QWidget* frm = nullptr;
    switch (kind)
    {
        case ProjectFormKind::Data:
            frm = new FormDataView(_mbServer, _dataSimulator, _mainWindow);
            break;
        case ProjectFormKind::Traffic:
            frm = new FormTrafficView(_mbServer, _mainWindow);
            break;
        case ProjectFormKind::Script:
            frm = new FormScriptView(_mbServer, _dataSimulator, _mainWindow);
            break;
        case ProjectFormKind::DataMap:
            frm = new FormDataMapView(_mbServer, _mainWindow);
            break;
    }
    if(!frm)
        return nullptr;

    frm->setProperty(kFormIdProperty, QUuid::createUuid());

    if(addToWindowList)
    {
        const int num = nextFormDisplayNumber(kind);
        switch(kind)
        {
            case ProjectFormKind::Data:        frm->setWindowTitle(QString("Data%1").arg(num));        break;
            case ProjectFormKind::Traffic:     frm->setWindowTitle(QString("Traffic%1").arg(num));     break;
            case ProjectFormKind::Script:      frm->setWindowTitle(QString("Script%1").arg(num));      break;
            case ProjectFormKind::DataMap:     frm->setWindowTitle(QString("Map%1").arg(num));         break;
        }
    }

    enableAutoCompleteOnForm(frm, AppPreferences::instance().codeAutoComplete());

    auto wnd = area->addSubWindow(frm);
    if(!wnd)
    {
        frm->deleteLater();
        return nullptr;
    }

    wnd->installEventFilter(_mainWindow);
    wnd->setAttribute(Qt::WA_DeleteOnClose, true);
    setupMdiChild(frm, wnd, addToWindowList);

    return frm;
}

///
/// \brief AppProject::setupMdiChild
/// \param frm
/// \param wnd
/// \param addToWindowList
///
void AppProject::setupMdiChild(QWidget* frm, QMdiSubWindow* wnd, bool addToWindowList)
{
    if(!frm || !wnd)
        return;

    // Keep the subwindow (and therefore tab icon/title) in sync with the form.
    wnd->setWindowTitle(frm->windowTitle());
    wnd->setWindowIcon(frm->windowIcon());
    connect(frm, &QWidget::windowTitleChanged, wnd, &QMdiSubWindow::setWindowTitle);
    connect(frm, &QWidget::windowIconChanged, wnd, &QMdiSubWindow::setWindowIcon);

    auto updateCodepage = [this](const QString& name)
    {
        _mainWindow->selectAnsiCodepage(name);
    };

    if (auto* data = qobject_cast<FormDataView*>(frm)) {
        connect(data, &FormDataView::codepageChanged, _mainWindow, [updateCodepage](const QString& name) { updateCodepage(name); });
    }

    connect(wnd, &QMdiSubWindow::windowStateChanged, _mainWindow,
            [this, frm, updateCodepage](Qt::WindowStates, Qt::WindowStates newState)
    {
        switch(newState & ~Qt::WindowMaximized & ~Qt::WindowMinimized)
        {
            case Qt::WindowActive:
                _mainWindow->updateHelpWidgetState();
                updateCodepage(codepageOfForm(frm));
                connectEditSlotsOnForm(frm);
            break;

            case Qt::WindowNoState:
                disconnectEditSlotsOnForm(frm);
            break;
        }
    });

    auto onPointTypeChanged = [frm](QModbusDataUnit::RegisterType type)
    {
        switch(type)
        {
            case QModbusDataUnit::Coils:
            case QModbusDataUnit::DiscreteInputs:
                frm->setProperty("PrevDataType", QVariant::fromValue(dataTypeOfForm(frm)));
                setDataTypeOnForm(frm, DataType::Binary);
                break;
            case QModbusDataUnit::HoldingRegisters:
            case QModbusDataUnit::InputRegisters:
            {
                const auto prevType = frm->property("PrevDataType");
                if(prevType.isValid())
                    setDataTypeOnForm(frm, prevType.value<DataType>());
            }
            break;
            default:
                break;
        }
    };
    if (auto* data = qobject_cast<FormDataView*>(frm)) {
        connect(data, &FormDataView::pointTypeChanged, _mainWindow, onPointTypeChanged);
    }

    auto onShowed = [this, frm]
    {
        AppTrace::log("AppProject::onShowed",
                      QStringLiteral("form=%1 state=%2")
                          .arg(formTag(frm))
                          .arg(mdiExState(_mdiArea)));
        // Activate whichever subwindow currently holds this form
        for (auto w : _mdiArea->subWindowList()) {
            if (w && w->widget() == frm) {
                AppTrace::log("AppProject::onShowed",
                              QStringLiteral("calling MainWindow::windowActivate %1")
                                  .arg(AppTrace::subWindowTag(w)));
                _mainWindow->windowActivate(w);
                break;
            }
        }
    };
    if (auto* data = qobject_cast<FormDataView*>(frm)) {
        connect(data, &FormDataView::showed, _mainWindow, onShowed);
    } else if (auto* traffic = qobject_cast<FormTrafficView*>(frm)) {
        connect(traffic, &FormTrafficView::showed, _mainWindow, onShowed);
    } else if (auto* script = qobject_cast<FormScriptView*>(frm)) {
        connect(script, &FormScriptView::showed, _mainWindow, onShowed);
    } else if (auto* regMap = qobject_cast<FormDataMapView*>(frm)) {
        connect(regMap, &FormDataMapView::showed, _mainWindow, onShowed);
    }

    auto onHelpRequested = [this](const QString& helpKey)
    {
        _mainWindow->showHelpContext(helpKey);
    };
    if (auto* data = qobject_cast<FormDataView*>(frm)) {
        connect(data, &FormDataView::helpContextRequested, _mainWindow, onHelpRequested);
    } else if (auto* script = qobject_cast<FormScriptView*>(frm)) {
        connect(script, &FormScriptView::helpContextRequested, _mainWindow, onHelpRequested);
    }

    auto onConsoleMessage = [this](const QString& source, const QString& text, ConsoleOutput::MessageType type) {
        _mainWindow->appendConsoleMessage(source, text, type);
    };
    if (auto* script = qobject_cast<FormScriptView*>(frm)) {
        connect(script, &FormScriptView::consoleMessage, _mainWindow, onConsoleMessage);
    }

    auto onScriptRunning = [this, frm]()
    {
        frm->setProperty(kSplitScriptRunning, true);
        _projectTree->setFormScriptRunning(frm, true);
        updateSplitPairScriptIcons(frm);
        _mainWindow->showOutputConsole();
    };
    auto onScriptStopped = [this, frm]()
    {
        frm->setProperty(kSplitScriptRunning, false);
        _projectTree->setFormScriptRunning(frm, false);
        updateSplitPairScriptIcons(frm);
    };
    if (auto* script = qobject_cast<FormScriptView*>(frm)) {
        connect(script, &FormScriptView::scriptRunning, _mainWindow, onScriptRunning);
        connect(script, &FormScriptView::scriptStopped, _mainWindow, onScriptStopped);
    }

    connect(wnd, &QObject::destroyed, _mdiArea, [this]() {
        resetSplitViewIfEmpty();
    });

    if (auto* data = qobject_cast<FormDataView*>(frm)) {
        connect(data, &FormDataView::definitionChanged, _mainWindow, &MainWindow::markModified);
    } else if (auto* traffic = qobject_cast<FormTrafficView*>(frm)) {
        connect(traffic, &FormTrafficView::definitionChanged, _mainWindow, &MainWindow::markModified);
    } else if (auto* regMap = qobject_cast<FormDataMapView*>(frm)) {
        connect(regMap, &FormDataMapView::definitionChanged, _mainWindow, &MainWindow::markModified);
    } else if (auto* script = qobject_cast<FormScriptView*>(frm)) {
        connect(script, &FormScriptView::scriptSettingsChanged, _mainWindow, [this](const ScriptSettings&) { _mainWindow->markModified(); });
        connect(script->scriptDocument(), &QTextDocument::contentsChanged, _mainWindow, &MainWindow::markModified);
    }

    if(addToWindowList) {
        bool okKind = false;
        const auto kind = projectFormKindFromWidget(frm, &okKind);
        if(okKind) {
            _projectTree->addForm(toProjectFormType(kind), frm);
            connect(frm, &QWidget::windowTitleChanged, _projectTree, [this, frm](const QString&) {
                _projectTree->updateFormTitle(frm);
            });
        }
    }
}

///
/// \brief AppProject::rewrapMdiChild
/// Re-opens a previously "closed" (hidden) QWidget by creating a new MDI subwindow for it.
///
void AppProject::rewrapMdiChild(QWidget* frm)
{
    if (!frm || !containsClosedForm(frm))
        return;

    auto area = activeCreateArea();
    if (!area)
        return;

    removeClosedForm(frm);

    frm->setParent(nullptr); // detach from MainWindow before adding to MDI area
    auto wnd = area->addSubWindow(frm);
    if (!wnd) {
        frm->setParent(_mainWindow);
        frm->hide();
        addClosedForm(frm);
        return;
    }

    wnd->installEventFilter(_mainWindow);
    wnd->setAttribute(Qt::WA_DeleteOnClose, true);
    wnd->setWindowTitle(frm->windowTitle());
    wnd->setWindowIcon(frm->windowIcon());

    // Window-specific connections (new wnd each time)
    auto updateCodepage = [this](const QString& name) { _mainWindow->selectAnsiCodepage(name); };
    connect(wnd, &QMdiSubWindow::windowStateChanged, _mainWindow,
            [this, frm, updateCodepage](Qt::WindowStates, Qt::WindowStates newState)
    {
        switch(newState & ~Qt::WindowMaximized & ~Qt::WindowMinimized)
        {
            case Qt::WindowActive:
                _mainWindow->updateHelpWidgetState();
                updateCodepage(codepageOfForm(frm));
                connectEditSlotsOnForm(frm);
            break;
            case Qt::WindowNoState:
                disconnectEditSlotsOnForm(frm);
            break;
        }
    });

    connect(wnd, &QObject::destroyed, _mdiArea, [this]() {
        resetSplitViewIfEmpty();
    });

    _projectTree->setFormOpen(frm, true);
    AppTrace::log("AppProject::rewrapMdiChild",
                  QStringLiteral("activateForm form=%1").arg(formTag(frm)));
    _projectTree->activateForm(frm);

    frm->show();
}

///
/// \brief AppProject::closeMdiChild
/// \param frm
///
void AppProject::closeMdiChild(QWidget* frm)
{
    for(auto&& wnd : _mdiArea->subWindowList()) {
        auto* f = wnd ? wnd->widget() : nullptr;
        if(f == frm) wnd->close();
    }
}

///
/// \brief AppProject::deleteForm
/// Deletes a form permanently from the project (closes its tab if open).
///
void AppProject::deleteForm(QWidget* frm)
{
    if (!frm) return;
    if (frm->property(kDeleteLockedProperty).toBool())
        return;

    // Close the MDI subwindow if the form is currently open
    for (auto wnd : _mdiArea->subWindowList()) {
        if (wnd && wnd->widget() == frm) {
            wnd->close(); // triggers closing signal -> moves frm to _closedForms
            break;
        }
    }

    // Now frm is either in _closedForms (just closed) or was already there
    removeClosedForm(frm);
    _projectTree->removeForm(frm);
    delete frm;
}

///
/// \brief AppProject::currentMdiChild
/// \return
///
QWidget* AppProject::currentMdiChild() const
{
    auto wnd = _mdiArea->currentSubWindow();
    if (!wnd) {
        AppTrace::log("AppProject::currentMdiChild",
                      QStringLiteral("mdiArea->currentSubWindow() is null; state=%1")
                          .arg(mdiExState(_mdiArea)));
    }

    if(!wnd && _mdiArea->viewMode() == QMdiArea::TabbedView) {
        // Qt5: d->active may still be null on the first event loop iteration
        // because _q_currentTabChanged is posted via QueuedConnection while
        // awake() fires before posted events are processed. Read the tab bar
        // directly to get the correct subwindow.
        if (const auto* tabBar = qobject_cast<const MdiTabBar*>(_mdiArea->tabBar())) {
            wnd = tabBar->subWindowAt(tabBar->currentIndex());
        } else {
            const auto* fallbackTabBar = _mdiArea->tabBar();
            const auto list = _mdiArea->subWindowList();
            const auto idx = fallbackTabBar ? fallbackTabBar->currentIndex() : -1;
            if(idx >= 0 && idx < list.size())
                wnd = list.at(idx);
        }

        AppTrace::log("AppProject::currentMdiChild",
                      QStringLiteral("fallback picked wnd=%1 state=%2")
                          .arg(AppTrace::subWindowTag(wnd))
                          .arg(mdiExState(_mdiArea)));
    }

    AppTrace::log("AppProject::currentMdiChild",
                  QStringLiteral("result=%1").arg(AppTrace::subWindowTag(wnd)));
    return wnd ? qobject_cast<QWidget*>(wnd->widget()) : nullptr;
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
    for(auto&& wnd : _mdiArea->subWindowList())
    {
        const auto frm = qobject_cast<QWidget*>(wnd->widget());
        if(frm && formIdOf(frm) == id) return frm;
    }
    return nullptr;
}

///
/// \brief AppProject::findMdiChildInArea
/// \param area
/// \param id
/// \return
///
QWidget* AppProject::findMdiChildInArea(MdiArea* area, QUuid id) const
{
    if(!area)
        return nullptr;

    for(auto&& wnd : area->localSubWindowList())
    {
        const auto frm = qobject_cast<QWidget*>(wnd->widget());
        if(frm && formIdOf(frm) == id)
            return frm;
    }

    return nullptr;
}

///
/// \brief AppProject::resolveFormForActiveArea
/// \param primaryForm
/// \return The clone of primaryForm in the secondary area if the secondary area is active,
///         otherwise primaryForm itself.
///
QWidget* AppProject::resolveFormForActiveArea(QWidget* primaryForm) const
{
    if(!primaryForm || !isSplitTabbedView())
        return primaryForm;

    auto* secondary = secondaryArea();
    if(!secondary)
        return primaryForm;

    if(activeCreateArea() != secondary)
        return primaryForm;

    const QUuid originId = formIdOf(primaryForm);
    for(auto* wnd : secondary->localSubWindowList())
    {
        auto* cloneFrm = qobject_cast<QWidget*>(wnd ? wnd->widget() : nullptr);
        if(cloneFrm &&
           cloneFrm->property(kSplitAutoCloneProperty).toBool() &&
           cloneFrm->property(kSplitOriginIdProperty).toUuid() == originId)
        {
            return cloneFrm;
        }
    }

    return primaryForm;
}

///
/// \brief AppProject::createCloneOnArea
/// Creates a visual clone of \a source on \a area. Returns the clone widget, or nullptr on failure.
///
QWidget* AppProject::createCloneOnArea(QWidget* source, MdiArea* area)
{
    if(!source || !area)
        return nullptr;

    bool okKind = false;
    const auto cloneKind = projectFormKindFromWidget(source, &okKind);
    if(!okKind)
        return nullptr;

    auto* clone = createMdiChildOnArea(cloneKind, area, false);
    if(!clone)
        return nullptr;

    QUuid originId = source->property(kSplitOriginIdProperty).toUuid();
    if(originId.isNull())
        originId = formIdOf(source);

    cloneMdiChildState(source, clone);
    clone->setWindowTitle(source->windowTitle());
    clone->setWindowIcon(source->windowIcon());
    clone->setFont(source->font()); // copy form-level font (affects definitions panel labels)
    clone->setProperty(kSplitOriginIdProperty, originId);
    clone->setProperty(kSplitAutoCloneProperty, true);
    clone->setProperty(kSplitScriptRunning, source->property(kSplitScriptRunning));

    connect(source, &QWidget::windowTitleChanged, clone, [clone](const QString& title) {
        if(clone->windowTitle() != title)
            clone->setWindowTitle(title);
    });
    connect(source, &QWidget::windowIconChanged, clone, [clone](const QIcon& icon) {
        clone->setWindowIcon(icon);
    });

    if(auto* srcScript = qobject_cast<FormScriptView*>(source)) {
        if(auto* cloneScript = qobject_cast<FormScriptView*>(clone)) {
            cloneScript->setScriptDocument(srcScript->scriptDocument());
            cloneScript->setScriptSettings(srcScript->scriptSettings());
            connect(srcScript,   &FormScriptView::scriptSettingsChanged,
                    cloneScript, &FormScriptView::setScriptSettings);
            connect(cloneScript, &FormScriptView::scriptSettingsChanged,
                    srcScript,   &FormScriptView::setScriptSettings);
            connect(srcScript, &QWidget::windowTitleChanged, cloneScript, [cloneScript](const QString& title) {
                if(cloneScript->windowTitle() != title)
                    cloneScript->setFormName(title);
            });
            cloneScript->linkRunStopTo(srcScript);
        }
    }

    if(auto* srcTraffic = qobject_cast<FormTrafficView*>(source)) {
        if(auto* cloneTraffic = qobject_cast<FormTrafficView*>(clone)) {
            cloneTraffic->linkTo(srcTraffic);
        }
    }

    if(auto* srcData = qobject_cast<FormDataView*>(source)) {
        if(auto* cloneData = qobject_cast<FormDataView*>(clone)) {
            cloneData->linkTo(srcData);
        }
    }

    clone->show();
    if(auto* cloneWnd = qobject_cast<QMdiSubWindow*>(clone->parentWidget()))
        if(area->viewMode() == QMdiArea::TabbedView)
            cloneWnd->showMaximized();

    return clone;
}

///
/// \brief AppProject::openFormOnActivePanel
/// Activates \a frm (or its split clone) on the active panel.
/// If the form is not present there, opens it on that panel.
///
void AppProject::openFormOnActivePanel(QWidget* frm)
{
    if(!frm)
        return;

    auto* panel = _mdiArea->activePanel();

    // Search in active panel for frm or its clone (clones share the same title)
    if(panel) {
        const QString title = frm->windowTitle();
        for(auto* wnd : panel->localSubWindowList()) {
            auto* w = wnd ? wnd->widget() : nullptr;
            if(!w) continue;
            if(w == frm || (w->property(kSplitAutoCloneProperty).toBool() && w->windowTitle() == title)) {
                traceMdiActivationRequest("AppProject::openFormOnActivePanel",
                                          _mdiArea, wnd,
                                          QStringLiteral("found target on active panel"));
                _mdiArea->setActiveSubWindow(wnd);
                return;
            }
        }
    }

    // Form is closed - rewrap on the active panel
    if(containsClosedForm(frm)) {
        rewrapMdiChild(frm);
        return;
    }

    // Form is open in the other panel - create a clone on the active panel
    if(isSplitTabbedView() && panel) {
        auto* clone = createCloneOnArea(frm, panel);
        if(clone) {
            if(auto* cloneWnd = qobject_cast<QMdiSubWindow*>(clone->parentWidget())) {
                traceMdiActivationRequest("AppProject::openFormOnActivePanel",
                                          _mdiArea, cloneWnd,
                                          QStringLiteral("activating newly created clone"));
                _mdiArea->setActiveSubWindow(cloneWnd);
            }
            return;
        }
    }

    // Fallback: activate wherever the form is open
    for(auto* wnd : _mdiArea->subWindowList()) {
        if(wnd->widget() == frm) {
            traceMdiActivationRequest("AppProject::openFormOnActivePanel",
                                      _mdiArea, wnd,
                                      QStringLiteral("fallback activate existing window"));
            _mdiArea->setActiveSubWindow(wnd);
            return;
        }
    }
}

///
/// \brief AppProject::firstMdiChild
/// \return
///
QWidget* AppProject::firstMdiChild() const
{
    for(auto&& wnd : _mdiArea->subWindowList())
        return qobject_cast<QWidget*>(wnd->widget());

    return nullptr;
}

///
/// \brief AppProject::cloneMdiChildState
/// \param source
/// \param target
/// \return
///
bool AppProject::cloneMdiChildState(QWidget* source, QWidget* target) const
{
    if(!source || !target)
        return false;

    QByteArray xmlBuffer;
    QBuffer writeBuffer(&xmlBuffer);
    if(!writeBuffer.open(QIODevice::WriteOnly))
        return false;

    QXmlStreamWriter writer(&writeBuffer);
    writer.writeStartDocument();
    saveXmlOfForm(source, writer);
    writer.writeEndDocument();
    writeBuffer.close();

    QBuffer readBuffer(&xmlBuffer);
    if(!readBuffer.open(QIODevice::ReadOnly))
        return false;

    QXmlStreamReader reader(&readBuffer);
    if(!reader.readNextStartElement())
        return false;

    loadXmlOfForm(target, reader);
    if(reader.hasError())
        return false;

    return true;
}

///
/// \brief AppProject::activeCreateArea
/// \return
///
MdiArea* AppProject::activeCreateArea() const
{
    auto* primary = _mdiArea->primaryArea();
    if(!primary)
        return nullptr;

    if(!isSplitTabbedView())
        return primary;

    auto* secondary = secondaryArea();
    if(!secondary)
        return primary;

    if(auto* focus = QApplication::focusWidget()) {
        if(secondary->isAncestorOf(focus))
            return secondary;
        if(primary->isAncestorOf(focus))
            return primary;
    }

    // Focus is outside both areas (e.g., project tree, menu bar).
    // Use the last area where a subwindow was activated.
    return _mdiArea->activePanel();
}

///
/// \brief AppProject::areaOfForm
/// \param frm
/// \return
///
MdiArea* AppProject::areaOfForm(QWidget* frm) const
{
    if(!frm)
        return nullptr;

    auto* primary = _mdiArea->primaryArea();
    if(!primary)
        return nullptr;

    auto* wnd = qobject_cast<QMdiSubWindow*>(frm->parentWidget());
    if(!wnd)
        return nullptr;

    if(primary->localSubWindowList().contains(wnd))
        return primary;

    if(auto* secondary = secondaryArea())
        if(secondary->localSubWindowList().contains(wnd))
            return secondary;

    return nullptr;
}

///
/// \brief AppProject::secondaryArea
/// \return
///
MdiArea* AppProject::secondaryArea() const
{
    return _mdiArea->secondaryArea();
}

///
/// \brief AppProject::isSplitTabbedView
/// \return
///
bool AppProject::isSplitTabbedView() const
{
    return _mdiArea->viewMode() == QMdiArea::TabbedView &&
           _mdiArea->isSplitView() &&
           secondaryArea() != nullptr;
}

///
/// \brief AppProject::canMoveFormToOtherPanel
/// Returns true when \a frm can be moved to the opposite panel.
/// Auto-clones are excluded; split view must be active.
///
bool AppProject::canMoveFormToOtherPanel(QWidget* frm) const
{
    if(!frm || !isSplitTabbedView())
        return false;

    if(frm->property(kSplitAutoCloneProperty).toBool())
        return false;

    return areaOfForm(frm) != nullptr;
}

///
/// \brief AppProject::moveFormToOtherPanel
/// Moves \a frm from its current panel to the opposite panel.
/// If an auto-clone of this form already exists in the target panel it is
/// deleted first, so there is never a duplicate.
///
void AppProject::moveFormToOtherPanel(QWidget* frm, QPoint globalDropPos)
{
    if(!canMoveFormToOtherPanel(frm))
        return;

    auto* sourceArea    = areaOfForm(frm);
    auto* primaryArea   = _mdiArea->primaryArea();
    auto* secondaryArea = _mdiArea->secondaryArea();
    auto* targetArea    = (sourceArea == primaryArea) ? secondaryArea : primaryArea;
    if(!targetArea)
        return;

    // Remove auto-clone of this form from target panel (if any).
    const QUuid originId = frm->property(kSplitOriginIdProperty).toUuid().isNull()
                               ? formIdOf(frm)
                               : frm->property(kSplitOriginIdProperty).toUuid();

    for(auto* wnd : targetArea->localSubWindowList()) {
        auto* candidate = qobject_cast<QWidget*>(wnd ? wnd->widget() : nullptr);
        if(candidate
            && candidate->property(kSplitAutoCloneProperty).toBool()
            && candidate->property(kSplitOriginIdProperty).toUuid() == originId)
        {
            // WA_DeleteOnClose is set; auto-clones bypass markFormClosed (see eventFilter guard)
            wnd->close();
            break;
        }
    }

    auto* subWnd = qobject_cast<QMdiSubWindow*>(frm->parentWidget());
    if(subWnd)
        _mdiArea->moveSubWindowToOtherPanel(subWnd, globalDropPos);
}

///
/// \brief AppProject::resetSplitViewIfEmpty
///
void AppProject::resetSplitViewIfEmpty()
{
    if(!isSplitTabbedView())
        return;

    auto* secondary = secondaryArea();
    if(!secondary || !secondary->localSubWindowList().isEmpty())
        return;

    _mdiArea->setSplitViewEnabled(false);
}

///
/// \brief AppProject::isScriptRunningOnSplitPair
/// \param frm
/// \return
///
bool AppProject::isScriptRunningOnSplitPair(QWidget* frm) const
{
    if(!frm)
        return false;

    return canStopScriptOnForm(frm) || frm->property(kSplitScriptRunning).toBool();
}

///
/// \brief AppProject::updateSplitPairScriptIcons
/// \param frm
///
void AppProject::updateSplitPairScriptIcons(QWidget* frm)
{
    if(!frm)
        return;

    auto applyIcon = [this](QWidget* target, bool running)
    {
        if(!target)
            return;

        auto targetWnd = qobject_cast<QMdiSubWindow*>(target->parentWidget());
        if(!targetWnd)
            return;

        if(running)
            crossFadeWindowIcon(targetWnd, targetWnd->windowIcon(), QIcon(":/res/actionRunScript.png"));
        else
            crossFadeWindowIcon(targetWnd, targetWnd->windowIcon(), target->windowIcon());
    };

    const bool periodicMode = scriptSettingsOfForm(frm).Mode == RunMode::Periodically;
    const bool running = periodicMode && isScriptRunningOnSplitPair(frm);
    applyIcon(frm, running);
}

///
/// \brief AppProject::duplicatePrimaryTabsToSecondary
/// Opens a clone of the currently active primary tab on the secondary panel.
/// Reuses an existing auto-clone on primary (leftover from a previous split) if available.
/// \return 1 if a window was placed on secondary, 0 otherwise
///
int AppProject::duplicatePrimaryTabsToSecondary()
{
    if(!isSplitTabbedView())
        return 0;

    auto* primary = _mdiArea->primaryArea();
    auto* secondary = secondaryArea();
    if(!primary || !secondary)
        return 0;

    // Tabbed split should keep activated subwindows maximized on both panels.
    primary->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);
    secondary->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);

    // Secondary must be empty when split is just enabled.
    if(!secondary->localSubWindowList().isEmpty())
        return 0;

    auto* primaryActiveWnd = _mdiArea->activePrimarySubWindow();
    if(!primaryActiveWnd)
        return 0;

    auto* activeFrm = qobject_cast<QWidget*>(primaryActiveWnd->widget());
    if(!activeFrm)
        return 0;

    // Ensure origin tracking properties are initialized.
    QUuid activeOriginId = activeFrm->property(kSplitOriginIdProperty).toUuid();
    if(activeOriginId.isNull()) {
        activeOriginId = formIdOf(activeFrm);
        activeFrm->setProperty(kSplitOriginIdProperty, activeOriginId);
    }
    if(!activeFrm->property(kSplitAutoCloneProperty).isValid())
        activeFrm->setProperty(kSplitAutoCloneProperty, false);

    // If the active window is itself an auto-clone, resolve to its non-clone counterpart.
    if(activeFrm->property(kSplitAutoCloneProperty).toBool()) {
        for(auto* wnd : primary->localSubWindowList()) {
            auto* frm = qobject_cast<QWidget*>(wnd ? wnd->widget() : nullptr);
            if(frm
                && frm->property(kSplitOriginIdProperty).toUuid() == activeOriginId
                && !frm->property(kSplitAutoCloneProperty).toBool()) {
                activeFrm = frm;
                primaryActiveWnd = wnd;
                break;
            }
        }
    }

    // Check for an existing auto-clone of the active form on primary (leftover from a previous split).
    QMdiSubWindow* toMove = nullptr;
    for(auto* wnd : primary->localSubWindowList()) {
        auto* frm = qobject_cast<QWidget*>(wnd ? wnd->widget() : nullptr);
        if(frm
            && frm->property(kSplitOriginIdProperty).toUuid() == activeOriginId
            && frm->property(kSplitAutoCloneProperty).toBool()) {
            toMove = wnd;
            break;
        }
    }

    QMdiSubWindow* secondaryWnd = nullptr;
    if(toMove) {
        // Reuse the existing auto-clone rather than creating a new one.
        primary->removeSubWindow(toMove);
        secondary->addSubWindow(toMove, Qt::WindowFlags());
        if(auto* frm = qobject_cast<QWidget*>(toMove->widget()))
            frm->show();
        secondaryWnd = toMove;
    } else {
        // Split clones are visual peers only: do not add them to project tree/window menu.
        auto* clone = createCloneOnArea(activeFrm, secondary);
        if(clone)
            secondaryWnd = qobject_cast<QMdiSubWindow*>(clone->parentWidget());
    }

    if(secondaryWnd) {
        if(secondary->viewMode() == QMdiArea::TabbedView && !secondaryWnd->isMaximized())
            secondaryWnd->showMaximized();
        AppTrace::log("AppProject::splitActiveFormToSecondary",
                      QStringLiteral("secondary->setActiveSubWindow %1")
                          .arg(AppTrace::subWindowTag(secondaryWnd)));
        secondary->setActiveSubWindow(secondaryWnd);
    }

    // Restore the primary panel's active tab (moving toMove may have shifted focus).
    AppTrace::log("AppProject::splitActiveFormToSecondary",
                  QStringLiteral("restore primary->setActiveSubWindow %1")
                      .arg(AppTrace::subWindowTag(primaryActiveWnd)));
    primary->setActiveSubWindow(primaryActiveWnd);

    return secondaryWnd ? 1 : 0;
}

///
/// \brief AppProject::removeSplitAutoClonesFromSecondary
/// Removes transient split clones from secondary panel before split merge.
///
void AppProject::removeSplitAutoClonesFromSecondary()
{
    if(!isSplitTabbedView())
        return;

    auto* secondary = secondaryArea();
    if(!secondary)
        return;

    const auto secondaryWindows = secondary->localSubWindowList();
    for(auto* wnd : secondaryWindows)
    {
        auto* frm = qobject_cast<QWidget*>(wnd ? wnd->widget() : nullptr);
        if(frm && frm->property(kSplitAutoCloneProperty).toBool())
            wnd->close();
    }
}

///
/// \brief AppProject::loadProject
/// \param filename
///
void AppProject::loadProject(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QFile::ReadOnly))
        return;

    _mbServer.clearDescriptions();
    _mbServer.clearTimestamps();

    ModbusDefinitions defs;
    QList<ConnectionDetails> conns;
    QMdiArea::ViewMode viewMode = QMdiArea::TabbedView;
    bool splitView = false;
    QString activePrimaryWin;
    QString activeSecWin;
    bool viewPreparedForForms = false;
    QStringList secondaryForms;
    bool hasSecondaryFormsSaved = false;
    QStringList primaryTabOrder;
    QStringList secondaryTabOrder;

    AddressDescriptionMap globalDescriptionMap;
    bool hasGlobalDescriptionMap = false;
    AddressTimestampMap globalTimestampMap;
    bool hasGlobalTimestampMap = false;
    struct PendingValue {
        quint8 deviceId;
        QModbusDataUnit::RegisterType type;
        quint16 address;
        quint16 value;
    };
    QList<PendingValue> pendingValues;

    QXmlStreamReader xml(&file);
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("OpenModSim")) {
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("AppPreferences")) {
                    // Backward compatibility: ignore legacy app-preferences block in project files.
                    xml.skipCurrentElement();
                }
                else if (xml.name() == QLatin1String("ViewSettings")) {
                    const auto attrs = xml.attributes();
                    viewMode = (QMdiArea::ViewMode)qBound(0, attrs.value("ViewMode").toInt(), 1);
                    splitView = attrs.value("SplitView").toInt() != 0;
                    activePrimaryWin = attrs.value("ActivePrimaryWindow").toString();
                    activeSecWin = attrs.value("ActiveSecondaryWindow").toString();
                    xml.skipCurrentElement();
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
                    if(!viewPreparedForForms) {
                        _mainWindow->setViewMode(viewMode);
                        if(_mdiArea->viewMode() == QMdiArea::TabbedView && _mdiArea->isSplitView() != splitView)
                            _mdiArea->setSplitViewEnabled(splitView);
                        viewPreparedForForms = true;
                    }

                    _mdiArea->closeAllSubWindows();
                    // Clean up forms that were already closed (hidden)
                    const auto closed = _closedForms;
                    for (auto&& frm : closed) {
                        _projectTree->removeForm(frm);
                        delete frm;
                    }
                    _closedForms.clear();
                    while (xml.readNextStartElement()) {
                        ProjectFormKind kind;
                        bool isForm = true;
                        if (xml.name() == QLatin1String("FormDataView")) {
                            kind = ProjectFormKind::Data;
                        } else if (xml.name() == QLatin1String("FormTrafficView")) {
                            kind = ProjectFormKind::Traffic;
                        } else if (xml.name() == QLatin1String("FormScriptView")) {
                            kind = ProjectFormKind::Script;
                        } else if (xml.name() == QLatin1String("FormDataMapView")) {
                            kind = ProjectFormKind::DataMap;
                        } else {
                            isForm = false;
                        }

                        if (isForm) {
                            MdiArea* targetArea = _mdiArea->primaryArea();
                            const auto attrs = xml.attributes();
                            const QString panel = attrs.value("Panel").toString();
                            const bool isClosed = attrs.value("Closed").toString() == "1";
                            if(splitView && panel.compare(QLatin1String(kPanelRight), Qt::CaseInsensitive) == 0) {
                                if(auto* secondary = secondaryArea())
                                    targetArea = secondary;
                            }

                            auto frm = createMdiChildOnArea(kind, targetArea, true);
                            if (frm) {
                                loadXmlOfForm(frm, xml);
                                if (isClosed) {
                                    // Park closed forms directly without emitting close/activation churn.
                                    auto* wnd = qobject_cast<QMdiSubWindow*>(frm->parentWidget());
                                    markFormClosed(frm);
                                    if (wnd) {
                                        targetArea->removeSubWindow(wnd);
                                        wnd->deleteLater();
                                    }
                                } else {
                                    frm->show();
                                }
                            } else {
                                xml.skipCurrentElement();
                            }
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                }
                else if (xml.name() == QLatin1String("SplitSecondaryForms")) {
                    hasSecondaryFormsSaved = true;
                    while(xml.readNextStartElement()) {
                        if(xml.name() == QLatin1String("FormRef"))
                            secondaryForms << xml.attributes().value("title").toString();
                        xml.skipCurrentElement();
                    }
                }
                else if (xml.name() == QLatin1String("TabOrder")) {
                    const auto attrs = xml.attributes();
                    const QString panel = attrs.value("Panel").toString();
                    QStringList* targetOrder = &primaryTabOrder;
                    if(panel.compare(QLatin1String(kPanelRight), Qt::CaseInsensitive) == 0)
                        targetOrder = &secondaryTabOrder;

                    while(xml.readNextStartElement()) {
                        if(xml.name() == QLatin1String("TabRef"))
                            targetOrder->append(xml.attributes().value("title").toString());
                        xml.skipCurrentElement();
                    }
                }
                else if (xml.name() == QLatin1String("Scripts")) {
                    xml.skipCurrentElement();
                }
                else if (xml.name() == QLatin1String("AddressSpace")) {
                    while (xml.readNextStartElement()) {
                        if (xml.name() == QLatin1String("AddressDescriptionMap")) {
                            hasGlobalDescriptionMap = true;
                            xml >> globalDescriptionMap;
                        }
                        else if (xml.name() == QLatin1String("AddressTimestampMap")) {
                            hasGlobalTimestampMap = true;
                            xml >> globalTimestampMap;
                        }
                        else if (xml.name() == QLatin1String("ModbusSimulationMap")) {
                            while (xml.readNextStartElement()) {
                                if (xml.name() == QLatin1String("Simulation")) {
                                    const auto attrs = xml.attributes();
                                    bool ok;
                                    const quint8 deviceId = static_cast<quint8>(attrs.value("DeviceId").toUShort(&ok));
                                    if (ok) {
                                        const auto type = static_cast<QModbusDataUnit::RegisterType>(attrs.value("Type").toInt(&ok));
                                        if (ok) {
                                            const quint16 addr = attrs.value("Address").toUShort(&ok);
                                            if (ok && xml.readNextStartElement()) {
                                                ModbusSimulationParams params;
                                                xml >> params;
                                                _dataSimulator->startSimulation(deviceId, type, addr, params);
                                            }
                                        }
                                    }
                                    xml.skipCurrentElement();
                                } else {
                                    xml.skipCurrentElement();
                                }
                            }
                        }
                        else if (xml.name() == QLatin1String("ModbusDataValues")) {
                            while (xml.readNextStartElement()) {
                                if (xml.name() == QLatin1String("Value")) {
                                    const auto attrs = xml.attributes();
                                    bool ok;
                                    const quint8 deviceId = static_cast<quint8>(attrs.value("DeviceId").toUShort(&ok));
                                    if (ok) {
                                        const auto type = static_cast<QModbusDataUnit::RegisterType>(attrs.value("Type").toInt(&ok));
                                        if (ok) {
                                            const quint16 address = attrs.value("Address").toUShort(&ok);
                                            if (ok) {
                                                const quint16 value = xml.readElementText().toUShort(&ok);
                                                if (ok) pendingValues.append({deviceId, type, address, value});
                                                continue;
                                            }
                                        }
                                    }
                                    xml.skipCurrentElement();
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
        }
        else {
            xml.skipCurrentElement();
        }
    }

    if(!viewPreparedForForms) {
        _mainWindow->setViewMode(viewMode);
        if(_mdiArea->viewMode() == QMdiArea::TabbedView && _mdiArea->isSplitView() != splitView)
            _mdiArea->setSplitViewEnabled(splitView);
    }

    // Apply values from <AddressSpace> (requires forms to exist so _mbServer has unit maps)
    for (const auto& pv : std::as_const(pendingValues)) {
        QModbusDataUnit unit(pv.type, pv.address, 1);
        unit.setValue(0, pv.value);
        _mbServer.setData(pv.deviceId, unit);
    }

    // Prefer global AddressSpace metadata when present; otherwise keep legacy per-form values.
    if (hasGlobalDescriptionMap)
        _mbServer.setDescriptionMap(globalDescriptionMap);
    if (hasGlobalTimestampMap)
        _mbServer.setTimestampMap(globalTimestampMap);

    _mainWindow->applyConnections(defs, conns);
    syncAutoRequestMap(_mbServer.getModbusDefinitions());

    // Restore secondary panel state.
    if(splitView && secondaryArea()) {
        if(hasSecondaryFormsSaved) {
            // Restore exactly the forms that were open on the secondary panel.
            if(auto* primary = _mdiArea->primaryArea()) {
                for(const QString& title : secondaryForms) {
                    for(auto* wnd : primary->localSubWindowList()) {
                        auto* w = wnd ? wnd->widget() : nullptr;
                        if(w && w->windowTitle() == title) {
                            createCloneOnArea(w, secondaryArea());
                            break;
                        }
                    }
                }
            }
        } else {
            // Old project file without SplitSecondaryForms: duplicate all primary forms.
            duplicatePrimaryTabsToSecondary();
        }
    }

    if(auto* primary = _mdiArea->primaryArea())
        applyTabOrderToArea(primary, primaryTabOrder);
    if(splitView)
        if(auto* secondary = secondaryArea())
            applyTabOrderToArea(secondary, secondaryTabOrder);

    _pendingActivePrimaryWin = activePrimaryWin;
    _pendingActiveSecWin = splitView ? activeSecWin : QString();

    if(_mdiArea->isVisible())
        restoreActiveWindows();
}

///
/// \brief AppProject::restoreActiveWindows
/// Restores the active sub-window selection after a project load.
/// Called immediately if the MDI area is already visible, or deferred via
/// MainWindow::showEvent if the window has not been shown yet.
///
void AppProject::restoreActiveWindows()
{
    if(_pendingActivePrimaryWin.isEmpty() && _pendingActiveSecWin.isEmpty())
        return;

    if(!_pendingActivePrimaryWin.isEmpty()) {
        if(auto* primary = _mdiArea->primaryArea()) {
            for(auto* wnd : primary->localSubWindowList()) {
                if(wnd && wnd->widget() && wnd->widget()->windowTitle() == _pendingActivePrimaryWin) {
                    AppTrace::log("AppProject::restoreActiveWindows",
                                  QStringLiteral("primary->setActiveSubWindow %1")
                                      .arg(AppTrace::subWindowTag(wnd)));
                    primary->setActiveSubWindow(wnd);
                    break;
                }
            }
        }
    }

    if(!_pendingActiveSecWin.isEmpty()) {
        if(auto* secondary = secondaryArea()) {
            for(auto* wnd : secondary->localSubWindowList()) {
                if(wnd && wnd->widget() && wnd->widget()->windowTitle() == _pendingActiveSecWin) {
                    AppTrace::log("AppProject::restoreActiveWindows",
                                  QStringLiteral("secondary->setActiveSubWindow %1")
                                      .arg(AppTrace::subWindowTag(wnd)));
                    secondary->setActiveSubWindow(wnd);
                    break;
                }
            }
        }
    }

    // Final safety sync: make sure the visible tab and the active QMdi page are aligned.
    if (auto* primary = _mdiArea->primaryArea()) {
        if (auto* tabBar = qobject_cast<MdiTabBar*>(primary->tabBar())) {
            if (auto* tabWnd = tabBar->currentSubWindow()) {
                AppTrace::log("AppProject::restoreActiveWindows",
                              QStringLiteral("sync primary->setActiveSubWindow(tabCurrent) %1")
                                  .arg(AppTrace::subWindowTag(tabWnd)));
                primary->setActiveSubWindow(tabWnd);
            }
        }
    }

    _pendingActivePrimaryWin.clear();
    _pendingActiveSecWin.clear();
}

///
/// \brief AppProject::saveProject
/// \param filename
///
void AppProject::saveProject(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QFile::WriteOnly))
        return;

    QXmlStreamWriter w(&file);
    w.setAutoFormatting(true);

    w.writeStartDocument();
    w.writeStartElement("OpenModSim");
    w.writeAttribute("Version", qApp->applicationVersion());

    w << _mbServer.getModbusDefinitions();

    w.writeStartElement("Connections");
    for(auto&& cd : _mbServer.connections()) {
        w << cd;
    }
    w.writeEndElement(); // Connections

    {
        const auto globalDescriptionMap = _mbServer.descriptionMap();
        const auto globalTimestampMap = _mbServer.timestampMap();

        w.writeStartElement("AddressSpace");

        w << globalDescriptionMap;
        w << globalTimestampMap;

        {
            const auto simMap = _dataSimulator->simulationMap();
            w.writeStartElement("ModbusSimulationMap");
            for (auto it = simMap.constBegin(); it != simMap.constEnd(); ++it) {
                const auto& key = it.key();
                const auto& params = it.value();
                if (params.Mode != SimulationMode::Off && params.Mode != SimulationMode::Disabled) {
                    w.writeStartElement("Simulation");
                    w.writeAttribute("DeviceId", QString::number(key.DeviceId));
                    w.writeAttribute("Type", QString::number(key.Type));
                    w.writeAttribute("Address", QString::number(key.Address));
                    w << params;
                    w.writeEndElement(); // Simulation
                }
            }
            w.writeEndElement(); // ModbusSimulationMap
        }

        {
            const auto maxLen = (_mbServer.getModbusDefinitions().AddrSpace == AddressSpace::Addr6Digits)
                                ? quint16(65535) : quint16(9999);
            const QList<QModbusDataUnit::RegisterType> regTypes = {
                QModbusDataUnit::Coils, QModbusDataUnit::DiscreteInputs,
                QModbusDataUnit::InputRegisters, QModbusDataUnit::HoldingRegisters
            };
            w.writeStartElement("ModbusDataValues");
            for (const int deviceId : _mbServer.registeredDeviceIds()) {
                for (auto regType : regTypes) {
                    const auto unit = _mbServer.data(static_cast<quint8>(deviceId), regType, 0, maxLen);
                    quint16 address = 0;
                    for (const auto value : unit.values()) {
                        if (value != 0) {
                            w.writeStartElement("Value");
                            w.writeAttribute("DeviceId", QString::number(deviceId));
                            w.writeAttribute("Type", QString::number(regType));
                            w.writeAttribute("Address", QString::number(address));
                            w.writeCharacters(QString::number(value));
                            w.writeEndElement(); // Value
                        }
                        address++;
                    }
                }
            }
            w.writeEndElement(); // ModbusDataValues
        }

        w.writeEndElement(); // AddressSpace
    }

    w.writeStartElement("ViewSettings");
    w.writeAttribute("ViewMode", QString::number(_mdiArea->viewMode()));
    w.writeAttribute("SplitView", _mdiArea->isSplitView() ? "1" : "0");
    if(auto primary = _mdiArea->primaryArea())
        if(auto wnd = primary->activeSubWindow())
            if(auto frm = wnd->widget())
                w.writeAttribute("ActivePrimaryWindow", frm->windowTitle());
    if(isSplitTabbedView())
        if(auto secondary = secondaryArea())
            if(auto wnd = secondary->activeSubWindow())
                if(auto frm = wnd->widget())
                    w.writeAttribute("ActiveSecondaryWindow", frm->windowTitle());
    w.writeEndElement(); // ViewSettings

    w.writeStartElement("Forms");
    for(auto&& wnd : _mdiArea->subWindowList()) {
        auto* widget = wnd ? wnd->widget() : nullptr;
        if(!widget || widget->property(kSplitAutoCloneProperty).toBool())
            continue;

        const bool onRight = areaOfForm(qobject_cast<QWidget*>(widget)) == secondaryArea();
        widget->setProperty(kFormPanelProperty, onRight ? QLatin1String(kPanelRight) : QLatin1String(kPanelLeft));
        saveXmlOfForm(widget, w);
        widget->setProperty(kFormPanelProperty, QVariant());
    }
    // Also save forms that are closed (hidden in project tree)
    const auto closed = _closedForms;
    for (auto&& frm : closed) {
        if (frm) {
            frm->setProperty(kFormPanelProperty, QLatin1String(kPanelLeft));
            frm->setProperty(kFormClosedProperty, true);
            saveXmlOfForm(frm, w);
            frm->setProperty(kFormPanelProperty, QVariant());
            frm->setProperty(kFormClosedProperty, QVariant());
        }
    }
    w.writeEndElement(); // Forms

    if(isSplitTabbedView()) {
        if(auto* secondary = secondaryArea()) {
            w.writeStartElement("SplitSecondaryForms");
            for(auto* wnd : secondary->localSubWindowList()) {
                auto* widget = wnd ? wnd->widget() : nullptr;
                if(!widget || !widget->property(kSplitAutoCloneProperty).toBool())
                    continue;
                w.writeStartElement("FormRef");
                w.writeAttribute("title", widget->windowTitle());
                w.writeEndElement(); // FormRef
            }
            w.writeEndElement(); // SplitSecondaryForms
        }
    }

    auto writeTabOrder = [&](MdiArea* area, const char* panel) {
        const auto order = tabTitlesForArea(area);
        if(order.isEmpty())
            return;
        w.writeStartElement("TabOrder");
        w.writeAttribute("Panel", panel);
        for(const auto& title : order) {
            w.writeStartElement("TabRef");
            w.writeAttribute("title", title);
            w.writeEndElement(); // TabRef
        }
        w.writeEndElement(); // TabOrder
    };
    writeTabOrder(_mdiArea->primaryArea(), kPanelLeft);
    if(isSplitTabbedView())
        writeTabOrder(secondaryArea(), kPanelRight);

    w.writeEndElement(); // OpenModSim
    w.writeEndDocument();
}

///
/// \brief AppProject::destroyContentForShutdown
///
void AppProject::destroyContentForShutdown()
{
    closeProject();
}

