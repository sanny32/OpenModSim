// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectformmanager.cpp
/// \brief Implements project form ownership and MDI lifecycle management.
///

#include "projectformmanager.h"

#include <QCoreApplication>
#include <QEvent>
#include <QMdiSubWindow>
#include <QTextDocument>

#include "apppreferences.h"
#include "controls/mdiareaex.h"
#include "controls/mditabbar.h"
#include "controls/projecttreewidget.h"
#include "datasimulator.h"
#include "formdatamapview.h"
#include "formdataview.h"
#include "formscriptview.h"
#include "formtrafficview.h"
#include "mainwindow.h"
#include "modbusmultiserver.h"
#include "projectformmetadata.h"
#include "projectformxml.h"

namespace {

ProjectFormType toProjectFormType(ProjectFormKind kind)
{
    switch (kind) {
    case ProjectFormKind::Data: return ProjectFormType::Data;
    case ProjectFormKind::Traffic: return ProjectFormType::Traffic;
    case ProjectFormKind::Script: return ProjectFormType::Script;
    case ProjectFormKind::DataMap: return ProjectFormType::DataMap;
    }
    return ProjectFormType::Data;
}

void enableAutoComplete(QWidget* form, bool enabled)
{
    if (auto* script = qobject_cast<FormScriptView*>(form))
        script->enableAutoComplete(enabled);
}

void connectEditSlots(QWidget* form)
{
    if (auto* traffic = qobject_cast<FormTrafficView*>(form))
        traffic->connectEditSlots();
    else if (auto* script = qobject_cast<FormScriptView*>(form))
        script->connectEditSlots();
}

void disconnectEditSlots(QWidget* form)
{
    if (auto* traffic = qobject_cast<FormTrafficView*>(form))
        traffic->disconnectEditSlots();
    else if (auto* script = qobject_cast<FormScriptView*>(form))
        script->disconnectEditSlots();
}

}

///
/// \brief Creates a project form manager for an MDI workspace.
///
ProjectFormManager::ProjectFormManager(MdiAreaEx* mdiArea,
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
    Q_ASSERT(_mdiArea);
    Q_ASSERT(_dataSimulator);
    Q_ASSERT(_projectTree);
    Q_ASSERT(_mainWindow);
}

/// \brief Returns all canonical forms, including parked forms.
QList<QWidget*> ProjectFormManager::forms() const
{
    QList<QWidget*> result;
    for (auto* window : _mdiArea->subWindowList()) {
        auto* form = window ? window->widget() : nullptr;
        if (form && !isSplitClone(form) && !result.contains(form))
            result.append(form);
    }
    for (auto* form : _closedForms) {
        if (form && !result.contains(form))
            result.append(form);
    }
    return result;
}

/// \brief Returns canonical forms of a specific kind.
QList<QWidget*> ProjectFormManager::forms(ProjectFormKind kind) const
{
    QList<QWidget*> result;
    for (auto* form : forms()) {
        bool valid = false;
        if (projectFormKindFromWidget(form, &valid) == kind && valid)
            result.append(form);
    }
    return result;
}

/// \brief Returns all script forms.
QList<FormScriptView*> ProjectFormManager::scriptForms() const
{
    QList<FormScriptView*> result;
    for (auto* form : forms()) {
        if (auto* script = qobject_cast<FormScriptView*>(form))
            result.append(script);
    }
    return result;
}

/// \brief Returns whether a canonical form is parked.
bool ProjectFormManager::isClosed(QWidget* form) const
{
    return form && _closedForms.contains(form);
}

/// \brief Returns the next display number for a form kind.
int ProjectFormManager::nextDisplayNumber(ProjectFormKind kind)
{
    switch (kind) {
    case ProjectFormKind::Data: return ++_dataCounter;
    case ProjectFormKind::Traffic: return ++_trafficCounter;
    case ProjectFormKind::Script: return ++_scriptCounter;
    case ProjectFormKind::DataMap: return ++_dataMapCounter;
    }
    return 0;
}

