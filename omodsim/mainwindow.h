#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "formmodsim.h"
#include "modbusmultiserver.h"
#include "windowactionlist.h"
#include "recentfileactionlist.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject * obj, QEvent * e) override;

private slots:
    void on_awake();

    /* File menu slots */
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionClose_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionSaveTestConfig_triggered();
    void on_actionRestoreTestConfig_triggered();
    void on_actionPrint_triggered();
    void on_actionPrintSetup_triggered();
    void on_actionExit_triggered();

    /* Connection menu slots */
    void on_connectAction(ConnectionType type, const QString& port);
    void on_disconnectAction(ConnectionType type, const QString& port);

    /* Setup menu slots*/
    void on_actionDataDefinition_triggered();
    void on_actionShowData_triggered();
    void on_actionShowTraffic_triggered();
    void on_actionBinary_triggered();
    void on_actionUnsignedDecimal_triggered();
    void on_actionInteger_triggered();
    void on_actionHex_triggered();
    void on_actionFloatingPt_triggered();
    void on_actionSwappedFP_triggered();
    void on_actionDblFloat_triggered();
    void on_actionSwappedDbl_triggered();
    void on_actionHexAddresses_triggered();

    /* View menu slots */
    void on_actionToolbar_triggered();
    void on_actionStatusBar_triggered();
    void on_actionDisplayBar_triggered();
    void on_actionBackground_triggered();
    void on_actionForeground_triggered();
    void on_actionStatus_triggered();
    void on_actionFont_triggered();

    /* Window menu slots */
    void on_actionCascade_triggered();
    void on_actionTile_triggered();
    void on_actionWindows_triggered();

    /* Help menu slots */
    void on_actionAbout_triggered();

    void on_connectionError(const QString& error);

    void updateMenuWindow();
    void openFile(const QString& filename);
    void windowActivate(QMdiSubWindow* wnd);

private:
    void addRecentFile(const QString& filename);
    void updateDataDisplayMode(DataDisplayMode mode);

    FormModSim* createMdiChild(int id);
    FormModSim* currentMdiChild() const;
    FormModSim* findMdiChild(int id) const;
    FormModSim* firstMdiChild() const;

    FormModSim* loadMdiChild(const QString& filename);
    void saveMdiChild(FormModSim* frm);

    void loadConfig(const QString& filename);
    void saveConfig(const QString& filename);

    void loadSettings();
    void saveSettings();

private:
    Ui::MainWindow *ui;

private:
    int _windowCounter;

    ModbusMultiServer _mbMultiServer;
    WindowActionList* _windowActionList;
    RecentFileActionList* _recentFileActionList;
    QSharedPointer<QPrinter> _selectedPrinter;
};

#endif // MAINWINDOW_H
