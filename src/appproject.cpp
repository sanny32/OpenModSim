#include <QtWidgets>
#include <QBuffer>
#include <QScopedValueRollback>
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
    auto frm = createMdiChildOnArea(id, kind, _mdiArea->primaryArea(), true);
    if(frm)
        ensureSplitMirrorForForm(frm);

    return frm;
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

    connect(frm, &FormModSim::definitionChanged, _mainWindow, [this, frm]()
    {
        syncSplitPeerDisplayDefinition(frm);
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

    connect(frm, &FormModSim::statisticCtrsReseted, _mainWindow, [this, frm]()
    {
        if(_splitDisableInProgress || !isSplitTabbedView())
            return;

        if(auto peer = splitPeer(frm))
            peer->resetCtrs();
    });

    connect(frm, &FormModSim::statisticLogStateChanged, _mainWindow, [this, frm](LogViewState state)
    {
        if(_splitDisableInProgress || !isSplitTabbedView())
            return;

        if(auto peer = splitPeer(frm))
            peer->setLogViewState(state);
    });

    connect(frm, &FormModSim::consoleMessage, _mainWindow, [this](const QString& source, const QString& text, ConsoleOutput::MessageType type) {
        _mainWindow->showConsoleMessage(source, text, type);
    });

    connect(frm, &FormModSim::scriptRunning, _mainWindow, [this, frm]()
    {
        frm->setProperty(kSplitScriptRunning, true);
        if(auto peer = splitPeer(frm))
            peer->setProperty(kSplitScriptRunning, true);
        updateSplitPairScriptIcons(frm);
    });

    connect(frm, &FormModSim::scriptStopped, _mainWindow, [this, frm]()
    {
        frm->setProperty(kSplitScriptRunning, false);
        if(auto peer = splitPeer(frm))
            peer->setProperty(kSplitScriptRunning, false);
        updateSplitPairScriptIcons(frm);
    });

    connect(frm, &FormModSim::closing, _mainWindow, [this, frm]()
    {
        const int peerId = frm->property(kSplitMirrorPeerId).toInt();
        if(peerId <= 0)
            return;

        if(auto peer = findMdiChild(peerId))
            peer->setProperty(kSplitMirrorPeerId, QVariant());

        frm->setProperty(kSplitMirrorPeerId, QVariant());
    });

    connect(wnd, &QObject::destroyed, _mainWindow, [this]() {
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

    auto area = _mdiArea->primaryArea();
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

    connect(wnd, &QObject::destroyed, _mainWindow, [this]() {
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
/// \brief AppProject::splitPeer
/// \param frm
/// \return
///
FormModSim* AppProject::splitPeer(FormModSim* frm) const
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
    if(_splitDisableInProgress || !isSplitTabbedView())
        return;

    auto secondary = splitSecondaryArea();
    if(!secondary || !secondary->localSubWindowList().isEmpty())
        return;

    _mdiArea->toggleVerticalSplit();
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

    auto peer = splitPeer(frm);
    const bool periodicMode = frm->scriptSettings().Mode == RunMode::Periodically ||
                              (peer && peer->scriptSettings().Mode == RunMode::Periodically);
    const bool running = periodicMode && isScriptRunningOnSplitPair(frm);

    applyIcon(frm, running);
    applyIcon(peer, running);
}

///
/// \brief AppProject::ensureSplitMirrorForForm
/// \param frm
///
void AppProject::ensureSplitMirrorForForm(FormModSim* frm)
{
    if(!frm || !isSplitTabbedView())
        return;

    auto secondary = splitSecondaryArea();
    if(!secondary)
        return;

    MdiArea* ownerArea = nullptr;
    for(auto&& wnd : _mdiArea->localSubWindowList()) {
        if(wnd && wnd->widget() == frm) {
            ownerArea = _mdiArea->primaryArea();
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
    MdiArea* targetArea = ownerArea == _mdiArea->primaryArea() ? secondary : _mdiArea->primaryArea();
    if(peerId > 0) {
        if(auto existingPeer = findMdiChildInArea(targetArea, peerId)) {
            if(auto* doc = frm->scriptDocument()) {
                if(doc->parent() != _mainWindow)
                    doc->setParent(_mainWindow);
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

    const bool addToWindowList = (targetArea == _mdiArea->primaryArea());
    auto mirror = createMdiChildOnArea(mirrorId, frm->formKind(), targetArea, addToWindowList);
    if(!mirror)
        return;

    cloneMdiChildState(frm, mirror);
    mirror->setStatisticCounters(frm->requestCount(), frm->responseCount());
    mirror->setLogViewState(frm->logViewState());
    if(auto* doc = frm->scriptDocument()) {
        if(doc->parent() != _mainWindow)
            doc->setParent(_mainWindow);
        mirror->setScriptDocument(doc);
    }
    frm->setProperty(kSplitMirrorPeerId, mirror->formId());
    mirror->setProperty(kSplitMirrorPeerId, frm->formId());
    mirror->setProperty(kSplitScriptRunning, frm->property(kSplitScriptRunning));
    updateSplitPairScriptIcons(frm);
    mirror->show();
}

///
/// \brief AppProject::syncSplitPeerDisplayDefinition
/// \param frm
///
void AppProject::syncSplitPeerDisplayDefinition(FormModSim* frm)
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
/// \brief AppProject::syncSplitPeerState
/// \param frm
///
void AppProject::syncSplitPeerState(FormModSim* frm)
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
/// \brief AppProject::syncSplitForms
///
void AppProject::syncSplitForms()
{
    if(!isSplitTabbedView())
        return;

    auto secondary = splitSecondaryArea();
    if(!secondary)
        return;

    QList<FormModSim*> primaryForms;
    for(auto&& wnd : _mdiArea->localSubWindowList()) {
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
/// \brief AppProject::clearSplitMirrorsFromSecondary
///
void AppProject::clearSplitMirrorsFromSecondary()
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
        auto peer = findMdiChildInArea(_mdiArea->primaryArea(), peerId);
        if(!peer)
            continue;

        peer->setProperty(kSplitMirrorPeerId, QVariant());
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
                            auto frm = createMdiChild(++_windowCounter, kind);
                            if (frm) {
                                frm->loadXml(xml);
                                syncSplitPeerState(frm);
                                frm->show();
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

    _mainWindow->setViewMode(viewMode);
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
        _mdiArea->setSplitViewEnabled(true);

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
    for(auto&& wnd : _mdiArea->localSubWindowList()) {
        if (auto frm = qobject_cast<FormModSim*>(wnd->widget()))
            frm->saveXml(w);
    }
    // Also save forms that are closed (hidden in project tree)
    for (auto&& frm : _closedForms) {
        if (frm)
            frm->saveXml(w);
    }
    w.writeEndElement(); // Forms

    if(isSplitTabbedView())
    {
        w.writeStartElement("SecondaryPanel");
        for(auto&& wnd : _mdiArea->localSubWindowList())
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
/// \brief AppProject::destroyContentForShutdown
///
void AppProject::destroyContentForShutdown()
{
    closeProject();
}
