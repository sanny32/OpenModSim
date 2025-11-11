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

#endif // DATASIMULATOR_H
