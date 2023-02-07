#include <QRandomGenerator>
#include "datasimulator.h"
#include "modbusmultiserver.h"

///
/// \brief DataSimulator::DataSimulator
/// \param server
///
DataSimulator::DataSimulator(ModbusMultiServer* server)
    : QObject{server}
    ,_mbMultiServer(server)
    ,_elapsed(0)
{
    Q_ASSERT(_mbMultiServer != nullptr);
    connect(&_timer, &QTimer::timeout, this, &DataSimulator::on_timeout);
    _timer.start(1000);
}

///
/// \brief DataSimulator::~DataSimulator
///
DataSimulator::~DataSimulator()
{
    stopSimulations();
}

///
/// \brief DataSimulator::startSimulation
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params)
{
    _simulationMap[{ type, addr}] = params;
}

///
/// \brief DataSimulator::stopSimulation
/// \param type
/// \param addr
///
void DataSimulator::stopSimulation(QModbusDataUnit::RegisterType type, quint16 addr)
{
    _simulationMap.remove({ type, addr});
}

///
/// \brief DataSimulator::stopSimulations
///
void DataSimulator::stopSimulations()
{
    _simulationMap.clear();
}

///
/// \brief DataSimulator::on_timeout
///
void DataSimulator::on_timeout()
{
    _elapsed++;
    for(auto&& key : _simulationMap.keys())
    {
        const auto params = _simulationMap[key];
        const auto interval = params.Interval;

        if(_elapsed % interval) continue;

        switch(params.Mode)
        {
            case SimulationMode::Random:
                randomSimulation(key.first, key.second, params.RandomParams);
            break;

            case SimulationMode::Increment:
                incrementSimulation(key.first, key.second, params.IncrementParams);
            break;

            case SimulationMode::Decrement:
                decrementSimailation(key.first, key.second, params.DecrementParams);
            break;

            case SimulationMode::Toggle:
                toggleSimulation(key.first, key.second);
            break;

            default:
            break;
        }
    }
}

///
/// \brief DataSimulator::randomSimulation
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::randomSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const RandomSimulationParams& params)
{
    const quint16 value = QRandomGenerator::global()->bounded(params.Range.from(), params.Range.to() + 1);

    QModbusDataUnit data(type, addr, 1);
    data.setValue(0, value);
    _mbMultiServer->setData(data);
}

///
/// \brief DataSimulator::incrementSimulation
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::incrementSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const IncrementSimulationParams& params)
{
    auto data = _mbMultiServer->data(type, addr, 1);

    quint32 value = data.value(0) + params.Step;
    if(value > params.Range.to()) value = params.Range.from();

    data.setValue(0, value);
    _mbMultiServer->setData(data);
}

///
/// \brief DataSimulator::decrementSimailation
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::decrementSimailation(QModbusDataUnit::RegisterType type, quint16 addr, const DecrementSimulationParams& params)
{
    auto data = _mbMultiServer->data(type, addr, 1);

    qint32 value = data.value(0) - params.Step;
    if(value < params.Range.from()) value = params.Range.to();

    data.setValue(0, value);
    _mbMultiServer->setData(data);
}

///
/// \brief DataSimulator::toggleSimulation
/// \param type
/// \param addr
///
void DataSimulator::toggleSimulation(QModbusDataUnit::RegisterType type, quint16 addr)
{
    auto data = _mbMultiServer->data(type, addr, 1);
    data.setValue(0, !data.value(0));

    _mbMultiServer->setData(data);
}
