#ifndef APPPROJECT_H
#define APPPROJECT_H

#include <QObject>
#include <QList>
#include <QMdiArea>
#include "modbusmultiserver.h"
#include "datasimulator.h"
#include "formmodsim.h"

class MdiAreaEx;
class MdiArea;
class QMdiSubWindow;
class ProjectTreeWidget;
class WindowActionList;
class MainWindow;

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
                        WindowActionList* windowActionList,
                        MainWindow* mainWindow,
                        QObject* parent = nullptr);
    ~AppProject() override;

    // Project lifecycle
    void closeProject();
    void markFormClosed(FormModSim* frm);

    // Forms
    FormModSim* createMdiChild(int id, FormModSim::FormKind kind = FormModSim::FormKind::Data);
    FormModSim* createMdiChildOnArea(int id, FormModSim::FormKind kind, MdiArea* area, bool addToWindowList);
    void        rewrapMdiChild(FormModSim* frm);
    void        closeMdiChild(FormModSim* frm);
    void        deleteForm(FormModSim* frm);

    FormModSim* currentMdiChild() const;
    FormModSim* findMdiChild(int id) const;
    FormModSim* findMdiChildInArea(MdiArea* area, int id) const;
    FormModSim* firstMdiChild() const;
    bool        cloneMdiChildState(FormModSim* source, FormModSim* target) const;

    // Split-view
    MdiArea*    splitSecondaryArea() const;
    bool        isSplitTabbedView() const;
    bool        isScriptRunningOnSplitPair(FormModSim* frm) const;
    void        updateSplitPairScriptIcons(FormModSim* frm);
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

    // Accessors for MainWindow (loadProfile/saveProfile/eventFilter)
    const QList<FormModSim*>&     closedForms() const { return _closedForms; }
    QString savePath() const        { return _savePath; }
    void    setSavePath(const QString& p) { _savePath = p; }
    int     windowCounter() const   { return _windowCounter; }
    void    setWindowCounter(int v) { _windowCounter = v; }

private:
    void setupMdiChild(FormModSim* frm, QMdiSubWindow* wnd, bool addToWindowList);
    MdiArea* activeCreateArea() const;
    MdiArea* areaOfForm(FormModSim* frm) const;

private:
    MdiAreaEx*                    _mdiArea;
    ModbusMultiServer&            _mbServer;
    DataSimulator*                _dataSimulator;
    ProjectTreeWidget*            _projectTree;
    WindowActionList*             _windowActionList;
    MainWindow*                   _mainWindow;

    int                _windowCounter = 0;
    QList<FormModSim*> _closedForms;
    QString            _savePath;
};

#endif // APPPROJECT_H
