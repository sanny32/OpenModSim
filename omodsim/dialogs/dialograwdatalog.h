#ifndef DIALOGRAWDATALOG_H
#define DIALOGRAWDATALOG_H

#include <QDialog>
#include <QQueue>
#include "bufferinglistmodel.h"
#include "modbusmultiserver.h"

namespace Ui {
class DialogRawDataLog;
}

struct RawData {
    enum Communication {
        Tx = 0,
        Rx
    };

    Communication Direction;
    QDateTime Time;
    QByteArray Data;
    bool Valid;
};
Q_DECLARE_METATYPE(RawData)

///
/// \brief The RawDataLogModel class
///
class  RawDataLogModel : public BufferingListModel<RawData>
{
    Q_OBJECT

public:
    explicit RawDataLogModel(QObject* parent = nullptr);
    QVariant data(const QModelIndex& index, int role) const override;
};

///
/// \brief The DialogRawDataLog class
///
class DialogRawDataLog : public QDialog
{
    Q_OBJECT

public:
    explicit DialogRawDataLog(const ModbusMultiServer& server, QWidget *parent = nullptr);
    ~DialogRawDataLog();

    LogViewState logViewState() const;
    void setLogViewState(LogViewState state);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_connected(const ConnectionDetails& cd);
    void on_rawDataReceived(const ConnectionDetails& cd, const QDateTime& time, const QByteArray& data);
    void on_rawDataSended(const ConnectionDetails& cd, const QDateTime& time, const QByteArray& data);
    void on_customContextMenuRequested(const QPoint &pos);
    void on_comboBoxServers_currentIndexChanged(int index);
    void on_comboBoxRowLimit_currentTextChanged(const QString& text);
    void on_pushButtonPause_clicked();
    void on_pushButtonClear_clicked();
    void on_pushButtonExport_clicked();


private:
    void comboBoxServers_addConnection(const ConnectionDetails& cd);

private:
    Ui::DialogRawDataLog *ui;
    QAction* _copyAct;
    QAction* _copyBytesAct;
};

#endif // DIALOGRAWDATALOG_H
