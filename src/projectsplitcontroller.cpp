// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectsplitcontroller.cpp
/// \brief Implements project split-view coordination.
///

#include "projectsplitcontroller.h"

#include <QApplication>
#include <QBuffer>
#include <QMdiSubWindow>
#include <QPointer>
#include <QTimer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "controls/mdiareaex.h"
#include "formdataview.h"
#include "formscriptview.h"
#include "formtrafficview.h"
#include "projectformmanager.h"
#include "projectformmetadata.h"
#include "projectformxml.h"
#include "themedicons.h"
#include "uiutils.h"

namespace {
constexpr const char* kPanelRight = "R";
}

/// \brief Creates a controller for an MDI workspace and form manager.
ProjectSplitController::ProjectSplitController(MdiAreaEx* mdiArea,
                                               ProjectFormManager* forms,
                                               QObject* parent)
    : QObject(parent)
    , _mdiArea(mdiArea)
    , _forms(forms)
{
    Q_ASSERT(_mdiArea);
    Q_ASSERT(_forms);
}

/// \brief Returns the panel on which a new form should be opened.
MdiArea* ProjectSplitController::activeCreateArea() const
{
    auto* primary = _mdiArea->primaryArea();
    if (!primary || !isSplitTabbedView())
        return primary;

    auto* secondary = secondaryArea();
    if (!secondary)
        return primary;

    if (auto* focus = QApplication::focusWidget()) {
        if (secondary->isAncestorOf(focus))
            return secondary;
        if (primary->isAncestorOf(focus))
            return primary;
    }
    return _mdiArea->activePanel();
}

/// \brief Returns the panel containing a form.
MdiArea* ProjectSplitController::areaOfForm(QWidget* form) const
{
    if (!form)
        return nullptr;
    auto* window = qobject_cast<QMdiSubWindow*>(form->parentWidget());
    if (!window)
        return nullptr;
    auto* primary = _mdiArea->primaryArea();
    if (primary && primary->localSubWindowList().contains(window))
        return primary;
    auto* secondary = secondaryArea();
    return secondary && secondary->localSubWindowList().contains(window) ? secondary : nullptr;
}

/// \brief Returns the secondary panel when it exists.
MdiArea* ProjectSplitController::secondaryArea() const
{
    return _mdiArea->secondaryArea();
}

/// \brief Returns whether tabbed split view is active.
bool ProjectSplitController::isSplitTabbedView() const
{
    return _mdiArea->viewMode() == QMdiArea::TabbedView
        && _mdiArea->isSplitView() && secondaryArea();
}

/// \brief Resolves a canonical form to the peer on the active panel.
QWidget* ProjectSplitController::resolveForActiveArea(QWidget* canonicalForm) const
{
    if (!canonicalForm || !isSplitTabbedView() || activeCreateArea() != secondaryArea())
        return canonicalForm;

    const auto originId = projectFormId(canonicalForm);
    for (auto* window : secondaryArea()->localSubWindowList()) {
        auto* form = window ? window->widget() : nullptr;
        if (form && isSplitClone(form) && splitOriginId(form) == originId)
            return form;
    }
    return canonicalForm;
}

/// \brief Copies serialized state between compatible forms.
bool ProjectSplitController::cloneState(QWidget* source, QWidget* target) const
{
    if (!source || !target)
        return false;
    QByteArray xml;
    QBuffer writeBuffer(&xml);
    if (!writeBuffer.open(QIODevice::WriteOnly))
        return false;
    QXmlStreamWriter writer(&writeBuffer);
    writer.writeStartDocument();
    saveXmlOfForm(source, writer);
    writer.writeEndDocument();
    writeBuffer.close();

    QBuffer readBuffer(&xml);
    if (!readBuffer.open(QIODevice::ReadOnly))
        return false;
    QXmlStreamReader reader(&readBuffer);
    if (!reader.readNextStartElement())
        return false;
    loadCurrentXmlOfForm(target, reader);
    return !reader.hasError();
}

