#ifndef MAINSTATUSBAR_H
#define MAINSTATUSBAR_H

#include <QLabel>
#include <QStatusBar>
#include <QToolButton>
#include "modbusmultiserver.h"
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

    UpdateChecker* updateChecker() const { return _updateChecker; }
    void setCheckForUpdates(bool enabled);

protected:
    void changeEvent(QEvent* event) override;

private:
    void updateConnectionInfo(QLabel* label, const ConnectionDetails& cd);
    void onNewVersionAvailable(const QString& version, const QString& url);

private:
    QList<QLabel*> _labels;
    QToolButton* _bellButton;
    UpdateChecker* _updateChecker;
};

#endif // MAINSTATUSBAR_H
