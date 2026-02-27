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

///
/// \brief The QEmptyPixmap class
///
class QEmptyPixmap : public QPixmap {
public:
    QEmptyPixmap(const QSize& size) :
        QPixmap(size) {
        fill(Qt::transparent);
    }
};

///
/// \brief The ItemMapKey class
///
struct ItemMapKey {
    quint8 DeviceId;
    QModbusDataUnit::RegisterType Type;
    quint16 Address;

    bool operator<(const  ItemMapKey &other) const {
        if (DeviceId != other.DeviceId)
            return DeviceId < other.DeviceId;
        if (Type != other.Type)
            return Type < other.Type;
        return Address < other.Address;
    }
};

typedef QMap<ItemMapKey, QColor> AddressColorMap;
typedef QMap<ItemMapKey, QString> AddressDescriptionMap2;
typedef QMap<QPair<QModbusDataUnit::RegisterType, quint16>, QString> AddressDescriptionMap;

///
/// \brief The OutputListModel class
///
class OutputListModel : public QAbstractListModel
{
    Q_OBJECT

    friend class OutputWidget;

public:
    enum SimulationIconType
    {
        SimulationIconNone,
        SimulationIcon16Bit,
        SimulationIcon32Bit,
        SimulationIcon64Bit
    };
    Q_ENUM(SimulationIconType)

    explicit OutputListModel(OutputWidget* parent);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool isValid() const;
    QVector<quint16> values() const;

    void clear();
    void update();
    void updateData(const QModbusDataUnit& data);

    int columnsDistance() const {
        return _columnsDistance;
    }
    void setColumnsDistance(int value) {
        _columnsDistance = qMax(1, value);
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

    OutputWidget* _parentWidget;
    QModbusDataUnit _lastData;
    const QPixmap _iconSimulation16Bit;
    const QPixmap _iconSimulation32Bit;
    const QPixmap _iconSimulation64Bit;
    const QEmptyPixmap _iconSimulationOff;
    int _columnsDistance = 16;
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

    void setup(const DisplayDefinition& dd,const ModbusSimulationMap2& simulations, const QModbusDataUnit& data);

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

    int zoomPercent() const;
    void setZoomPercent(int zoomPercent);

    int dataViewColumnsDistance() const;
    void setDataViewColumnsDistance(int value);

    int logViewLimit() const;
    void setLogViewLimit(int l);

    bool autoscrollLogView() const;
    void setAutosctollLogView(bool on);

    void setStatus(const QString& status);
    void setNotConnectedStatus();
    void setInvalidLengthStatus();

    void paint(const QRect& rc, QPainter& painter);

    void updateTraffic(QSharedPointer<const ModbusMessage> msg);
    void updateData(const QModbusDataUnit& data);

    AddressColorMap colorMap() const;
    void setColor(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr);

    AddressDescriptionMap2 descriptionMap() const;
    void setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc);

    void setSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, bool on);

public slots:
    void clearLogView();
    void setLogViewState(LogViewState state);

signals:
    void startTextCaptureError(const QString& error);
    void itemDoubleClicked(quint16 address, const QVariant& value);

protected:
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void on_listView_doubleClicked(const QModelIndex& index);
    void on_listView_customContextMenuRequested(const QPoint &pos);

private:
    void captureString(const QString& s);
    void showModbusMessage(const QModelIndex& index);
    void hideModbusMessage();
    void showZoomOverlay();
    void updateLogView(QSharedPointer<const ModbusMessage> msg);
    QModelIndex getValueIndex(const QModelIndex& index) const;

private:
    Ui::OutputWidget *ui;
    QLabel* _zoomLabel = nullptr;
    QTimer* _zoomHideTimer = nullptr;

private:
    qreal _baseFontSize = 0.0;
    int _zoomPercent = 100;

    bool _displayHexAddreses;
    DisplayMode _displayMode;
    DataDisplayMode _dataDisplayMode;
    ByteOrder _byteOrder;
    QString _codepage;
    DisplayDefinition _displayDefinition;
    QFile _fileCapture;
    AddressColorMap _colorMap;
    AddressDescriptionMap2 _descriptionMap;
    QSharedPointer<OutputListModel> _listModel;
};


inline QSettings& operator<<(QSettings& out, const AddressDescriptionMap2& map)
{
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream << map;
    out.setValue("AddressDescriptionMap", array);

    return out;
}

