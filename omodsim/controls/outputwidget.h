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

struct AddressDescriptionMapKey {
    quint8 DeviceId;
    QModbusDataUnit::RegisterType Type;
    quint16 Address;

    bool operator<(const  AddressDescriptionMapKey &other) const {
        if (DeviceId != other.DeviceId)
            return DeviceId < other.DeviceId;
        if (Type != other.Type)
            return Type < other.Type;
        return Address < other.Address;
    }
};

typedef QMap<AddressDescriptionMapKey, QString> AddressDescriptionMap2;
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

    int columnsDistance() const {
        return _columnsDistance;
    }
    void setColumnsDistance(int value) {
        _columnsDistance = qMax(1, value);
    }

    QModelIndex find(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr) const;

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

    AddressDescriptionMap2 descriptionMap() const;
    void setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc);

    void setSimulated(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, bool on);

public slots:
    void clearLogView();
    void setLogViewState(LogViewState state);

signals:
    void startTextCaptureError(const QString& error);
    void itemDoubleClicked(quint16 address, const QVariant& value);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_listView_doubleClicked(const QModelIndex& index);
    void on_listView_customContextMenuRequested(const QPoint &pos);

private:
    void captureString(const QString& s);
    void showModbusMessage(const QModelIndex& index);
    void hideModbusMessage();
    void updateLogView(QSharedPointer<const ModbusMessage> msg);

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
    AddressDescriptionMap2 _descriptionMap;
    QSharedPointer<OutputListModel> _listModel;
};


///
/// \brief operator <<
/// \param out
/// \param key
/// \return
///
inline QDataStream& operator <<(QDataStream& out, const AddressDescriptionMapKey& key)
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
inline QDataStream& operator >>(QDataStream& in, AddressDescriptionMapKey& key)
{
    in >> key.DeviceId;
    in >> key.Type;
    in >> key.Address;
    return in;
}

///
/// \brief operator <<
/// \param xml
/// \param descriptionMap
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const AddressDescriptionMap2& descriptionMap)
{
    xml.writeStartElement("AddressDescriptionMap2");

    for (auto it = descriptionMap.constBegin(); it != descriptionMap.constEnd(); ++it) {
        const AddressDescriptionMapKey& key = it.key();
        const QString& description = it.value();

        if(!description.isEmpty())
        {
            xml.writeStartElement("DescriptionItem");

            xml.writeAttribute("DeviceId", QString::number(key.DeviceId));
            xml.writeAttribute("Type", enumToString<QModbusDataUnit::RegisterType>(key.Type));
            xml.writeAttribute("Address", QString::number(key.Address));

            if (!description.isEmpty()) {
                xml.writeCDATA(description);
            }

            xml.writeEndElement(); // DescriptionItem
        }
    }

    xml.writeEndElement(); // AddressDescriptionMap2
    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param descriptionMap
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, AddressDescriptionMap2& descriptionMap)
{
    descriptionMap.clear();

    if (xml.isStartElement() && xml.name() == QLatin1String("AddressDescriptionMap2")) {
        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("DescriptionItem")) {
                AddressDescriptionMapKey key;
                QString description;

                const QXmlStreamAttributes attributes = xml.attributes();

                if (attributes.hasAttribute("DeviceId")) {
                    bool ok; const quint8 deviceId = attributes.value("DeviceId").toUShort(&ok);
                    if (ok) key.DeviceId = deviceId;
                }

                if (attributes.hasAttribute("Type")) {
                    key.Type = enumFromString<QModbusDataUnit::RegisterType>(attributes.value("Type").toString());
                }

                if (attributes.hasAttribute("Address")) {
                    bool ok; const quint16 address = attributes.value("Address").toUShort(&ok);
                    if (ok) key.Address = address;
                }

                if (xml.isCDATA()) {
                    description = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                } else {
                    description = xml.readElementText();
                }

                if (key.DeviceId > 0 && key.Type != QModbusDataUnit::Invalid) {
                    descriptionMap.insert(key, description);
                }
            } else {
                xml.skipCurrentElement();
            }
        }
    }

    return xml;
}

#endif // OUTPUTWIDGET_H
