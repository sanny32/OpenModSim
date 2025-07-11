#ifndef OUTPUTWIDGET_H
#define OUTPUTWIDGET_H

#include <QFile>
#include <QWidget>
#include <QListWidgetItem>
#include <QModbusReply>
#include "enums.h"
#include "modbusmessage.h"
#include "datasimulator.h"
#include "displaydefinition.h"

namespace Ui {
class OutputWidget;
}

class OutputWidget;
typedef QMap<QPair<QModbusDataUnit::RegisterType, quint16>, QString> AddressDescriptionMap;

///
/// \brief The OutputListModel class
///
class OutputListModel : public QAbstractListModel
{
    Q_OBJECT

    friend class OutputWidget;

public:
    explicit OutputListModel(OutputWidget* parent);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool isValid() const;
    QVector<quint16> values() const;

    void clear();
    void update();
    void updateData(const QModbusDataUnit& data);

    QModelIndex find(QModbusDataUnit::RegisterType type, quint16 addr) const;

private:
    struct ItemData
    {
        quint32 Address = 0;
        QVariant Value;
        QString ValueStr;
        QString Description;
        bool Simulated = false;
    };

    OutputWidget* _parentWidget;
    QModbusDataUnit _lastData;
    QIcon _iconPointGreen;
    QIcon _iconPointEmpty;
    QMap<int, ItemData> _mapItems;
};


///
/// \brief The OutputWidget class
///
class OutputWidget : public QWidget
{
    Q_OBJECT

    friend class OutputListModel;

public:  
    explicit OutputWidget(QWidget *parent = nullptr);
    ~OutputWidget();

    QVector<quint16> data() const;

    void setup(const DisplayDefinition& dd,const ModbusSimulationMap& simulations, const QModbusDataUnit& data);

    DisplayMode displayMode() const;
    void setDisplayMode(DisplayMode mode);

    DataDisplayMode dataDisplayMode() const;
    void setDataDisplayMode(DataDisplayMode mode);

    const ByteOrder* byteOrder() const;
    void setByteOrder(ByteOrder order);

    QString codepage() const;
    void setCodepage(const QString& name);

    bool displayHexAddresses() const;
    void setDisplayHexAddresses(bool on);

    CaptureMode captureMode() const;
    void startTextCapture(const QString& file);
    void stopTextCapture();

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QColor statusColor() const;
    void setStatusColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    int logViewLimit() const;
    void setLogViewLimit(int l);

    void setStatus(const QString& status);
    void setNotConnectedStatus();
    void setInvalidLengthStatus();

    void paint(const QRect& rc, QPainter& painter);

    void updateTraffic(const QModbusRequest& request, int server, int transactionId, ModbusMessage::ProtocolType protocol);
    void updateTraffic(const QModbusResponse& response, int server, int transactionId, ModbusMessage::ProtocolType protocol);
    void updateData(const QModbusDataUnit& data);

    AddressDescriptionMap descriptionMap() const;
    void setDescription(QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc);

    void setSimulated(QModbusDataUnit::RegisterType type, quint16 addr, bool on);

signals:
    void itemDoubleClicked(quint16 address, const QVariant& value);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_listView_doubleClicked(const QModelIndex& index);
    void on_listView_customContextMenuRequested(const QPoint &pos);

private:
    void captureString(const QString& s);
    void showModbusMessage(const QModelIndex& index);
    void updateLogView(bool request, int deviceId, int transactionId, ModbusMessage::ProtocolType protocol, const QModbusPdu& pdu);

private:
    Ui::OutputWidget *ui;

private:
    bool _displayHexAddreses;
    DisplayMode _displayMode;
    DataDisplayMode _dataDisplayMode;
    ByteOrder _byteOrder;
    QString _codepage;
    DisplayDefinition _displayDefinition;
    QFile _fileCapture;
    AddressDescriptionMap _descriptionMap;
    QSharedPointer<OutputListModel> _listModel;
};

#endif // OUTPUTWIDGET_H
