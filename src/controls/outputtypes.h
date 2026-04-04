#ifndef OUTPUTTYPES_H
#define OUTPUTTYPES_H

#include <QColor>
#include <QDataStream>
#include <QDateTime>
#include <QMap>
#include <QModbusDataUnit>
#include <QSettings>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

///
/// \brief The ItemMapKey class
///
struct ItemMapKey {
    quint8 DeviceId;
    QModbusDataUnit::RegisterType Type;
    quint16 Address;

    bool operator<(const ItemMapKey& other) const
    {
        if (DeviceId != other.DeviceId)
            return DeviceId < other.DeviceId;
        if (Type != other.Type)
            return Type < other.Type;
        return Address < other.Address;
    }
};

using AddressColorMap = QMap<ItemMapKey, QColor>;
using AddressDescriptionMap = QMap<ItemMapKey, QString>;
using AddressTimestampMap = QMap<ItemMapKey, QDateTime>;

inline QDataStream& operator<<(QDataStream& out, const ItemMapKey& key)
{
    out << key.DeviceId;
    out << key.Type;
    out << key.Address;
    return out;
}

inline QDataStream& operator>>(QDataStream& in, ItemMapKey& key)
{
    in >> key.DeviceId;
    in >> key.Type;
    in >> key.Address;
    return in;
}

inline QSettings& operator<<(QSettings& out, const AddressDescriptionMap& map)
{
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream << map;
    out.setValue("AddressDescriptionMap", array);
    return out;
}

inline QSettings& operator>>(QSettings& in, AddressDescriptionMap& map)
{
    const auto array = in.value("AddressDescriptionMap").toByteArray();
    if (!array.isEmpty()) {
        QDataStream stream(array);
        stream >> map;
    }
    return in;
}

inline QSettings& operator<<(QSettings& out, const AddressTimestampMap& map)
{
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream << map;
    out.setValue("AddressTimestampMap", array);
    return out;
}

inline QSettings& operator>>(QSettings& in, AddressTimestampMap& map)
{
    const auto array = in.value("AddressTimestampMap").toByteArray();
    if (!array.isEmpty()) {
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
    if (!array.isEmpty()) {
        QDataStream stream(array);
        stream >> map;
    }
    return in;
}

inline QXmlStreamWriter& operator<<(QXmlStreamWriter& out, const AddressDescriptionMap& map)
{
    out.writeStartElement("AddressDescriptionMap");
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        if (!it.value().isEmpty()) {
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

inline QXmlStreamReader& operator>>(QXmlStreamReader& in, AddressDescriptionMap& map)
{
    while (in.readNextStartElement()) {
        bool skip = true;
        if (in.name() == QLatin1String("Description")) {
            const auto attributes = in.attributes();
            bool ok;
            auto device_id = static_cast<quint8>(attributes.value("DeviceId").toUShort(&ok));
            if (!ok)
                device_id = 0;

            auto type = static_cast<QModbusDataUnit::RegisterType>(attributes.value("Type").toInt(&ok));
            if (!ok)
                type = QModbusDataUnit::RegisterType::Invalid;

            const auto address = attributes.value("Address").toUShort(&ok);
            if (ok) {
                skip = false;
                const auto value = in.readElementText(QXmlStreamReader::IncludeChildElements);
                if (!value.isEmpty())
                    map.insert({ device_id, type, address }, value);
            }
        }

        if (skip)
            in.skipCurrentElement();
    }

    return in;
}

inline QXmlStreamWriter& operator<<(QXmlStreamWriter& out, const AddressTimestampMap& map)
{
    out.writeStartElement("AddressTimestampMap");
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        if (it.value().isValid()) {
            out.writeStartElement("Timestamp");
            out.writeAttribute("DeviceId", QString::number(it.key().DeviceId));
            out.writeAttribute("Type", QString::number(it.key().Type));
            out.writeAttribute("Address", QString::number(it.key().Address));
            out.writeAttribute("Value", it.value().toString(Qt::ISODateWithMs));
            out.writeEndElement(); // Timestamp
        }
    }
    out.writeEndElement(); // AddressTimestampMap
    return out;
}

inline QXmlStreamReader& operator>>(QXmlStreamReader& in, AddressTimestampMap& map)
{
    while (in.readNextStartElement()) {
        if (in.name() == QLatin1String("Timestamp")) {
            const auto attributes = in.attributes();
            bool ok;
            const auto device_id = static_cast<quint8>(attributes.value("DeviceId").toUShort(&ok));
            if (ok) {
                const auto type = static_cast<QModbusDataUnit::RegisterType>(attributes.value("Type").toInt(&ok));
                if (ok) {
                    const auto address = attributes.value("Address").toUShort(&ok);
                    if (ok) {
                        const auto valueText = attributes.value("Value").toString();
                        auto value = QDateTime::fromString(valueText, Qt::ISODateWithMs);
                        if (!value.isValid())
                            value = QDateTime::fromString(valueText, Qt::ISODate);
                        if (value.isValid())
                            map.insert({ device_id, type, address }, value);
                    }
                }
            }
        }
        in.skipCurrentElement();
    }

    return in;
}

inline QXmlStreamWriter& operator<<(QXmlStreamWriter& out, const AddressColorMap& map)
{
    out.writeStartElement("AddressColorMap");
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        if (it.value().isValid()) {
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
    while (in.readNextStartElement()) {
        if (in.name() == QLatin1String("Color")) {
            const auto attributes = in.attributes();
            bool ok;
            const auto device_id = static_cast<quint8>(attributes.value("DeviceId").toUShort(&ok));
            if (ok) {
                const auto type = static_cast<QModbusDataUnit::RegisterType>(attributes.value("Type").toInt(&ok));
                if (ok) {
                    const auto address = attributes.value("Address").toUShort(&ok);
                    if (ok) {
                        const auto value = attributes.value("Value").toString();
                        if (!value.isEmpty())
                            map.insert({ device_id, type, address }, value);
                    }
                }
            }
        }
        in.skipCurrentElement();
    }

    return in;
}

#endif // OUTPUTTYPES_H
