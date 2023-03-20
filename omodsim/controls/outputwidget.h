#ifndef OUTPUTWIDGET_H
#define OUTPUTWIDGET_H

#include <QFile>
#include <QWidget>
#include <QListWidgetItem>
#include <QModbusReply>
#include "enums.h"
#include "datasimulator.h"
#include "displaydefinition.h"

namespace Ui {
class OutputWidget;
}

///
/// \brief The OutputWidget class
///
class OutputWidget : public QWidget
{
    Q_OBJECT

public:  
    explicit OutputWidget(QWidget *parent = nullptr);
    ~OutputWidget();

    QVector<quint16> data() const;

    void setup(const DisplayDefinition& dd,const ModbusSimulationMap& simulations, const QModbusDataUnit& data = QModbusDataUnit());

    DisplayMode displayMode() const;
    void setDisplayMode(DisplayMode mode);

    DataDisplayMode dataDisplayMode() const;
    void setDataDisplayMode(DataDisplayMode mode);

    const ByteOrder& byteOrder() const;
    void setByteOrder(ByteOrder order);

    bool displayHexAddresses() const;
    void setDisplayHexAddresses(bool on);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QColor statusColor() const;
    void setStatusColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    void setStatus(const QString& status);
    void setNotConnectedStatus();
    void setInvalidLengthStatus();

    void paint(const QRect& rc, QPainter& painter);

    void updateTraffic(const QModbusRequest& request, int server);
    void updateTraffic(const QModbusResponse& response, int server);
    void updateData(const QModbusDataUnit& data);

    void setSimulated(QModbusDataUnit::RegisterType type, quint16 addr, bool on);

signals:
    void itemDoubleClicked(quint16 address, const QVariant& value);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

private:
    void updateDataWidget(const QModbusDataUnit& data);
    void updateTrafficWidget(bool request, int server, const QModbusPdu& pdu);
    const QIcon& listWidgetItemIcon(QModbusDataUnit::RegisterType type, quint16 addr) const;

private:
    Ui::OutputWidget *ui;

private:
    bool _displayHexAddreses;
    DisplayMode _displayMode;
    DataDisplayMode _dataDisplayMode;
    ByteOrder _byteOrder;
    DisplayDefinition _displayDefinition;
    QModbusDataUnit _lastData;
    QFile _fileCapture;
    QIcon _iconPointGreen;
    QIcon _iconPointEmpty;
    QMap<QPair<QModbusDataUnit::RegisterType, quint16>, bool> _simulatedItems;
};

#endif // OUTPUTWIDGET_H