/// \brief Creates a transient visual clone on an MDI area.
QWidget* ProjectSplitController::createClone(QWidget* source, MdiArea* area)
{
    if (!source || !area)
        return nullptr;
    bool valid = false;
    const auto kind = projectFormKindFromWidget(source, &valid);
    if (!valid)
        return nullptr;

    auto* clone = _forms->create(kind, area, false);
    if (!clone)
        return nullptr;
    cloneState(source, clone);
    clone->setWindowTitle(source->windowTitle());
    clone->setWindowIcon(source->windowIcon());
    clone->setFont(source->font());
    setSplitOriginId(clone, splitOriginId(source));
    setSplitClone(clone, true);
    clone->setProperty(ProjectFormMetadata::SplitScriptRunning,
                       source->property(ProjectFormMetadata::SplitScriptRunning));

    connect(source, &QWidget::windowTitleChanged, clone, [clone](const QString& title) {
        if (clone->windowTitle() != title)
            clone->setWindowTitle(title);
    });
    connect(source, &QWidget::windowIconChanged, clone, &QWidget::setWindowIcon);

    if (auto* sourceScript = qobject_cast<FormScriptView*>(source)) {
        if (auto* cloneScript = qobject_cast<FormScriptView*>(clone)) {
            cloneScript->setScriptDocument(sourceScript->scriptDocument());
            cloneScript->setScriptSettings(sourceScript->scriptSettings());
            connect(sourceScript, &FormScriptView::scriptSettingsChanged,
                    cloneScript, &FormScriptView::setScriptSettings);
            connect(cloneScript, &FormScriptView::scriptSettingsChanged,
                    sourceScript, &FormScriptView::setScriptSettings);
            connect(sourceScript, &QWidget::windowTitleChanged, cloneScript,
                    [cloneScript](const QString& title) {
                if (cloneScript->windowTitle() != title)
                    cloneScript->setFormName(title);
            });
            cloneScript->linkRunStopTo(sourceScript);
        }
    }
    if (auto* sourceTraffic = qobject_cast<FormTrafficView*>(source)) {
        if (auto* cloneTraffic = qobject_cast<FormTrafficView*>(clone))
            cloneTraffic->linkTo(sourceTraffic);
    }
    if (auto* sourceData = qobject_cast<FormDataView*>(source)) {
        if (auto* cloneData = qobject_cast<FormDataView*>(clone))
            cloneData->linkTo(sourceData);
    }

    clone->show();
    if (auto* window = qobject_cast<QMdiSubWindow*>(clone->parentWidget())) {
        if (area->viewMode() == QMdiArea::TabbedView)
            window->showMaximized();
    }
    return clone;
}

/// \brief Opens or activates a form on the active panel.
void ProjectSplitController::openOnActivePanel(QWidget* form)
{
    if (!form)
        return;
    auto* panel = _mdiArea->activePanel();
    const auto originId = splitOriginId(form);
    if (panel) {
        for (auto* window : panel->localSubWindowList()) {
            auto* candidate = window ? window->widget() : nullptr;
            if (candidate == form
                || (candidate && isSplitClone(candidate) && splitOriginId(candidate) == originId)) {
                _mdiArea->setActiveSubWindow(window);
                return;
            }
        }
    }
    if (_forms->isClosed(form)) {
        _forms->reopen(form, activeCreateArea());
        return;
    }
    if (isSplitTabbedView() && panel) {
        if (auto* clone = createClone(form, panel)) {
            if (auto* window = qobject_cast<QMdiSubWindow*>(clone->parentWidget()))
                _mdiArea->setActiveSubWindow(window);
            return;
        }
    }
    for (auto* window : _mdiArea->subWindowList()) {
        if (window && window->widget() == form) {
            _mdiArea->setActiveSubWindow(window);
            return;
        }
    }
}

/// \brief Returns whether a form can move to the opposite panel.
bool ProjectSplitController::canMoveToOtherPanel(QWidget* form) const
{
    return form && isSplitTabbedView() && !isSplitClone(form) && areaOfForm(form);
}

/// \brief Moves a canonical form to the opposite panel.
void ProjectSplitController::moveToOtherPanel(QWidget* form, QPoint globalDropPos)
{
    if (!canMoveToOtherPanel(form))
        return;
    auto* source = areaOfForm(form);
    auto* target = source == _mdiArea->primaryArea() ? secondaryArea() : _mdiArea->primaryArea();
    if (!target)
        return;

    const auto originId = splitOriginId(form);
    for (auto* window : target->localSubWindowList()) {
        auto* candidate = window ? window->widget() : nullptr;
        if (candidate && isSplitClone(candidate) && splitOriginId(candidate) == originId) {
            window->close();
            break;
        }
    }
    if (auto* window = qobject_cast<QMdiSubWindow*>(form->parentWidget()))
        _mdiArea->moveSubWindowToOtherPanel(window, globalDropPos);
}

/// \brief Disables split view when the secondary panel becomes empty.
void ProjectSplitController::resetIfEmpty()
{
    if (isSplitTabbedView() && secondaryArea()->localSubWindowList().isEmpty())
        _mdiArea->setSplitViewEnabled(false);
}

/// \brief Returns the shared running state of a script pair.
bool ProjectSplitController::isScriptPairRunning(QWidget* form) const
{
    auto* script = qobject_cast<FormScriptView*>(form);
    return (script && script->canStopScript()) || isSplitScriptRunning(form);
}

/// \brief Updates the running icon of a script peer.
void ProjectSplitController::updateScriptPairIcons(QWidget* form)
{
    auto* script = qobject_cast<FormScriptView*>(form);
    if (!script)
        return;
    auto* window = qobject_cast<QMdiSubWindow*>(form->parentWidget());
    if (!window)
        return;
    const bool running = script->scriptSettings().Mode == RunMode::Periodically
        && isScriptPairRunning(form);
    const auto icon = running ? themedIcon(QStringLiteral("omodsim/run-script")) : form->windowIcon();
    crossFadeWindowIcon(window, window->windowIcon(), icon);
}