/// \brief Creates a form on the requested MDI area.
QWidget* ProjectFormManager::create(ProjectFormKind kind, MdiArea* area, bool registerForm)
{
    if (!area)
        return nullptr;

    QWidget* form = nullptr;
    switch (kind) {
    case ProjectFormKind::Data:
        form = new FormDataView(_mbServer, _dataSimulator, _mainWindow);
        break;
    case ProjectFormKind::Traffic:
        form = new FormTrafficView(_mbServer, _mainWindow);
        break;
    case ProjectFormKind::Script:
        form = new FormScriptView(_mbServer, _dataSimulator, _mainWindow);
        break;
    case ProjectFormKind::DataMap:
        form = new FormDataMapView(_mbServer, _mainWindow);
        break;
    }

    setProjectFormId(form, QUuid::createUuid());
    if (registerForm) {
        const int number = nextDisplayNumber(kind);
        switch (kind) {
        case ProjectFormKind::Data: form->setWindowTitle(QStringLiteral("Data%1").arg(number)); break;
        case ProjectFormKind::Traffic: form->setWindowTitle(QStringLiteral("Traffic%1").arg(number)); break;
        case ProjectFormKind::Script: form->setWindowTitle(QStringLiteral("Script%1").arg(number)); break;
        case ProjectFormKind::DataMap: form->setWindowTitle(QStringLiteral("Map%1").arg(number)); break;
        }
    }

    enableAutoComplete(form, AppPreferences::instance().codeAutoComplete());
    auto* window = area->addSubWindow(form);
    if (!window) {
        form->deleteLater();
        return nullptr;
    }

    window->setAttribute(Qt::WA_DeleteOnClose, true);
    setupForm(form, window, registerForm);
    return form;
}

/// \brief Connects one MDI window to its form and lifecycle handler.
void ProjectFormManager::wireSubWindow(QWidget* form, QMdiSubWindow* window)
{
    window->installEventFilter(this);
    window->setWindowTitle(form->windowTitle());
    window->setWindowIcon(form->windowIcon());
    connect(form, &QWidget::windowTitleChanged, window, &QMdiSubWindow::setWindowTitle);
    connect(form, &QWidget::windowIconChanged, window, &QMdiSubWindow::setWindowIcon);
    connect(window, &QMdiSubWindow::windowStateChanged, window,
            [this, form](Qt::WindowStates, Qt::WindowStates state) {
        switch (state & ~Qt::WindowMaximized & ~Qt::WindowMinimized) {
        case Qt::WindowActive:
            emit helpStateUpdateRequested();
            connectEditSlots(form);
            break;
        case Qt::WindowNoState:
            disconnectEditSlots(form);
            break;
        default:
            break;
        }
    });
    connect(window, &QObject::destroyed, this, &ProjectFormManager::splitMayBeEmpty);
}

