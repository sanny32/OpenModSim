#ifndef DATASIMULATOR_H
#define DATASIMULATOR_H

#include <QTimer>
#include <QModbusDataUnit>
#include <QXmlStreamWriter>
#include "modbussimulationparams.h"

struct ModbusSimulationMapKey {
    quint8 DeviceId;
    QModbusDataUnit::RegisterType Type;
    quint16 Address;

    bool operator<(const ModbusSimulationMapKey &other) const {
        if (DeviceId != other.DeviceId)
            return DeviceId < other.DeviceId;
        if (Type != other.Type)
            return Type < other.Type;
        return Address < other.Address;
    }
};

typedef QMap<QPair<QModbusDataUnit::RegisterType, quint16>, ModbusSimulationParams> ModbusSimulationMap;
typedef QMap<ModbusSimulationMapKey, ModbusSimulationParams> ModbusSimulationMap2;

///
/// \brief The DataSimulator class
///
class DataSimulator : public QObject
{
    Q_OBJECT
public:
    explicit DataSimulator(QObject* parent = nullptr);
    ~DataSimulator() override;

    void startSimulation(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params);
    void stopSimulation(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr);
    void stopSimulations();

    void pauseSimulations();
    void resumeSimulations();
    void restartSimulations();

    ModbusSimulationParams simulationParams(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr) const;
    ModbusSimulationMap2 simulationMap() const;

signals:
    void simulationStarted(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr);
    void simulationStopped(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr);
    void dataSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, QVariant value);

private slots:
    void on_timeout();

private:
    void randomSimulation(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const RandomSimulationParams& params);
    void incrementSimulation(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const IncrementSimulationParams& params);
    void decrementSimailation(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const DecrementSimulationParams& params);
    void toggleSimulation(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr);

private:
    QTimer _timer;
    quint32 _elapsed;
    const int _interval = 1;

    struct SimulationParams {
        DataDisplayMode Mode;
        ModbusSimulationParams Params;
        QVariant CurrentValue;
    };

    QMap<ModbusSimulationMapKey, SimulationParams> _simulationMap;
};

///
/// \brief operator <<
/// \param out
/// \param key
/// \return
///
inline QDataStream& operator <<(QDataStream& out, const ModbusSimulationMapKey& key)
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
inline QDataStream& operator >>(QDataStream& in, ModbusSimulationMapKey& key)
{
    in >> key.DeviceId;
    in >> key.Type;
    in >> key.Address;
    return in;
}

///
/// \brief operator <<
/// \param xml
/// \param simulationMap
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const ModbusSimulationMap2& simulationMap)
{
    xml.writeStartElement("ModbusSimulationMap2");

    for (auto it = simulationMap.constBegin(); it != simulationMap.constEnd(); ++it) {
        const ModbusSimulationMapKey& key = it.key();
        const ModbusSimulationParams& params = it.value();

        if(params.Mode != SimulationMode::No)
        {
            xml.writeStartElement("SimulationItem");
            xml.writeAttribute("DeviceId", QString::number(key.DeviceId));
            xml.writeAttribute("Type", enumToString<QModbusDataUnit::RegisterType>(key.Type));
            xml.writeAttribute("Address", QString::number(key.Address));
            xml << params;
            xml.writeEndElement();
        }
    }

    xml.writeEndElement();
    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param simulationMap
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, ModbusSimulationMap2& simulationMap)
{
    simulationMap.clear();

    if (xml.isStartElement() && xml.name() == QLatin1String("ModbusSimulationMap2")) {
        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("SimulationItem")) {
                ModbusSimulationMapKey key;
                ModbusSimulationParams params;

                const QXmlStreamAttributes attributes = xml.attributes();

                // DeviceId
                if (attributes.hasAttribute("DeviceId")) {
                    bool ok;
                    const quint8 deviceId = attributes.value("DeviceId").toUShort(&ok);
                    if (ok) key.DeviceId = deviceId;
                }

                // Type
                if (attributes.hasAttribute("Type")) {
                    key.Type = enumFromString<QModbusDataUnit::RegisterType>(attributes.value("Type").toString());
                }

                // Address
                if (attributes.hasAttribute("Address")) {
                    bool ok;
                    const quint16 address = attributes.value("Address").toUShort(&ok);
                    if (ok) key.Address = address;
                }

                // Params
                xml >> params;

                xml.skipCurrentElement();

                if (key.DeviceId > 0 && key.Type != QModbusDataUnit::Invalid) {
                    simulationMap.insert(key, params);
                }

            } else {
                xml.skipCurrentElement();
            }
        }
    }

    return xml;
}

#endif // DATASIMULATOR_H