inline QSettings& operator>>(QSettings& in, AddressDescriptionMap2& map)
{
    const auto array = in.value("AddressDescriptionMap").toByteArray();
    if(!array.isEmpty())
    {
        QDataStream stream(array);
        stream >> map;
    }

    return in;
}

inline QSettings& operator<<(QSettings& out, const AddressColorMap& map)
{
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream << map;
    out.setValue("AddressColorMap", array);

    return out;
}

inline QSettings& operator>>(QSettings& in, AddressColorMap& map)
{
    const auto array = in.value("AddressColorMap").toByteArray();
    if(!array.isEmpty())
    {
        QDataStream stream(array);
        stream >> map;
    }

    return in;
}

inline QXmlStreamWriter& operator<<(QXmlStreamWriter& out, const AddressDescriptionMap2& map)
{
    out.writeStartElement("AddressDescriptionMap");
    for(auto it = map.cbegin(); it != map.cend(); ++it)
    {
        if(!it.value().isEmpty())
        {
            out.writeStartElement("Description");
            out.writeAttribute("DeviceId", QString::number(it.key().DeviceId));
            out.writeAttribute("Type", QString::number(it.key().Type));
            out.writeAttribute("Address", QString::number(it.key().Address));
            out.writeCDATA(it.value());
            out.writeEndElement(); // Description
        }
    }
    out.writeEndElement(); // AddressDescriptionMap

    return out;
}

inline QXmlStreamReader& operator>>(QXmlStreamReader& in, AddressDescriptionMap2& map)
{
    while (in.readNextStartElement())
    {
        bool skip = true;
        if (in.name() == QLatin1String("Description"))
        {
            const auto attributes = in.attributes();
            bool ok;
            const auto device_id = static_cast<quint8>(attributes.value("DeviceId").toUShort(&ok));
            if(ok)
            {
                const auto type = static_cast<QModbusDataUnit::RegisterType>(attributes.value("Type").toInt(&ok));
                if(ok)
                {
                    const auto address = attributes.value("Address").toUShort(&ok);
                    if(ok)
                    {
                        skip = false;
                        const auto value = in.readElementText(QXmlStreamReader::IncludeChildElements);
                        if(!value.isEmpty())
                        {
                            map.insert( { device_id, type, address }, value );
                        }
                    }
                }
            }

        }
        if(skip)
        {
            in.skipCurrentElement();
        }
    }

    return in;
}

inline QXmlStreamWriter& operator<<(QXmlStreamWriter& out, const AddressColorMap& map)
{
    out.writeStartElement("AddressColorMap");
    for(auto it = map.cbegin(); it != map.cend(); ++it)
    {
        if(it.value().isValid())
        {
            out.writeStartElement("Color");
            out.writeAttribute("DeviceId", QString::number(it.key().DeviceId));
            out.writeAttribute("Type", QString::number(it.key().Type));
            out.writeAttribute("Address", QString::number(it.key().Address));
            out.writeAttribute("Value", it.value().name());
            out.writeEndElement(); // Color
        }
    }
    out.writeEndElement(); // AddressColorMap

    return out;
}

inline QXmlStreamReader& operator>>(QXmlStreamReader& in, AddressColorMap& map)
{
    while(in.readNextStartElement())
    {
        if(in.name() == QLatin1String("Color"))
        {
            const auto attributes = in.attributes();
            bool ok;
            const auto device_id = static_cast<quint8>(attributes.value("DeviceId").toUShort(&ok));
            if(ok)
            {
                const auto type = static_cast<QModbusDataUnit::RegisterType>(attributes.value("Type").toInt(&ok));
                if(ok)
                {
                    const auto address = attributes.value("Address").toUShort(&ok);
                    if(ok)
                    {
                        const auto value = attributes.value("Value").toString();
                        if(!value.isEmpty())
                        {
                            map.insert( { device_id, type, address }, value );
                        }
                    }
                }
            }
        }
        in.skipCurrentElement();
    }

    return in;
}

///
/// \brief operator <<
/// \param out
/// \param key
/// \return
///
inline QDataStream& operator <<(QDataStream& out, const ItemMapKey& key)
{
    out << key.DeviceId;
    out << key.Type;
    out << key.Address;

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param params
/// \return
///
inline QDataStream& operator >>(QDataStream& in, ItemMapKey& key)
{
    in >> key.DeviceId;
    in >> key.Type;
    in >> key.Address;

    return in;
}

#endif // OUTPUTWIDGET_H