/// \brief Performs one-time setup for a newly created form.
void ProjectFormManager::setupForm(QWidget* form, QMdiSubWindow* window, bool registerForm)
{
    wireSubWindow(form, window);

    if (auto* data = qobject_cast<FormDataView*>(form)) {
        connect(data, &FormDataView::pointTypeChanged, data,
                [data](QModbusDataUnit::RegisterType type) {
            switch (type) {
            case QModbusDataUnit::Coils:
            case QModbusDataUnit::DiscreteInputs:
                data->setProperty("PrevDataType", QVariant::fromValue(data->dataType()));
                data->setDataType(DataType::Binary);
                break;
            case QModbusDataUnit::HoldingRegisters:
            case QModbusDataUnit::InputRegisters: {
                const auto previous = data->property("PrevDataType");
                if (previous.isValid())
                    data->setDataType(previous.value<DataType>());
                break;
            }
            default:
                break;
            }
        });
    }

    const auto activateForm = [this, form] {
        for (auto* candidate : _mdiArea->subWindowList()) {
            if (candidate && candidate->widget() == form) {
                emit formActivationRequested(candidate);
                break;
            }
        }
    };
    if (auto* data = qobject_cast<FormDataView*>(form))
        connect(data, &FormDataView::showed, this, activateForm);
    else if (auto* traffic = qobject_cast<FormTrafficView*>(form))
        connect(traffic, &FormTrafficView::showed, this, activateForm);
    else if (auto* script = qobject_cast<FormScriptView*>(form))
        connect(script, &FormScriptView::showed, this, activateForm);
    else if (auto* map = qobject_cast<FormDataMapView*>(form))
        connect(map, &FormDataMapView::showed, this, activateForm);

    if (auto* data = qobject_cast<FormDataView*>(form))
        connect(data, &FormDataView::helpContextRequested, this, &ProjectFormManager::helpRequested);
    else if (auto* script = qobject_cast<FormScriptView*>(form))
        connect(script, &FormScriptView::helpContextRequested, this, &ProjectFormManager::helpRequested);

    if (auto* script = qobject_cast<FormScriptView*>(form)) {
        connect(script, &FormScriptView::consoleMessage, this, &ProjectFormManager::consoleMessage);
        connect(script, &FormScriptView::scriptRunning, this, [this, form] {
            form->setProperty(ProjectFormMetadata::SplitScriptRunning, true);
            _projectTree->setFormScriptRunning(form, true);
            emit splitFormStateChanged(form);
            emit outputConsoleRequested();
        });
        connect(script, &FormScriptView::scriptStopped, this, [this, form] {
            form->setProperty(ProjectFormMetadata::SplitScriptRunning, false);
            _projectTree->setFormScriptRunning(form, false);
            emit splitFormStateChanged(form);
        });
        connect(script, &FormScriptView::scriptSettingsChanged, this,
                [this](const ScriptSettings&) { emit modified(); });
        connect(script->scriptDocument(), &QTextDocument::contentsChanged,
                this, &ProjectFormManager::modified);
    } else if (auto* data = qobject_cast<FormDataView*>(form)) {
        connect(data, &FormDataView::definitionChanged, this, &ProjectFormManager::modified);
    } else if (auto* traffic = qobject_cast<FormTrafficView*>(form)) {
        connect(traffic, &FormTrafficView::definitionChanged, this, &ProjectFormManager::modified);
    } else if (auto* map = qobject_cast<FormDataMapView*>(form)) {
        connect(map, &FormDataMapView::definitionChanged, this, &ProjectFormManager::modified);
    }

    if (registerForm) {
        bool valid = false;
        const auto kind = projectFormKindFromWidget(form, &valid);
        if (valid) {
            _projectTree->addForm(toProjectFormType(kind), form);
            connect(form, &QWidget::windowTitleChanged, _projectTree,
                    [this, form](const QString&) { _projectTree->updateFormTitle(form); });
        }
    }
}

/// \brief Reopens a parked canonical form on an MDI area.
bool ProjectFormManager::reopen(QWidget* form, MdiArea* area)
{
    if (!form || !area || !isClosed(form))
        return false;

    removeClosedForm(form);
    form->setParent(nullptr);
    auto* window = area->addSubWindow(form);
    if (!window) {
        form->setParent(_mainWindow);
        form->hide();
        addClosedForm(form);
        return false;
    }

    window->setAttribute(Qt::WA_DeleteOnClose, true);
    wireSubWindow(form, window);
    _projectTree->setFormOpen(form, true);
    _projectTree->activateForm(form);
    form->show();
    emit formOpened(form);
    return true;
}

/// \brief Closes every MDI window displaying the form.
void ProjectFormManager::close(QWidget* form)
{
    for (auto* window : _mdiArea->subWindowList()) {
        if (window && window->widget() == form)
            window->close();
    }
}

/// \brief Parks a canonical form after its MDI window closes.
void ProjectFormManager::park(QWidget* form)
{
    if (!form || isSplitClone(form) || isClosed(form))
        return;

    if (auto* window = qobject_cast<QMdiSubWindow*>(form->parentWidget()))
        form->removeEventFilter(window);
    form->setParent(_mainWindow);
    form->hide();
    addClosedForm(form);
    _projectTree->setFormOpen(form, false);
    emit formClosed(form);
}

