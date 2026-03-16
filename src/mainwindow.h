#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QTranslator>
#include "helpwidget.h"
#include "formmodsim.h"
#include "ansimenu.h"
#include "modbusmultiserver.h"
#include "windowactionlist.h"
#include "controls/consoleoutput.h"
#include "controls/projecttreewidget.h"
#include "appproject.h"

namespace Ui {
class MainWindow;
}

class MdiAreaEx;
class MdiArea;

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
    void applyFont(const QFont& font);
    void applyScriptFont(const QFont& font);
    void applyZoom(int zoomPercent);
    void applyColors(const QColor& bg, const QColor& fg, const QColor& status);
    void applyDisplayDefaults(const DisplayDefinition& dd);
    void applyCheckForUpdates(bool enabled);

    void loadProject(const QString& filename);
    void saveProject(const QString& filename);

    void selectAnsiCodepage(const QString& name);
    void showConsoleMessage(const QString& source, const QString& text, ConsoleOutput::MessageType type);
    void showHelpContext(const QString& helpKey);
    QIcon runScriptIcon() const;
    void applyConnections(const ModbusDefinitions& defs, const QList<ConnectionDetails>& conns);

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
    void setCodepage(const QString& name);

private slots:
    void on_awake();

    /* File menu slots */
    void on_actionNew_triggered();
    void on_actionNewDataView_triggered();
    void on_actionNewTrafficView_triggered();
    void on_actionClose_triggered();
    void on_actionCloseAll_triggered();
    void on_actionOpenProject_triggered();
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
    void on_actionShowData_triggered();
    void on_actionShowTraffic_triggered();
    void on_actionShowScript_triggered();
    void on_actionBinary_triggered();
    void on_actionUInt16_triggered();
    void on_actionInt16_triggered();
    void on_actionInt32_triggered();
    void on_actionSwappedInt32_triggered();
    void on_actionUInt32_triggered();
    void on_actionSwappedUInt32_triggered();
    void on_actionInt64_triggered();
    void on_actionSwappedInt64_triggered();
    void on_actionUInt64_triggered();
    void on_actionSwappedUInt64_triggered();
    void on_actionHex_triggered();
    void on_actionAnsi_triggered();
    void on_actionFloatingPt_triggered();
    void on_actionSwappedFP_triggered();
    void on_actionDblFloat_triggered();
    void on_actionSwappedDbl_triggered();
    void on_actionSwapBytes_triggered();
    void on_actionHexAddresses_triggered();
    void on_actionForceCoils_triggered();
    void on_actionForceDiscretes_triggered();
    void on_actionPresetInputRegs_triggered();
    void on_actionPresetHoldingRegs_triggered();
    void on_actionMsgParser_triggered();
    void on_actionRawDataLog_triggered();
    void on_actionTextCapture_triggered();
    void on_actionCaptureOff_triggered();
    void on_actionResetCtrs_triggered();

    /* View menu slots */
    void on_actionTabbedView_triggered();
    void on_actionSplitView_triggered();
    void on_actionToolbar_triggered();
    void on_actionStatusBar_triggered();
    void on_actionScriptHelp_triggered();
    void on_actionConsoleOutput_triggered();

    /* Window menu slots */
    void on_actionCascade_triggered();
    void on_actionTile_triggered();
    void on_actionWindows_triggered();

    /* Help menu slots */
    void on_actionAbout_triggered();

    /* Script slots */
    void on_actionNewScript_triggered();
    void on_actionImportScript_triggered();
    void on_actionRunScript_triggered();
    void on_actionStopScript_triggered();

    void on_searchText(const QString& text);

    void on_connectionError(const QString& error);

    void updateMenuWindow();

private:
    void createNewForm(FormModSim::FormKind kind);
    void updateDataDisplayMode(DataDisplayMode mode);

    void forceCoils(QModbusDataUnit::RegisterType type);
    void presetRegs(QModbusDataUnit::RegisterType type);

    bool loadProfile(const QString& filename);
    void saveProfile();

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

    AnsiMenu* _ansiMenu;
    WindowActionList* _windowActionList;
    QSharedPointer<QPrinter> _selectedPrinter;
    DataSimulator* _dataSimulator = nullptr;
    QString _profile;

    AppProject* _project = nullptr;
};

#endif // MAINWINDOW_H
