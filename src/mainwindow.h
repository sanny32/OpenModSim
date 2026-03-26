#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QTranslator>
#include <QHash>
#include "helpwidget.h"
#include "modbusmultiserver.h"
#include "controls/consoleoutput.h"
#include "controls/projecttreewidget.h"
#include "appproject.h"
#include "dialogs/dialogsetuppresetdata.h"

namespace Ui {
class MainWindow;
}

class MdiAreaEx;
class MdiArea;
class FormDataView;
class FormTrafficView;
class FormScriptView;
class QMenu;
class QAction;

///
/// \brief The MainWindow class
///
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& profile, bool useSession, QWidget *parent = nullptr);
    ~MainWindow();

    void setLanguage(const QString& lang);
    void applyAutoComplete(bool enable);
    void applyConsoleMaxLines(int n);
    void applyFont(const QFont& font);
    void applyScriptFont(const QFont& font);
    void applyZoom(int zoomPercent);
    void applyColors(const QColor& bg, const QColor& fg, const QColor& status, const QColor& addr, const QColor& comment);
    void applyCheckForUpdates(bool enabled);

    void loadProject(const QString& filename);
    void saveProject(const QString& filename);

    void selectAnsiCodepage(const QString& name);
    void appendConsoleMessage(const QString& source, const QString& text, ConsoleOutput::MessageType type);
    void showOutputConsole();
    void showHelpContext(const QString& helpKey);
    void applyConnections(const ModbusDefinitions& defs, const QList<ConnectionDetails>& conns);
    ModbusMultiServer& mbMultiServer() { return _mbMultiServer; }
    const ModbusMultiServer& mbMultiServer() const { return _mbMultiServer; }
    DataSimulator* dataSimulator() const { return _dataSimulator; }

    void setViewMode(QMdiArea::ViewMode mode);

signals:
    void selectAll();
    void search(const QString& text);
    void find();
    void replace();

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject * obj, QEvent * e) override;

public slots:
    void windowActivate(QMdiSubWindow* wnd);
    void updateHelpWidgetState();
    void markModified();
private slots:
    void on_awake();

    /* File menu slots */
    void on_actionNew_triggered();
    void on_actionNewDataView_triggered();
    void on_actionNewTrafficView_triggered();
    void on_actionClose_triggered();
    void on_actionCloseAll_triggered();
    void on_actionOpenProject_triggered();
    void on_actionSaveProject_triggered();
    void on_actionSaveProjectAs_triggered();
    void on_actionCloseProject_triggered();
    void on_actionPrint_triggered();
    void on_actionPrintSetup_triggered();
    void on_actionExit_triggered();

    void on_actionPreferences_triggered();

    /* Edit menu slots */
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionCut_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionSelectAll_triggered();
    void on_actionFind_triggered();
    void on_actionReplace_triggered();

    /* Connection menu slots */
    void on_connectAction(ConnectionDetails& cd);
    void on_disconnectAction(ConnectionType type, const QString& port);
    void on_actionMbDefinitions_triggered();

    /* Setup menu slots*/
    void on_actionForceCoils_triggered();
    void on_actionForceDiscretes_triggered();
    void on_actionPresetInputRegs_triggered();
    void on_actionPresetHoldingRegs_triggered();
    void on_actionMsgParser_triggered();

    /* View menu slots */
    void on_actionTabbedView_triggered();
    void on_actionSplitView_triggered();
    void on_actionToolbar_triggered();
    void on_actionStatusBar_triggered();
    void on_actionProjectTree_triggered();
    void on_actionScriptHelp_triggered();
    void on_actionConsoleOutput_triggered();

    /* Window menu slots */
    void on_actionCascade_triggered();
    void on_actionTile_triggered();

    /* Help menu slots */
    void on_actionAbout_triggered();

    /* Script slots */
    void on_actionNewScript_triggered();
    void on_actionImportScript_triggered();

    void on_searchText(const QString& text);

    void on_connectionError(const QString& error);

    void updateMenuWindow();
    void on_tabContextMenuRequested(QMdiSubWindow* subWnd, MdiArea* sourcePanel, QPoint globalPos);
    void on_moveTabToOtherPanelRequested(QMdiSubWindow* subWnd, QPoint globalDropPos);

private:
    QWidget* createNewForm(ProjectFormKind kind);
    QWidget* currentForm() const;
    FormDataView* currentDataForm() const;
    FormTrafficView* currentTrafficForm() const;
    FormScriptView* currentScriptForm() const;
    QWidget* currentDataOrTrafficForm() const;
    void forceCoils(QModbusDataUnit::RegisterType type);
    void presetRegs(QModbusDataUnit::RegisterType type);

    bool loadAppSettings(const QString& filename);
    void saveAppSettings();
    bool confirmSaveOnClose();
    bool hasProjectContext() const;
    void addRecentProject(const QString& filePath);
    void rebuildRecentProjectsMenu();
    void clearRecentProjects();
    void openRecentProject(const QString& filePath);
    void updateProjectWindowTitle();

private:
    Ui::MainWindow *ui;
    QDockWidget* _helpDockWidget;
    HelpWidget* _helpWidget;
    QDockWidget* _consoleDockWidget = nullptr;
    ConsoleOutput* _globalConsole = nullptr;
    QDockWidget* _projectDockWidget = nullptr;
    ProjectTreeWidget* _projectTree = nullptr;
    QString _lang;
    QTranslator _qtTranslator;
    QTranslator _appTranslator;

private:
    bool _useSession;

    ModbusMultiServer _mbMultiServer;

    QSharedPointer<QPrinter> _selectedPrinter;
    DataSimulator* _dataSimulator = nullptr;
    QString _profile;
    QString _projectFilePath;
    ProjectFormKind _newFormKind = ProjectFormKind::Data;
    QStringList _recentProjects;
    QString _lastProjectPath;
    bool _isModified = false;
    QMenu* _openRecentMenu = nullptr;
    QAction* _clearRecentAction = nullptr;
    QHash<QModbusDataUnit::RegisterType, SetupPresetParams> _presetParams;

    AppProject* _project = nullptr;
};

#endif // MAINWINDOW_H
