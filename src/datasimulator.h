#ifndef DATASIMULATOR_H
#define DATASIMULATOR_H

#include <QTimer>
#include <QElapsedTimer>
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

    bool canStartSimulation(DataType type, quint8 deviceId, QModbusDataUnit::RegisterType regType, quint16 addr) const;
    void startSimulation(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params);
    void stopSimulation(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr);
    void stopSimulations();

    void pauseSimulations();
    void resumeSimulations();
    void restartSimulations();

    ModbusSimulationParams simulationParams(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr) const;
    ModbusSimulationMap2 simulationMap() const;

    bool hasSimulation(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr) const;

signals:
    void simulationStarted(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, const QVector<quint16>& addresses);
    void simulationStopped(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, const QVector<quint16>& addresses);
    void dataSimulated(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, quint16 startAddress, QVariant value);

private slots:
    void on_timeout();

private:
    void scheduleNextRun();
    void randomSimulation(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, quint16 addr, const RandomSimulationParams& params);
    void incrementSimulation(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, quint16 addr, const IncrementSimulationParams& params);
    void decrementSimailation(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, quint16 addr, const DecrementSimulationParams& params);
    void toggleSimulation(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr);

private:
    QTimer _timer;
    QElapsedTimer _masterTimer;

    struct SimulationParams {
        DataType Type = DataType::UInt16;
        RegisterOrder Order = RegisterOrder::MSRF;
        ModbusSimulationParams Params;
        QVariant CurrentValue;
        qint64 NextRunTime = 0;
    };

    QMap<ModbusSimulationMapKey, SimulationParams> _simulationMap;
};

#endif // DATASIMULATOR_H
