#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTranslator>
#include <QWidgetAction>
#include "formmodsim.h"
#include "ansimenu.h"
#include "modbusmultiserver.h"
#include "windowactionlist.h"
#include "recentfileactionlist.h"

namespace Ui {
class MainWindow;
}

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

    void loadConfig(const QString& filename, bool startup = false);
    void saveConfig(const QString& filename, SerializationFormat format);

signals:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void search(const QString& text);

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject * obj, QEvent * e) override;

private slots:
    void on_awake();

    /* File menu slots */
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionClose_triggered();
    void on_actionCloseAll_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionSaveTestConfig_triggered();
    void on_actionRestoreTestConfig_triggered();
    void on_actionPrint_triggered();
    void on_actionPrintSetup_triggered();
    void on_actionExit_triggered();

    /* Edit menu slots */
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionCut_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionSelectAll_triggered();

    /* Connection menu slots */
    void on_connectAction(ConnectionDetails& cd);
    void on_disconnectAction(ConnectionType type, const QString& port);
    void on_actionMbDefinitions_triggered();

    /* Setup menu slots*/
    void on_actionDataDefinition_triggered();
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

    /* View menu slots */
    void on_actionTabbedView_triggered();
    void on_actionToolbar_triggered();
    void on_actionStatusBar_triggered();
    void on_actionDisplayBar_triggered();
    void on_actionScriptBar_triggered();
    void on_actionEditBar_triggered();
    void on_actionBackground_triggered();
    void on_actionForeground_triggered();
    void on_actionStatus_triggered();
    void on_actionFont_triggered();

    /* Language menu slots */
    void on_actionEnglish_triggered();
    void on_actionRussian_triggered();
    void on_actionChineseCN_triggered();
    void on_actionChineseTW_triggered();

    /* Window menu slots */
    void on_actionCascade_triggered();
    void on_actionTile_triggered();
    void on_actionWindows_triggered();

    /* Help menu slots */
    void on_actionAbout_triggered();

    /* Script menu slots */
    void on_actionRunScript_triggered();
    void on_actionStopScript_triggered();
    void on_actionScriptSettings_triggered();

    void on_runModeChanged(RunMode mode);
    void on_searchText(const QString& text);

    void on_connectionError(const QString& error);

    void updateMenuWindow();
    void openFile(const QString& filename);
    void windowActivate(QMdiSubWindow* wnd);
    void setCodepage(const QString& name);

private:
    void addRecentFile(const QString& filename);
    void updateDataDisplayMode(DataDisplayMode mode);

    void forceCoils(QModbusDataUnit::RegisterType type);
    void presetRegs(QModbusDataUnit::RegisterType type);

    FormModSim* createMdiChild(int id);
    FormModSim* currentMdiChild() const;
    FormModSim* findMdiChild(int id) const;
    FormModSim* firstMdiChild() const;

    FormModSim* loadMdiChild(const QString& filename);
    void saveMdiChild(FormModSim* frm, SerializationFormat format);
    void closeMdiChild(FormModSim* frm);

    bool loadProfile(const QString& filename);
    void saveProfile();

    void saveAs(FormModSim* frm, SerializationFormat format);

private:
    Ui::MainWindow *ui;
    QWidgetAction* _actionRunMode;
    QWidgetAction* _actionSearch;

    QString _lang;
    QTranslator _qtTranslator;
    QTranslator _appTranslator;

private:
    int _windowCounter;
    bool _useSession;

    ModbusMultiServer _mbMultiServer;

    AnsiMenu* _ansiMenu;
    WindowActionList* _windowActionList;
    RecentFileActionList* _recentFileActionList;
    QSharedPointer<QPrinter> _selectedPrinter;
    QSharedPointer<DataSimulator> _dataSimulator;
    QString _savePath;
    QString _profile;
};

#endif // MAINWINDOW_H
