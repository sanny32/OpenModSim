#ifndef OUTPUTDATAWIDGET_H
#define OUTPUTDATAWIDGET_H

#include <QAbstractListModel>
#include <QPixmap>
#include <QSharedPointer>
#include <QFrame>
#include "datasimulator.h"
#include "displaydefinition.h"
#include "enums.h"
#include "modbusmessage.h"
#include "outputtypes.h"

class QLabel;
class QPainter;
class QTimer;

namespace Ui {
class OutputDataWidget;
}

class OutputDataWidget;

class OutputDataListModel : public QAbstractListModel
{
    Q_OBJECT

    friend class OutputDataWidget;

public:
    enum SimulationIconType
    {
        SimulationIconNone,
        SimulationIcon16Bit,
        SimulationIcon32Bit,
        SimulationIcon64Bit
    };
    Q_ENUM(SimulationIconType)

    explicit OutputDataListModel(OutputDataWidget* parent);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    bool isValid() const;
    QVector<quint16> values() const;

    void clear();
    void update();
    void updateData(const QModbusDataUnit& data);

    int columnsDistance() const
    {
        return _columnsDistance;
    }
    void setColumnsDistance(int value)
    {
        const int distance = qMax(1, value);
        if (_columnsDistance == distance)
            return;

        _columnsDistance = distance;
        if (rowCount() > 0)
            emit dataChanged(index(0), index(rowCount() - 1), QVector<int>() << Qt::DisplayRole);
    }

    QModelIndex find(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr) const;

private:
    SimulationIconType simulationIcon(int row) const;

private:
    struct ItemData
    {
        quint32 Address = 0;
        QVariant Value;
        QString ValueStr;
        QString Description;
        bool Simulated = false;
        SimulationIconType SimulationIcon = SimulationIconNone;
        QColor BgColor;
        QColor FgColor;
    };

    OutputDataWidget* _parentWidget;
    QModbusDataUnit _lastData;
    DataDisplayMode _lastMode = DataDisplayMode::Binary;
    bool _lastLeadingZeros = false;
    QModbusDataUnit::RegisterType _lastPointType = QModbusDataUnit::RegisterType::Invalid;
    ByteOrder _lastByteOrder = ByteOrder::Direct;
    QString _lastCodepage;
    const QPixmap _iconSimulation16Bit;
    const QPixmap _iconSimulation32Bit;
    const QPixmap _iconSimulation64Bit;
    const QPixmap _iconSimulationOff;
    int _columnsDistance = 16;
    QMap<int, ItemData> _mapItems;
};

class OutputDataWidget : public QFrame
{
    Q_OBJECT

    friend class OutputDataListModel;

public:
    explicit OutputDataWidget(QWidget* parent = nullptr);
    ~OutputDataWidget() override;

    QVector<quint16> data() const;

    void setup(const DataViewDefinitions& dd, const ModbusSimulationMap2& simulations, const QModbusDataUnit& data);

    DataDisplayMode dataDisplayMode() const;
    void setDataDisplayMode(DataDisplayMode mode);

    const ByteOrder* byteOrder() const;
    void setByteOrder(ByteOrder order);

    QString codepage() const;
    void setCodepage(const QString& name);

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

    int zoomPercent() const;
    void setZoomPercent(int zoomPercent);

    int dataViewColumnsDistance() const;
    void setDataViewColumnsDistance(int value);

    void setStatus(const QString& status);
    void setNotConnectedStatus();
    void setInvalidLengthStatus();

    void paint(const QRect& rc, QPainter& painter);
    void updateData(const QModbusDataUnit& data);

    AddressColorMap colorMap() const;
    void setColor(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr);

    AddressDescriptionMap2 descriptionMap() const;
    void setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc);

    void setSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, bool on);

signals:
    void itemDoubleClicked(quint16 address, const QVariant& value);

protected:
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void on_listView_doubleClicked(const QModelIndex& index);
    void on_listView_customContextMenuRequested(const QPoint& pos);

private:
    void showModbusMessage(const QModelIndex& index);
    void hideModbusMessage();
    void showZoomOverlay();
    QModelIndex getValueIndex(const QModelIndex& index) const;

private:
    Ui::OutputDataWidget* ui;
    QLabel* _zoomLabel = nullptr;
    QTimer* _zoomHideTimer = nullptr;

private:
    qreal _baseFontSize = 0.0;
    int _zoomPercent = 100;

    bool _displayHexAddreses;
    DataDisplayMode _dataDisplayMode;
    ByteOrder _byteOrder;
    QString _codepage;
    DataViewDefinitions _displayDefinition;
    AddressColorMap _colorMap;
    AddressDescriptionMap2 _descriptionMap;
    QSharedPointer<OutputDataListModel> _listModel;
};

#endif // OUTPUTDATAWIDGET_H


