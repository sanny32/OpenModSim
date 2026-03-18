#include <QtWidgets>
#include <QBuffer>
#include <QSet>
#include "apppreferences.h"
#include "appproject.h"
#include "mainwindow.h"
#include "controls/mdiareaex.h"
#include "controls/projecttreewidget.h"
#include "windowactionlist.h"
#include "formdataview.h"
#include "formtrafficview.h"
#include "formscriptview.h"
#include "uiutils.h"

namespace {
constexpr const char* kFormPanelProperty = "SplitPanel";
constexpr const char* kPanelLeft = "L";
constexpr const char* kPanelRight = "R";
constexpr const char* kSplitOriginIdProperty = "SplitOriginId";
constexpr const char* kSplitAutoCloneProperty = "SplitAutoClone";

ProjectFormType toProjectFormType(ProjectFormKind kind)
{
    switch (kind)
    {
        case ProjectFormKind::Data:
            return ProjectFormType::Data;
        case ProjectFormKind::Traffic:
            return ProjectFormType::Traffic;
        case ProjectFormKind::Script:
            return ProjectFormType::Script;
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

    if(ok) *ok = false;
    return ProjectFormKind::Data;
}

int formIdOf(QWidget* widget)
{
    if (!widget)
        return -1;

    const QString title = widget->windowTitle();
    int idx = title.size() - 1;
    while (idx >= 0 && title.at(idx).isDigit())
        --idx;

    bool ok = false;
    const int id = title.mid(idx + 1).toInt(&ok);
    return ok ? id : -1;
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

DataDisplayMode dataDisplayModeOfForm(QWidget* widget)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) return frm->dataDisplayMode();
    return DataDisplayMode::Hex;
}

void setDataDisplayModeOnForm(QWidget* widget, DataDisplayMode mode)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->setDataDisplayMode(mode);
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
}

void loadXmlOfForm(QWidget* widget, QXmlStreamReader& r)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->loadXml(r);
    else if (auto* frm = qobject_cast<FormTrafficView*>(widget)) frm->loadXml(r);
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->loadXml(r);
    else r.skipCurrentElement();
}

}

///
/// \brief AppProject::AppProject
///
AppProject::AppProject(MdiAreaEx* mdiArea,
                       ModbusMultiServer& mbServer,
                       DataSimulator* dataSimulator,
                       ProjectTreeWidget* projectTree,
                       WindowActionList* windowActionList,
                       MainWindow* mainWindow,
                       QObject* parent)
    : QObject(parent)
    , _mdiArea(mdiArea)
    , _mbServer(mbServer)
    , _dataSimulator(dataSimulator)
    , _projectTree(projectTree)
    , _windowActionList(windowActionList)
    , _mainWindow(mainWindow)
{
    Q_ASSERT(_dataSimulator != nullptr);
}

///
/// \brief AppProject::~AppProject
///
AppProject::~AppProject()
{
}

void AppProject::addClosedForm(QWidget* frm)
{
    if(!frm)
        return;

    if(auto* data = qobject_cast<FormDataView*>(frm)) {
        if(!_closedDataForms.contains(data))
            _closedDataForms.append(data);
        return;
    }
    if(auto* traffic = qobject_cast<FormTrafficView*>(frm)) {
        if(!_closedTrafficForms.contains(traffic))
            _closedTrafficForms.append(traffic);
        return;
    }
    if(auto* script = qobject_cast<FormScriptView*>(frm)) {
        if(!_closedScriptForms.contains(script))
            _closedScriptForms.append(script);
    }
}

void AppProject::removeClosedForm(QWidget* frm)
{
    if(!frm)
        return;

    if(auto* data = qobject_cast<FormDataView*>(frm)) {
        _closedDataForms.removeOne(data);
        return;
    }
    if(auto* traffic = qobject_cast<FormTrafficView*>(frm)) {
        _closedTrafficForms.removeOne(traffic);
        return;
    }
    if(auto* script = qobject_cast<FormScriptView*>(frm)) {
        _closedScriptForms.removeOne(script);
    }
}

bool AppProject::containsClosedForm(QWidget* frm) const
{
    if(!frm)
        return false;

    if(auto* data = qobject_cast<FormDataView*>(frm))
        return _closedDataForms.contains(data);
    if(auto* traffic = qobject_cast<FormTrafficView*>(frm))
        return _closedTrafficForms.contains(traffic);
    if(auto* script = qobject_cast<FormScriptView*>(frm))
        return _closedScriptForms.contains(script);
    return false;
}

