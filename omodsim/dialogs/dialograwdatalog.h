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

private slots:
    void on_rawDataReceived(const ConnectionDetails& cd, const QDateTime& time, const QByteArray& data);
    void on_rawDataSended(const ConnectionDetails& cd, const QDateTime& time, const QByteArray& data);
    void on_comboBoxServers_currentIndexChanged(int index);
    void on_comboBoxRowLimit_currentTextChanged(const QString& text);
    void on_pushButtonPause_clicked();
    void on_pushButtonClear_clicked();
    void on_pushButtonExport_clicked();

private:
    Ui::DialogRawDataLog *ui;
};

#endif // DIALOGRAWDATALOG_H