/// \brief Places the active primary form on the secondary panel.
int ProjectSplitController::duplicatePrimaryTab()
{
    if (!isSplitTabbedView())
        return 0;
    auto* primary = _mdiArea->primaryArea();
    auto* secondary = secondaryArea();
    if (!primary || !secondary || !secondary->localSubWindowList().isEmpty())
        return 0;
    primary->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);
    secondary->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);

    auto* primaryWindow = _mdiArea->activePrimarySubWindow();
    auto* activeForm = primaryWindow ? primaryWindow->widget() : nullptr;
    if (!activeForm)
        return 0;
    const auto originId = splitOriginId(activeForm);
    setSplitOriginId(activeForm, originId);
    if (!activeForm->property(ProjectFormMetadata::SplitAutoClone).isValid())
        setSplitClone(activeForm, false);

    if (isSplitClone(activeForm)) {
        for (auto* window : primary->localSubWindowList()) {
            auto* candidate = window ? window->widget() : nullptr;
            if (candidate && splitOriginId(candidate) == originId && !isSplitClone(candidate)) {
                activeForm = candidate;
                primaryWindow = window;
                break;
            }
        }
    }

    QMdiSubWindow* secondaryWindow = nullptr;
    for (auto* window : primary->localSubWindowList()) {
        auto* candidate = window ? window->widget() : nullptr;
        if (candidate && splitOriginId(candidate) == originId && isSplitClone(candidate)) {
            primary->removeSubWindow(window);
            secondary->addSubWindow(window, Qt::WindowFlags());
            candidate->show();
            secondaryWindow = window;
            break;
        }
    }
    if (!secondaryWindow) {
        if (auto* clone = createClone(activeForm, secondary))
            secondaryWindow = qobject_cast<QMdiSubWindow*>(clone->parentWidget());
    }
    if (secondaryWindow) {
        if (secondary->viewMode() == QMdiArea::TabbedView && !secondaryWindow->isMaximized())
            secondaryWindow->showMaximized();
        secondary->setActiveSubWindow(secondaryWindow);
    }
    primary->setActiveSubWindow(primaryWindow);
    return secondaryWindow ? 1 : 0;
}

/// \brief Removes transient secondary peers before merging panels.
void ProjectSplitController::removeSecondaryClones()
{
    if (!isSplitTabbedView())
        return;
    const auto windows = secondaryArea()->localSubWindowList();
    for (auto* window : windows) {
        if (window && isSplitClone(window->widget()))
            window->close();
    }
}

/// \brief Stores active-window state parsed from a project.
void ProjectSplitController::setPendingActivation(const QString& primaryTitle,
                                                  const QString& secondaryTitle,
                                                  const QString& panel)
{
    _pendingPrimaryTitle = primaryTitle;
    _pendingSecondaryTitle = secondaryTitle;
    _pendingPanel = panel;
}

/// \brief Applies pending active-window state.
void ProjectSplitController::restoreActiveWindows()
{
    const auto findWindow = [](MdiArea* area, const QString& title) {
        if (!area || title.isEmpty())
            return static_cast<QMdiSubWindow*>(nullptr);
        for (auto* window : area->localSubWindowList()) {
            if (window && window->widget() && window->widget()->windowTitle() == title)
                return window;
        }
        return static_cast<QMdiSubWindow*>(nullptr);
    };

    auto* primaryTarget = findWindow(_mdiArea->primaryArea(), _pendingPrimaryTitle);
    auto* secondaryTarget = findWindow(secondaryArea(), _pendingSecondaryTitle);
    if (primaryTarget)
        _mdiArea->primaryArea()->setActiveSubWindow(primaryTarget);
    if (secondaryTarget)
        secondaryArea()->setActiveSubWindow(secondaryTarget);
    auto* finalTarget = _pendingPanel.compare(QLatin1String(kPanelRight), Qt::CaseInsensitive) == 0
        ? secondaryTarget : primaryTarget;
    if (finalTarget)
        _mdiArea->setActiveSubWindow(finalTarget);

    QPointer<QMdiSubWindow> queuedPrimary = primaryTarget;
    QPointer<QMdiSubWindow> queuedSecondary = secondaryTarget;
    QPointer<QMdiSubWindow> queuedFinal = finalTarget;
    _pendingPrimaryTitle.clear();
    _pendingSecondaryTitle.clear();
    _pendingPanel.clear();
    QTimer::singleShot(0, _mdiArea, [this, queuedPrimary, queuedSecondary, queuedFinal] {
        if (queuedPrimary && _mdiArea->primaryArea())
            _mdiArea->primaryArea()->setActiveSubWindow(queuedPrimary);
        if (queuedSecondary && secondaryArea())
            secondaryArea()->setActiveSubWindow(queuedSecondary);
        if (queuedFinal)
            _mdiArea->setActiveSubWindow(queuedFinal);
    });
}
