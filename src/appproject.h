#ifndef APPPROJECT_H
#define APPPROJECT_H

#include <QObject>
#include <QList>
#include <QMdiArea>
#include <QUuid>
#include "modbusmultiserver.h"
#include "datasimulator.h"
#include "formdataview.h"
#include "formtrafficview.h"
#include "formscriptview.h"

class MdiAreaEx;
class MdiArea;
class QMdiSubWindow;
class ProjectTreeWidget;
class MainWindow;

enum class ProjectFormKind
{
    Data = 0,
    Traffic,
    Script
};

///
/// \brief The AppProject class manages the project state:
/// forms, scripts, counters, split-view state, and project file I/O.
///
class AppProject : public QObject
{
    Q_OBJECT

public:
    // Shared constants for split-view runtime state
    static constexpr const char* kSplitScriptRunning = "SplitScriptRunning";

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
    QWidget* findMdiChild(QUuid id) const;
    QWidget* findMdiChildInArea(MdiArea* area, QUuid id) const;
    QWidget* firstMdiChild() const;
    QWidget* resolveFormForActiveArea(QWidget* primaryForm) const;
    bool        cloneMdiChildState(QWidget* source, QWidget* target) const;

    // Split-view
    MdiArea*    secondaryArea() const;
    bool        isSplitTabbedView() const;
    bool        isScriptRunningOnSplitPair(QWidget* frm) const;
    void        updateSplitPairScriptIcons(QWidget* frm);
    int         duplicatePrimaryTabsToSecondary();
    void        removeSplitAutoClonesFromSecondary();
    void        resetSplitViewIfEmpty();

    // Project I/O
    void loadProject(const QString& filename);
    void saveProject(const QString& filename);

    // Called from MainWindow::~MainWindow() before delete ui.
    // Closes MDI windows and deletes forms/scripts without touching the project tree UI
    // (avoids dangling QTextDocument pointers in QPlainTextEdit during QObject cleanup).
    void destroyContentForShutdown();

    // Accessors for MainWindow (loadAppSettings/saveAppSettings/eventFilter)
    const QList<QWidget*>& closedForms() const { return _closedForms; }
    bool isFormClosed(QWidget* frm) const;
    QString savePath() const        { return _savePath; }
    void    setSavePath(const QString& p) { _savePath = p; }
    int     nextFormDisplayNumber(ProjectFormKind kind);

private:
    void setupMdiChild(QWidget* frm, QMdiSubWindow* wnd, bool addToWindowList);
    QWidget* createCloneOnArea(QWidget* source, MdiArea* area);
    MdiArea* activeCreateArea() const;
    MdiArea* areaOfForm(QWidget* frm) const;
    void addClosedForm(QWidget* frm);
    void removeClosedForm(QWidget* frm);
    bool containsClosedForm(QWidget* frm) const;

private:
    MdiAreaEx*                    _mdiArea;
    ModbusMultiServer&            _mbServer;
    DataSimulator*                _dataSimulator;
    ProjectTreeWidget*            _projectTree;
    MainWindow*                   _mainWindow;

    int                _dataCounter = 0;
    int                _trafficCounter = 0;
    int                _scriptCounter = 0;
    QList<QWidget*>      _closedForms;
    QString            _savePath;
};

#endif // APPPROJECT_H