QList<QWidget*> AppProject::allClosedForms() const
{
    QList<QWidget*> forms;
    forms.reserve(_closedDataForms.size() + _closedTrafficForms.size() + _closedScriptForms.size());
    for(auto* f : _closedDataForms)
        forms.append(f);
    for(auto* f : _closedTrafficForms)
        forms.append(f);
    for(auto* f : _closedScriptForms)
        forms.append(f);
    return forms;
}

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
    const auto closed = allClosedForms();
    for (auto&& frm : closed) {
        _projectTree->removeForm(frm);
        delete frm;
    }
    _closedDataForms.clear();
    _closedTrafficForms.clear();
    _closedScriptForms.clear();

    _windowCounter = 0;
}

///
/// \brief AppProject::markFormClosed
/// Called when a primary form's subwindow is being closed — reparents the form so it survives.
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

FormDataView* AppProject::createDataMdiChild(int id)
{
    return qobject_cast<FormDataView*>(createMdiChild(id, ProjectFormKind::Data));
}

FormTrafficView* AppProject::createTrafficMdiChild(int id)
{
    return qobject_cast<FormTrafficView*>(createMdiChild(id, ProjectFormKind::Traffic));
}

FormScriptView* AppProject::createScriptMdiChild(int id)
{
    return qobject_cast<FormScriptView*>(createMdiChild(id, ProjectFormKind::Script));
}

FormDataView* AppProject::createDataMdiChildOnArea(int id, MdiArea* area, bool addToWindowList)
{
    return qobject_cast<FormDataView*>(createMdiChildOnArea(id, ProjectFormKind::Data, area, addToWindowList));
}

FormTrafficView* AppProject::createTrafficMdiChildOnArea(int id, MdiArea* area, bool addToWindowList)
{
    return qobject_cast<FormTrafficView*>(createMdiChildOnArea(id, ProjectFormKind::Traffic, area, addToWindowList));
}

FormScriptView* AppProject::createScriptMdiChildOnArea(int id, MdiArea* area, bool addToWindowList)
{
    return qobject_cast<FormScriptView*>(createMdiChildOnArea(id, ProjectFormKind::Script, area, addToWindowList));
}

///
/// \brief AppProject::createMdiChild
/// \param id
/// \return
///
QWidget* AppProject::createMdiChild(int id, ProjectFormKind kind)
{
    while(findMdiChild(id))
        ++id;

    _windowCounter = qMax(_windowCounter, id);
    return createMdiChildOnArea(id, kind, activeCreateArea(), true);
}

