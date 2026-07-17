// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appproject.h
/// \brief Declares the appproject interfaces.
///

#ifndef APPPROJECT_H
#define APPPROJECT_H

#include <QObject>
#include <QList>
#include <QMdiArea>
#include <QPointer>
#include <QUuid>
#include "modbusmultiserver.h"
#include "datasimulator.h"
#include "formdataview.h"
#include "formtrafficview.h"
#include "formscriptview.h"
#include "formdatamapview.h"
#include "projectformkind.h"
#include "projectloadresult.h"

class MdiAreaEx;
class MdiArea;
class QMdiSubWindow;
class ProjectTreeWidget;
class MainWindow;
class ProjectFormManager;
class ProjectSplitController;

///
/// \brief The AppProject class manages the project state:
/// forms, scripts, counters, split-view state, and project file I/O.
///
class AppProject : public QObject
{
    Q_OBJECT

public:
    // Shared constants for split-view runtime state
    explicit AppProject(MdiAreaEx* mdiArea,
                        ModbusMultiServer& mbServer,
                        DataSimulator* dataSimulator,
                        ProjectTreeWidget* projectTree,
                        MainWindow* mainWindow,
                        QObject* parent = nullptr);
    ~AppProject() override;

    // Project lifecycle
    void closeProject();
    void markFormClosed(QWidget* frm);

    // Forms
    QWidget* createMdiChild(ProjectFormKind kind = ProjectFormKind::Data);
    QWidget* createMdiChildOnArea(ProjectFormKind kind, MdiArea* area, bool addToWindowList);
    void        rewrapMdiChild(QWidget* frm);
    void        closeMdiChild(QWidget* frm);
    void        deleteForm(QWidget* frm);
    void        openFormOnActivePanel(QWidget* frm);

    QWidget* currentMdiChild() const;
    FormDataView* currentDataMdiChild() const;
    FormTrafficView* currentTrafficMdiChild() const;
    FormScriptView* currentScriptMdiChild() const;
    FormDataMapView* currentDataMapMdiChild() const;
    QList<QWidget*> forms(ProjectFormKind kind) const;
    QList<FormScriptView*> scriptForms() const;
    QWidget* findMdiChild(QUuid id) const;
    QWidget* findMdiChildInArea(MdiArea* area, QUuid id) const;
    QWidget* firstMdiChild() const;
    QWidget* resolveFormForActiveArea(QWidget* primaryForm) const;
    bool        cloneMdiChildState(QWidget* source, QWidget* target) const;

    // Move tab between panels
    bool        canMoveFormToOtherPanel(QWidget* frm) const;
    void        moveFormToOtherPanel(QWidget* frm, QPoint globalDropPos = QPoint());

    // Split-view
    MdiArea*    secondaryArea() const;
    bool        isSplitTabbedView() const;
    bool        isScriptRunningOnSplitPair(QWidget* frm) const;
    void        updateSplitPairScriptIcons(QWidget* frm);
    int         duplicatePrimaryTabsToSecondary();
    void        removeSplitAutoClonesFromSecondary();
    void        resetSplitViewIfEmpty();

    // Project I/O
    /// \brief Loads or merges a validated project document.
    /// \param filename Project file path.
    /// \return Load status and error text.
    ProjectLoadResult loadProject(const QString& filename);
    /// \brief Validates a project file without changing application state.
    /// \param filename Project file path.
    /// \return Validation status and error text.
    ProjectLoadResult validateProject(const QString& filename) const;
    bool saveProject(const QString& filename);
    void restoreActiveWindows();
    const QString & filePath() const noexcept { return _projectFilename; }

    // Called from MainWindow::~MainWindow() before delete ui.
    // Closes MDI windows and deletes forms/scripts without touching the project tree UI
    // (avoids dangling QTextDocument pointers in QPlainTextEdit during QObject cleanup).
    void destroyContentForShutdown();

    // Accessors for MainWindow (loadAppSettings/saveAppSettings/eventFilter)
    const QList<QWidget*>& closedForms() const;
    bool isFormClosed(QWidget* frm) const;
    QString savePath() const        { return _savePath; }
    void    setSavePath(const QString& p) { _savePath = p; }
    int     nextFormDisplayNumber(ProjectFormKind kind);

signals:
    void modified();
    void helpStateUpdateRequested();
    void helpRequested(const QString& key);
    void consoleMessage(const QString& source, const QString& text, ConsoleOutput::MessageType type);
    void outputConsoleRequested();
    void formActivationRequested(QMdiSubWindow* window);
    void projectOpened(const QString& filename);
    void projectClosed(const QString& filename);
    void projectSaved(const QString& filename);
    void projectSaveFailed(const QString& filename, const QString& error);
    void projectLoadFailed(const QString& filename, const QString& error);
    void formCreated(QWidget* form);
    void formOpened(QWidget* form);
    void formClosed(QWidget* form);
    void formDeleted(QWidget* form);

private:
    QWidget* createCloneOnArea(QWidget* source, MdiArea* area);
    MdiArea* activeCreateArea() const;
    MdiArea* areaOfForm(QWidget* frm) const;
    QList<QWidget*> allProjectForms() const;
    FormDataMapView* findAutoRequestMap() const;
    FormDataMapView* ensureAutoRequestMap();
    void syncAutoRequestMap(const ModbusDefinitions& defs);
    bool containsClosedForm(QWidget* frm) const;

private:
    QPointer<MdiAreaEx>           _mdiArea;
    ModbusMultiServer&            _mbServer;
    DataSimulator*                _dataSimulator;
    ProjectTreeWidget*            _projectTree;
    MainWindow*                   _mainWindow;
    ProjectFormManager*           _formManager;
    ProjectSplitController*       _splitController;
    QString            _savePath;
    QString            _projectFilename;
};

#endif // APPPROJECT_H

