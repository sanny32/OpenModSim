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

///
/// \brief AppProject::closeProject
/// Closes all open and hidden forms, resetting the workspace.
///
void AppProject::closeProject()
{
    // Close open MDI windows (FormModSim windows)
    _mdiArea->closeAllSubWindows();

    // Delete forms that were closed (hidden)
    for (auto&& frm : _closedForms) {
        _projectTree->removeForm(frm);
        delete frm;
    }
    _closedForms.clear();

    _windowCounter = 0;
}

///
/// \brief AppProject::markFormClosed
/// Called when a primary form's subwindow is being closed — reparents the form so it survives.
///
void AppProject::markFormClosed(FormModSim* frm)
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
    if (!_closedForms.contains(frm))
        _closedForms.append(frm);
    _projectTree->setFormOpen(frm, false);
}

///
/// \brief AppProject::createMdiChild
/// \param id
/// \return
///
FormModSim* AppProject::createMdiChild(int id, FormModSim::FormKind kind)
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
FormModSim* AppProject::createMdiChildOnArea(int id, FormModSim::FormKind kind, MdiArea* area, bool addToWindowList)
{
    if(!area)
        return nullptr;

    FormModSim* frm = nullptr;
    switch (kind)
    {
        case FormModSim::FormKind::Data:
            frm = new FormDataView(id, _mbServer, _dataSimulator, _mainWindow);
            break;
        case FormModSim::FormKind::Traffic:
            frm = new FormTrafficView(id, _mbServer, _dataSimulator, _mainWindow);
            break;
        case FormModSim::FormKind::Script:
            frm = new FormScriptView(id, _mbServer, _dataSimulator, _mainWindow);
            break;
    }
    if(!frm)
        return nullptr;

    frm->enableAutoComplete(AppPreferences::instance().codeAutoComplete());

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
void AppProject::setupMdiChild(FormModSim* frm, QMdiSubWindow* wnd, bool addToWindowList)
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

    connect(frm, &FormModSim::codepageChanged, _mainWindow, [updateCodepage](const QString& name)
    {
        updateCodepage(name);
    });

    connect(wnd, &QMdiSubWindow::windowStateChanged, _mainWindow,
            [this, frm, updateCodepage](Qt::WindowStates, Qt::WindowStates newState)
    {
        switch(newState & ~Qt::WindowMaximized & ~Qt::WindowMinimized)
        {
            case Qt::WindowActive:
                _mainWindow->updateHelpWidgetState();
                updateCodepage(frm->codepage());
                frm->connectEditSlots();
            break;

            case Qt::WindowNoState:
                frm->disconnectEditSlots();
            break;
        }
    });

    connect(frm, &FormModSim::pointTypeChanged, _mainWindow, [frm](QModbusDataUnit::RegisterType type)
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

    connect(frm, &FormModSim::showed, _mainWindow, [this, frm]
    {
        // Activate whichever subwindow currently holds this form
        for (auto w : _mdiArea->subWindowList()) {
            if (qobject_cast<FormModSim*>(w->widget()) == frm) {
                _mainWindow->windowActivate(w);
                break;
            }
        }
    });

    connect(frm, &FormModSim::captureError, _mainWindow, [this](const QString& error)
    {
        QMessageBox::critical(_mainWindow, _mainWindow->windowTitle(), tr("Capture Error:\r\n%1").arg(error));
    });

    connect(frm, &FormModSim::helpContextRequested, _mainWindow, [this](const QString& helpKey)
    {
        _mainWindow->showHelpContext(helpKey);
    });

    connect(frm, &FormModSim::consoleMessage, _mainWindow, [this](const QString& source, const QString& text, ConsoleOutput::MessageType type) {
        _mainWindow->showConsoleMessage(source, text, type);
    });

    connect(frm, &FormModSim::scriptRunning, _mainWindow, [this, frm]()
    {
        frm->setProperty(kSplitScriptRunning, true);
        updateSplitPairScriptIcons(frm);
    });

    connect(frm, &FormModSim::scriptStopped, _mainWindow, [this, frm]()
    {
        frm->setProperty(kSplitScriptRunning, false);
        updateSplitPairScriptIcons(frm);
    });

    connect(wnd, &QObject::destroyed, _mdiArea, [this]() {
        resetSplitViewIfEmpty();
    });

    if(addToWindowList) {
        _windowActionList->addWindow(wnd);
        _projectTree->addForm(frm);
    }
}

///
/// \brief AppProject::rewrapMdiChild
/// Re-opens a previously "closed" (hidden) FormModSim by creating a new MDI subwindow for it.
///
void AppProject::rewrapMdiChild(FormModSim* frm)
{
    if (!frm || !_closedForms.contains(frm))
        return;

    auto area = activeCreateArea();
    if (!area)
        return;

    _closedForms.removeOne(frm);

    frm->setParent(nullptr); // detach from MainWindow before adding to MDI area
    auto wnd = area->addSubWindow(frm);
    if (!wnd) {
        frm->setParent(_mainWindow);
        frm->hide();
        _closedForms.append(frm);
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
                updateCodepage(frm->codepage());
                frm->connectEditSlots();
            break;
            case Qt::WindowNoState:
                frm->disconnectEditSlots();
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
void AppProject::closeMdiChild(FormModSim* frm)
{
    for(auto&& wnd : _mdiArea->subWindowList()) {
        const auto f = qobject_cast<FormModSim*>(wnd->widget());
        if(f == frm) wnd->close();
    }
}

///
/// \brief AppProject::deleteForm
/// Deletes a form permanently from the project (closes its tab if open).
///
void AppProject::deleteForm(FormModSim* frm)
{
    if (!frm) return;

    // Close the MDI subwindow if the form is currently open
    for (auto wnd : _mdiArea->subWindowList()) {
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
/// \brief AppProject::currentMdiChild
/// \return
///
FormModSim* AppProject::currentMdiChild() const
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
    return wnd ? qobject_cast<FormModSim*>(wnd->widget()) : nullptr;
}

///
/// \brief AppProject::findMdiChild
/// \param id
/// \return
///
FormModSim* AppProject::findMdiChild(int id) const
{
    for(auto&& wnd : _mdiArea->subWindowList())
    {
        const auto frm = qobject_cast<FormModSim*>(wnd->widget());
        if(frm && frm->formId() == id) return frm;
    }
    return nullptr;
}

///
/// \brief AppProject::findMdiChildInArea
/// \param area
/// \param id
/// \return
///
FormModSim* AppProject::findMdiChildInArea(MdiArea* area, int id) const
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
/// \brief AppProject::firstMdiChild
/// \return
///
FormModSim* AppProject::firstMdiChild() const
{
    for(auto&& wnd : _mdiArea->subWindowList())
        return qobject_cast<FormModSim*>(wnd->widget());

    return nullptr;
}

///
/// \brief AppProject::cloneMdiChildState
/// \param source
/// \param target
/// \return
///
bool AppProject::cloneMdiChildState(FormModSim* source, FormModSim* target) const
{
    if(!source || !target)
        return false;

    QByteArray xmlBuffer;
    QBuffer writeBuffer(&xmlBuffer);
    if(!writeBuffer.open(QIODevice::WriteOnly))
        return false;

    QXmlStreamWriter writer(&writeBuffer);
    writer.writeStartDocument();
    source->saveXml(writer);
    writer.writeEndDocument();
    writeBuffer.close();

    QBuffer readBuffer(&xmlBuffer);
    if(!readBuffer.open(QIODevice::ReadOnly))
        return false;

    QXmlStreamReader reader(&readBuffer);
    if(!reader.readNextStartElement())
        return false;

    target->loadXml(reader);
    if(reader.hasError())
        return false;

    target->setFilename(source->filename());
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
MdiArea* AppProject::areaOfForm(FormModSim* frm) const
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
bool AppProject::isScriptRunningOnSplitPair(FormModSim* frm) const
{
    if(!frm)
        return false;

    return frm->canStopScript() || frm->property(kSplitScriptRunning).toBool();
}

///
/// \brief AppProject::updateSplitPairScriptIcons
/// \param frm
///
void AppProject::updateSplitPairScriptIcons(FormModSim* frm)
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
            crossFadeWindowIcon(targetWnd, targetWnd->windowIcon(), _mainWindow->runScriptIcon());
        else
            crossFadeWindowIcon(targetWnd, targetWnd->windowIcon(), target->windowIcon());
    };

    const bool periodicMode = frm->scriptSettings().Mode == RunMode::Periodically;
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
        auto* frm = qobject_cast<FormModSim*>(wnd ? wnd->widget() : nullptr);
        if(!frm)
            continue;

        int originId = frm->property(kSplitOriginIdProperty).toInt();
        if(originId <= 0) {
            originId = frm->formId();
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
            auto* candidateFrm = qobject_cast<FormModSim*>(candidate ? candidate->widget() : nullptr);
            if(candidateFrm && !candidateFrm->property(kSplitAutoCloneProperty).toBool()) {
                sourceWnd = candidate;
                break;
            }
        }
        if(!sourceWnd)
            sourceWnd = group.first();

        auto* source = qobject_cast<FormModSim*>(sourceWnd ? sourceWnd->widget() : nullptr);
        if(!source)
            continue;

        if(group.size() > 1)
        {
            QMdiSubWindow* toMove = nullptr;
            for(auto* candidate : group) {
                auto* candidateFrm = qobject_cast<FormModSim*>(candidate ? candidate->widget() : nullptr);
                if(candidateFrm && candidateFrm->property(kSplitAutoCloneProperty).toBool()) {
                    toMove = candidate;
                    break;
                }
            }
            if(!toMove)
                toMove = group.last();

            primary->removeSubWindow(toMove);
            secondary->addSubWindow(toMove, Qt::WindowFlags());
            if(auto* movedFrm = qobject_cast<FormModSim*>(toMove->widget()))
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
        auto* clone = createMdiChildOnArea(cloneId, source->formKind(), secondary, false);
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
        auto* frm = qobject_cast<FormModSim*>(wnd ? wnd->widget() : nullptr);
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
                    for (auto&& frm : _closedForms) {
                        _projectTree->removeForm(frm);
                        delete frm;
                    }
                    _closedForms.clear();
                    while (xml.readNextStartElement()) {
                        FormModSim::FormKind kind;
                        bool isForm = true;
                        if (xml.name() == QLatin1String("FormDataView")) {
                            kind = FormModSim::FormKind::Data;
                        } else if (xml.name() == QLatin1String("FormTrafficView")) {
                            kind = FormModSim::FormKind::Traffic;
                        } else if (xml.name() == QLatin1String("FormScriptView")) {
                            kind = FormModSim::FormKind::Script;
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
                                frm->loadXml(xml);
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
                if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
                    if(frm->windowTitle() == activePrimaryWin)
                    {
                        primary->setActiveSubWindow(wnd);
                        break;
                    }

    if(splitView)
    {
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

    AppPreferences::instance().saveXml(w);

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
            if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
                w.writeAttribute("ActivePrimaryWindow", frm->windowTitle());
    if(isSplitTabbedView())
        if(auto secondary = splitSecondaryArea())
            if(auto wnd = secondary->activeSubWindow())
                if(auto frm = qobject_cast<FormModSim*>(wnd->widget()))
                    w.writeAttribute("ActiveSecondaryWindow", frm->windowTitle());
    w.writeEndElement(); // ViewSettings

    w.writeStartElement("Forms");
    for(auto&& wnd : _mdiArea->subWindowList()) {
        if (auto frm = qobject_cast<FormModSim*>(wnd->widget())) {
            if(frm->property(kSplitAutoCloneProperty).toBool())
                continue;

            const bool onRight = areaOfForm(frm) == splitSecondaryArea();
            frm->setProperty(kFormPanelProperty, onRight ? QLatin1String(kPanelRight) : QLatin1String(kPanelLeft));
            frm->saveXml(w);
            frm->setProperty(kFormPanelProperty, QVariant());
        }
    }
    // Also save forms that are closed (hidden in project tree)
    for (auto&& frm : _closedForms) {
        if (frm) {
            frm->setProperty(kFormPanelProperty, QLatin1String(kPanelLeft));
            frm->saveXml(w);
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