///
/// \brief AppProject::createMdiChildOnArea
/// \param id
/// \param area
/// \param addToWindowList
/// \return
///
QWidget* AppProject::createMdiChildOnArea(int id, ProjectFormKind kind, MdiArea* area, bool addToWindowList)
{
    if(!area)
        return nullptr;

    QWidget* frm = nullptr;
    switch (kind)
    {
        case ProjectFormKind::Data:
            frm = new FormDataView(id, _mbServer, _dataSimulator, _mainWindow);
            break;
        case ProjectFormKind::Traffic:
            frm = new FormTrafficView(id, _mbServer, _mainWindow);
            break;
        case ProjectFormKind::Script:
            frm = new FormScriptView(id, _mbServer, _dataSimulator, _mainWindow);
            break;
    }
    if(!frm)
        return nullptr;

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
                frm->setProperty("PrevDataDisplayMode", QVariant::fromValue(dataDisplayModeOfForm(frm)));
                setDataDisplayModeOnForm(frm, DataDisplayMode::Binary);
                break;
            case QModbusDataUnit::HoldingRegisters:
            case QModbusDataUnit::InputRegisters:
            {
                const auto mode = frm->property("PrevDataDisplayMode");
                if(mode.isValid())
                    setDataDisplayModeOnForm(frm, mode.value<DataDisplayMode>());
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
        // Activate whichever subwindow currently holds this form
        for (auto w : _mdiArea->subWindowList()) {
            if (w && w->widget() == frm) {
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
        _mainWindow->showConsoleMessage(source, text, type);
    };
    if (auto* script = qobject_cast<FormScriptView*>(frm)) {
        connect(script, &FormScriptView::consoleMessage, _mainWindow, onConsoleMessage);
    }

    auto onScriptRunning = [this, frm]()
    {
        frm->setProperty(kSplitScriptRunning, true);
        _projectTree->setFormScriptRunning(frm, true);
        updateSplitPairScriptIcons(frm);
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

    if(addToWindowList) {
        _windowActionList->addWindow(wnd);
        bool okKind = false;
        const auto kind = projectFormKindFromWidget(frm, &okKind);
        if(okKind)
            _projectTree->addForm(toProjectFormType(kind), frm);
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

    _windowActionList->addWindow(wnd);
    _projectTree->setFormOpen(frm, true);
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

    // Close the MDI subwindow if the form is currently open
    for (auto wnd : _mdiArea->subWindowList()) {
        if (wnd && wnd->widget() == frm) {
            wnd->close(); // triggers closing signal → moves frm to _closedForms
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
    if(!wnd && _mdiArea->viewMode() == QMdiArea::TabbedView) {
        // Qt5: d->active may still be null on the first event loop iteration
        // because _q_currentTabChanged is posted via QueuedConnection while
        // awake() fires before posted events are processed. Read the tab bar
        // directly to get the correct subwindow.
        const auto tabBar = _mdiArea->tabBar();
        const auto list = _mdiArea->subWindowList();
        const auto idx = tabBar ? tabBar->currentIndex() : -1;
        if(idx >= 0 && idx < list.size())
            wnd = list.at(idx);
    }
    return wnd ? qobject_cast<QWidget*>(wnd->widget()) : nullptr;
}

FormDataView* AppProject::currentDataMdiChild() const
{
    return qobject_cast<FormDataView*>(currentMdiChild());
}

FormTrafficView* AppProject::currentTrafficMdiChild() const
{
    return qobject_cast<FormTrafficView*>(currentMdiChild());
}

FormScriptView* AppProject::currentScriptMdiChild() const
{
    return qobject_cast<FormScriptView*>(currentMdiChild());
}

///
/// \brief AppProject::findMdiChild
/// \param id
/// \return
///
QWidget* AppProject::findMdiChild(int id) const
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
QWidget* AppProject::findMdiChildInArea(MdiArea* area, int id) const
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

    auto* secondary = splitSecondaryArea();
    if(!secondary)
        return primary;

    if(auto activeWnd = _mdiArea->activeSubWindow()) {
        if(secondary->localSubWindowList().contains(activeWnd))
            return secondary;
        if(primary->localSubWindowList().contains(activeWnd))
            return primary;
    }

    if(auto* focus = QApplication::focusWidget()) {
        if(secondary->isAncestorOf(focus))
            return secondary;
        if(primary->isAncestorOf(focus))
            return primary;
    }

    return primary;
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

    if(auto* secondary = splitSecondaryArea())
        if(secondary->localSubWindowList().contains(wnd))
            return secondary;

    return nullptr;
}

///
/// \brief AppProject::splitSecondaryArea
/// \return
///
MdiArea* AppProject::splitSecondaryArea() const
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
           splitSecondaryArea() != nullptr;
}

///
/// \brief AppProject::resetSplitViewIfEmpty
///
void AppProject::resetSplitViewIfEmpty()
{
    if(!isSplitTabbedView())
        return;

    auto* secondary = splitSecondaryArea();
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
/// Rebuilds split tabs from primary panel:
/// - creates a secondary copy for tabs that exist once;
/// - reuses previously auto-cloned tabs (moves one back to secondary) to avoid growth.
/// \return number of tabs moved/duplicated into secondary
///
int AppProject::duplicatePrimaryTabsToSecondary()
{
    if(!isSplitTabbedView())
        return 0;

    auto* primary = _mdiArea->primaryArea();
    auto* secondary = splitSecondaryArea();
    if(!primary || !secondary)
        return 0;

    // Tabbed split should keep activated subwindows maximized on both panels.
    primary->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);
    secondary->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);

    auto normalizeTabbedState = [](QMdiSubWindow* wnd, MdiArea* area)
    {
        if(!wnd || !area)
            return;

        if(area->viewMode() == QMdiArea::TabbedView && !wnd->isMaximized())
            wnd->showMaximized();
    };

    auto normalizeAllTabbedStates = [normalizeTabbedState](MdiArea* area)
    {
        if(!area || area->viewMode() != QMdiArea::TabbedView)
            return;

        for(auto* wnd : area->localSubWindowList())
            normalizeTabbedState(wnd, area);
    };

    // Split was just enabled: secondary must be empty.
    if(!secondary->localSubWindowList().isEmpty())
        return 0;

    const auto primaryWindows = primary->localSubWindowList();
    if(primaryWindows.isEmpty())
        return 0;

    auto* primaryActive = primary->activeSubWindow();
    QMdiSubWindow* secondaryActive = nullptr;
    int movedOrDuplicated = 0;

    QHash<int, QList<QMdiSubWindow*>> groupsByOrigin;
    QList<int> orderedOrigins;
    QList<int> autoOnlyOrigins;
    QSet<int> seenNonAuto;
    QSet<int> seenAny;
    int activeOriginId = 0;

    for(auto* wnd : primaryWindows)
    {
        auto* frm = qobject_cast<QWidget*>(wnd ? wnd->widget() : nullptr);
        if(!frm)
            continue;

        int originId = frm->property(kSplitOriginIdProperty).toInt();
        if(originId <= 0) {
            originId = formIdOf(frm);
            frm->setProperty(kSplitOriginIdProperty, originId);
        }
        if(!frm->property(kSplitAutoCloneProperty).isValid())
            frm->setProperty(kSplitAutoCloneProperty, false);

        const bool isAutoClone = frm->property(kSplitAutoCloneProperty).toBool();
        groupsByOrigin[originId].append(wnd);
        if(wnd == primaryActive)
            activeOriginId = originId;

        if(!isAutoClone) {
            if(!seenNonAuto.contains(originId)) {
                orderedOrigins.append(originId);
                seenNonAuto.insert(originId);
            }
        } else if(!seenAny.contains(originId)) {
            autoOnlyOrigins.append(originId);
        }
        seenAny.insert(originId);
    }

    for(int originId : autoOnlyOrigins)
        if(!seenNonAuto.contains(originId))
            orderedOrigins.append(originId);

    for(int originId : orderedOrigins)
    {
        const auto group = groupsByOrigin.value(originId);
        if(group.isEmpty())
            continue;

        QMdiSubWindow* sourceWnd = nullptr;
        for(auto* candidate : group) {
            auto* candidateFrm = qobject_cast<QWidget*>(candidate ? candidate->widget() : nullptr);
            if(candidateFrm && !candidateFrm->property(kSplitAutoCloneProperty).toBool()) {
                sourceWnd = candidate;
                break;
            }
        }
        if(!sourceWnd)
            sourceWnd = group.first();

        auto* source = qobject_cast<QWidget*>(sourceWnd ? sourceWnd->widget() : nullptr);
        if(!source)
            continue;

        if(group.size() > 1)
        {
            QMdiSubWindow* toMove = nullptr;
            for(auto* candidate : group) {
                auto* candidateFrm = qobject_cast<QWidget*>(candidate ? candidate->widget() : nullptr);
                if(candidateFrm && candidateFrm->property(kSplitAutoCloneProperty).toBool()) {
                    toMove = candidate;
                    break;
                }
            }
            if(!toMove)
                toMove = group.last();

            primary->removeSubWindow(toMove);
            secondary->addSubWindow(toMove, Qt::WindowFlags());
            if(auto* movedFrm = qobject_cast<QWidget*>(toMove->widget()))
                movedFrm->show();
            normalizeTabbedState(toMove, secondary);

            ++movedOrDuplicated;
            if(originId == activeOriginId)
                secondaryActive = toMove;
            continue;
        }

        int cloneId = _windowCounter + 1;
        while(findMdiChild(cloneId))
            ++cloneId;
        _windowCounter = cloneId;

        // Split clones are visual peers only: do not add them to project tree/window menu.
        bool okKind = false;
        const auto cloneKind = projectFormKindFromWidget(source, &okKind);
        if(!okKind)
            continue;
        auto* clone = createMdiChildOnArea(cloneId, cloneKind, secondary, false);
        if(!clone)
            continue;

        cloneMdiChildState(source, clone);
        clone->setProperty(kSplitOriginIdProperty, originId);
        clone->setProperty(kSplitAutoCloneProperty, true);
        clone->setProperty(kSplitScriptRunning, source->property(kSplitScriptRunning));
        clone->show();
        if(auto* cloneWnd = qobject_cast<QMdiSubWindow*>(clone->parentWidget()))
            normalizeTabbedState(cloneWnd, secondary);
        ++movedOrDuplicated;

        if(originId == activeOriginId)
            secondaryActive = qobject_cast<QMdiSubWindow*>(clone->parentWidget());
    }

    if(secondaryActive)
        secondary->setActiveSubWindow(secondaryActive);

    // Final synchronous normalization.
    normalizeAllTabbedStates(secondary);

    return movedOrDuplicated;
}

///
/// \brief AppProject::removeSplitAutoClonesFromSecondary
/// Removes transient split clones from secondary panel before split merge.
///
void AppProject::removeSplitAutoClonesFromSecondary()
{
    if(!isSplitTabbedView())
        return;

    auto* secondary = splitSecondaryArea();
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

    ModbusDefinitions defs;
    QList<ConnectionDetails> conns;
    QMdiArea::ViewMode viewMode = QMdiArea::TabbedView;
    bool splitView = false;
    QString activePrimaryWin;
    QString activeSecWin;
    bool viewPreparedForForms = false;

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
                    const auto closed = allClosedForms();
                    for (auto&& frm : closed) {
                        _projectTree->removeForm(frm);
                        delete frm;
                    }
                    _closedDataForms.clear();
                    _closedTrafficForms.clear();
                    _closedScriptForms.clear();
                    while (xml.readNextStartElement()) {
                        ProjectFormKind kind;
                        bool isForm = true;
                        if (xml.name() == QLatin1String("FormDataView")) {
                            kind = ProjectFormKind::Data;
                        } else if (xml.name() == QLatin1String("FormTrafficView")) {
                            kind = ProjectFormKind::Traffic;
                        } else if (xml.name() == QLatin1String("FormScriptView")) {
                            kind = ProjectFormKind::Script;
                        } else {
                            isForm = false;
                        }

                        if (isForm) {
                            MdiArea* targetArea = _mdiArea->primaryArea();
                            const auto attrs = xml.attributes();
                            const QString panel = attrs.value("Panel").toString();
                            if(splitView && panel.compare(QLatin1String(kPanelRight), Qt::CaseInsensitive) == 0) {
                                if(auto* secondary = splitSecondaryArea())
                                    targetArea = secondary;
                            }

                            auto frm = createMdiChildOnArea(++_windowCounter, kind, targetArea, true);
                            if (frm) {
                                loadXmlOfForm(frm, xml);
                                frm->show();
                            } else {
                                xml.skipCurrentElement();
                            }
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                }
                else if (xml.name() == QLatin1String("Scripts")) {
                    xml.skipCurrentElement();
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

    _mainWindow->applyConnections(defs, conns);

    if(!activePrimaryWin.isEmpty())
        if(auto primary = _mdiArea->primaryArea())
            for(auto&& wnd : primary->localSubWindowList())
                if(wnd && wnd->widget() && wnd->widget()->windowTitle() == activePrimaryWin)
                {
                    primary->setActiveSubWindow(wnd);
                    break;
                }

    if(splitView)
    {
        if(!activeSecWin.isEmpty())
            if(auto secondary = splitSecondaryArea())
                for(auto&& wnd : secondary->localSubWindowList())
                    if(wnd && wnd->widget() && wnd->widget()->windowTitle() == activeSecWin)
                    {
                        secondary->setActiveSubWindow(wnd);
                        break;
                    }
    }
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

    w.writeStartElement("ViewSettings");
    w.writeAttribute("ViewMode", QString::number(_mdiArea->viewMode()));
    w.writeAttribute("SplitView", _mdiArea->isSplitView() ? "1" : "0");
    if(auto primary = _mdiArea->primaryArea())
        if(auto wnd = primary->activeSubWindow())
            if(auto frm = wnd->widget())
                w.writeAttribute("ActivePrimaryWindow", frm->windowTitle());
    if(isSplitTabbedView())
        if(auto secondary = splitSecondaryArea())
            if(auto wnd = secondary->activeSubWindow())
                if(auto frm = wnd->widget())
                    w.writeAttribute("ActiveSecondaryWindow", frm->windowTitle());
    w.writeEndElement(); // ViewSettings

    w.writeStartElement("Forms");
    for(auto&& wnd : _mdiArea->subWindowList()) {
        auto* widget = wnd ? wnd->widget() : nullptr;
        if(!widget || widget->property(kSplitAutoCloneProperty).toBool())
            continue;

        const bool onRight = areaOfForm(qobject_cast<QWidget*>(widget)) == splitSecondaryArea();
        widget->setProperty(kFormPanelProperty, onRight ? QLatin1String(kPanelRight) : QLatin1String(kPanelLeft));
        if (auto* frm = qobject_cast<FormDataView*>(widget))
            frm->saveXml(w);
        else if (auto* frm = qobject_cast<FormTrafficView*>(widget))
            frm->saveXml(w);
        else if (auto* frm = qobject_cast<FormScriptView*>(widget))
            frm->saveXml(w);
        widget->setProperty(kFormPanelProperty, QVariant());
    }
    // Also save forms that are closed (hidden in project tree)
    const auto closed = allClosedForms();
    for (auto&& frm : closed) {
        if (frm) {
            frm->setProperty(kFormPanelProperty, QLatin1String(kPanelLeft));
            saveXmlOfForm(frm, w);
            frm->setProperty(kFormPanelProperty, QVariant());
        }
    }
    w.writeEndElement(); // Forms

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
