// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectformmanager.h
/// \brief Declares project form ownership and MDI lifecycle management.
///

#ifndef PROJECTFORMMANAGER_H
#define PROJECTFORMMANAGER_H

#include <QList>
#include <QObject>
#include <QPointer>
#include <QUuid>

#include "controls/consoleoutput.h"
#include "projectformkind.h"

class DataSimulator;
class FormScriptView;
class MainWindow;
class MdiArea;
class MdiAreaEx;
class ModbusMultiServer;
class ProjectTreeWidget;
class QMdiSubWindow;
class QWidget;

///
/// \brief Owns canonical project forms and manages their MDI lifetime.
///
class ProjectFormManager : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Creates a project form manager for an MDI workspace.
    ///
    ProjectFormManager(MdiAreaEx* mdiArea,
                       ModbusMultiServer& mbServer,
                       DataSimulator* dataSimulator,
                       ProjectTreeWidget* projectTree,
                       MainWindow* mainWindow,
                       QObject* parent = nullptr);

    /// \brief Creates a form on the requested MDI area.
    QWidget* create(ProjectFormKind kind, MdiArea* area, bool registerForm = true);
    /// \brief Reopens a parked canonical form on an MDI area.
    bool reopen(QWidget* form, MdiArea* area);
    /// \brief Closes every MDI window displaying the form.
    void close(QWidget* form);
    /// \brief Permanently removes a canonical form and its split peers.
    void remove(QWidget* form);
    /// \brief Destroys every project form and resets display counters.
    void clear();

    /// \brief Returns all canonical forms, including parked forms.
    QList<QWidget*> forms() const;
    /// \brief Returns canonical forms of a specific kind.
    QList<QWidget*> forms(ProjectFormKind kind) const;
    /// \brief Returns all script forms.
    QList<FormScriptView*> scriptForms() const;
    /// \brief Returns the parked canonical forms.
    const QList<QWidget*>& closedForms() const { return _closedForms; }
    /// \brief Returns whether a canonical form is parked.
    bool isClosed(QWidget* form) const;
    /// \brief Returns the active form widget.
    QWidget* currentForm() const;
    /// \brief Finds an open form by UUID.
    QWidget* find(const QUuid& id) const;
    /// \brief Finds a form by UUID in one MDI area.
    QWidget* findInArea(MdiArea* area, const QUuid& id) const;
    /// \brief Returns the first open form, if any.
    QWidget* firstForm() const;

    /// \brief Parks a canonical form after its MDI window closes.
    void park(QWidget* form);
    /// \brief Returns the next display number for a form kind.
    int nextDisplayNumber(ProjectFormKind kind);

signals:
    void modified();
    void formOpened(QWidget* form);
    void formClosed(QWidget* form);
    void formDeleted(QWidget* form);
    void helpStateUpdateRequested();
    void helpRequested(const QString& key);
    void consoleMessage(const QString& source, const QString& text, ConsoleOutput::MessageType type);
    void outputConsoleRequested();
    void formActivationRequested(QMdiSubWindow* window);
    void splitFormStateChanged(QWidget* form);
    void splitMayBeEmpty();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupForm(QWidget* form, QMdiSubWindow* window, bool registerForm);
    void wireSubWindow(QWidget* form, QMdiSubWindow* window);
    void addClosedForm(QWidget* form);
    void removeClosedForm(QWidget* form);

private:
    QPointer<MdiAreaEx> _mdiArea;
    ModbusMultiServer& _mbServer;
    DataSimulator* _dataSimulator;
    ProjectTreeWidget* _projectTree;
    MainWindow* _mainWindow;
    QList<QWidget*> _closedForms;
    int _dataCounter = 0;
    int _trafficCounter = 0;
    int _scriptCounter = 0;
    int _dataMapCounter = 0;
};

#endif // PROJECTFORMMANAGER_H
