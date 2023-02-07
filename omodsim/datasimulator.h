#ifndef DATASIMULATOR_H
#define DATASIMULATOR_H

#include <QTimer>
#include <QModbusDataUnit>
#include "modbussimulationparams.h"

class ModbusMultiServer;

class DataSimulator : public QObject
{
    Q_OBJECT
public:
    explicit DataSimulator(ModbusMultiServer* server);
    ~DataSimulator() override;

    void startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params);
    void stopSimulation(QModbusDataUnit::RegisterType type, quint16 addr);
    void stopSimulations();

private slots:
    void on_timeout();

private:
    void randomSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const RandomSimulationParams& params);
    void incrementSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const IncrementSimulationParams& params);
    void decrementSimailation(QModbusDataUnit::RegisterType type, quint16 addr, const DecrementSimulationParams& params);
    void toggleSimulation(QModbusDataUnit::RegisterType type, quint16 addr);

private:
    ModbusMultiServer* _mbMultiServer;
    QTimer _timer;
    quint32 _elapsed;
    QMap<QPair<QModbusDataUnit::RegisterType, quint16>, ModbusSimulationParams> _simulationMap;
};

#endif // DATASIMULATOR_H
