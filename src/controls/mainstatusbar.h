#ifndef MAINSTATUSBAR_H
#define MAINSTATUSBAR_H

#include <QLabel>
#include <QStatusBar>
#include <QToolButton>
#include "modbusmultiserver.h"
#include "modbuserrorsimulations.h"
#include "updatechecker.h"

///
/// \brief The MainStatusBar class
///
class MainStatusBar : public QStatusBar
{
    Q_OBJECT
public:
    explicit MainStatusBar(const ModbusMultiServer& server, QWidget* parent = nullptr);
    ~MainStatusBar();

    void setCheckForUpdates(bool enabled);

protected:
    void changeEvent(QEvent* event) override;

private:
    void updateDeviceIdsInfo();
    void updateConnectionInfo(QLabel* label, const ConnectionDetails& cd);
    void updateErrorSimInfo();
    void updateClientCountInfo();
    void updateRequestCountInfo();
    void promptDownloadNewVersion();
    void onNewVersionAvailable(const QString& version, const QString& url);

private:
    QList<QLabel*> _labels;
    const ModbusMultiServer& _server;
    QLabel* _deviceIdsLabel;
    QLabel* _clientCountLabel;
    QLabel* _errorSimLabel;
    QLabel* _requestCountLabel;
    QList<int> _deviceIds;
    ModbusErrorSimulations _errorSimulations;
    QToolButton* _bellButton;
    UpdateChecker* _updateChecker;
    quint64 _requestCount  = 0;
    quint64 _responseCount = 0;
};

#endif // MAINSTATUSBAR_H