/// \brief Permanently removes a canonical form and its split peers.
void ProjectFormManager::remove(QWidget* form)
{
    if (!form)
        return;

    const QUuid originId = splitOriginId(form);
    QWidget* canonical = isSplitClone(form) ? nullptr : form;
    if (!canonical) {
        for (auto* candidate : forms()) {
            if (!isSplitClone(candidate) && projectFormId(candidate) == originId) {
                canonical = candidate;
                break;
            }
        }
    }
    if (!canonical || isFormDeletionLocked(canonical))
        return;

    emit formDeleted(canonical);
    const auto windows = _mdiArea->subWindowList();
    for (auto* window : windows) {
        auto* candidate = window ? window->widget() : nullptr;
        if (candidate && isSplitClone(candidate) && splitOriginId(candidate) == originId)
            window->close();
    }
    for (auto* window : windows) {
        if (window && window->widget() == canonical) {
            window->close();
            break;
        }
    }

    removeClosedForm(canonical);
    _projectTree->removeForm(canonical);
    delete canonical;
    emit splitMayBeEmpty();
}

/// \brief Destroys every project form and resets display counters.
void ProjectFormManager::clear()
{
    _mdiArea->closeAllSubWindows();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    const auto deleteMatching = [this](auto predicate) {
        const auto snapshot = _closedForms;
        for (auto* form : snapshot) {
            if (!form || !predicate(form))
                continue;
            _closedForms.removeOne(form);
            _projectTree->removeForm(form);
            delete form;
        }
    };
    deleteMatching([](QWidget* form) {
        return projectFormKindFromWidget(form) == ProjectFormKind::DataMap;
    });
    deleteMatching([](QWidget*) { return true; });
    _closedForms.clear();
    _dataCounter = 0;
    _trafficCounter = 0;
    _scriptCounter = 0;
    _dataMapCounter = 0;
}

/// \brief Returns the active form widget.
QWidget* ProjectFormManager::currentForm() const
{
    auto* window = _mdiArea->currentSubWindow();
    if (!window && _mdiArea->viewMode() == QMdiArea::TabbedView) {
        if (const auto* tabBar = qobject_cast<const MdiTabBar*>(_mdiArea->tabBar()))
            window = tabBar->subWindowAt(tabBar->currentIndex());
    }
    return window ? window->widget() : nullptr;
}

/// \brief Finds an open form by UUID.
QWidget* ProjectFormManager::find(const QUuid& id) const
{
    for (auto* window : _mdiArea->subWindowList()) {
        auto* form = window ? window->widget() : nullptr;
        if (form && projectFormId(form) == id)
            return form;
    }
    return nullptr;
}

/// \brief Finds a form by UUID in one MDI area.
QWidget* ProjectFormManager::findInArea(MdiArea* area, const QUuid& id) const
{
    if (!area)
        return nullptr;
    for (auto* window : area->localSubWindowList()) {
        auto* form = window ? window->widget() : nullptr;
        if (form && projectFormId(form) == id)
            return form;
    }
    return nullptr;
}

/// \brief Returns the first open form, if any.
QWidget* ProjectFormManager::firstForm() const
{
    const auto windows = _mdiArea->subWindowList();
    return windows.isEmpty() || !windows.first() ? nullptr : windows.first()->widget();
}

/// \brief Adds a form to the parked registry.
void ProjectFormManager::addClosedForm(QWidget* form)
{
    if (form && !_closedForms.contains(form))
        _closedForms.append(form);
}

/// \brief Removes a form from the parked registry.
void ProjectFormManager::removeClosedForm(QWidget* form)
{
    _closedForms.removeOne(form);
}

/// \brief Handles MDI close and movement events.
bool ProjectFormManager::eventFilter(QObject* watched, QEvent* event)
{
    auto* window = qobject_cast<QMdiSubWindow*>(watched);
    if (!window)
        return QObject::eventFilter(watched, event);

    auto* form = window->widget();
    if (event->type() == QEvent::Close) {
        if (form && !isSplitClone(form)) {
            park(form);
            emit modified();
        }
    } else if (event->type() == QEvent::Move && form
               && !window->isMinimized() && !window->isMaximized()) {
        if (qobject_cast<FormTrafficView*>(form))
            form->setProperty("ParentGeometry", window->geometry());
    }
    return QObject::eventFilter(watched, event);
}
