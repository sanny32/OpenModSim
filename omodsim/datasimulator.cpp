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
{
    Q_ASSERT(_mbMultiServer != nullptr);
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
    if(_simulationMap.find({ type, addr}) == _simulationMap.end())
    {
        _simulationMap[{ type, addr}] = QSharedPointer<QTimer>(new QTimer(this));
        connect(_simulationMap[{ type, addr}].get(), &QTimer::timeout, this, &DataSimulator::on_timeout);
    }

    auto timer = _simulationMap[{ type, addr}];
    timer->setProperty("PointType", type);
    timer->setProperty("Address", addr);
    timer->setProperty("SimulationParams", QVariant::fromValue(params));
    timer->setInterval(params.Interval * 1000);
    timer->start();
}

///
/// \brief DataSimulator::stopSimulation
/// \param type
/// \param addr
///
void DataSimulator::stopSimulation(QModbusDataUnit::RegisterType type, quint16 addr)
{
    if(_simulationMap.find({ type, addr}) != _simulationMap.end())
    {
        _simulationMap[{ type, addr}]->stop();
    }
}

///
/// \brief DataSimulator::stopSimulations
///
void DataSimulator::stopSimulations()
{
    for(auto& t : _simulationMap.values())
    {
        t->stop();
    }
    _simulationMap.clear();
}

///
/// \brief DataSimulator::on_timeout
///
void DataSimulator::on_timeout()
{
    const auto timer = (QTimer*)sender();
    const auto addr = timer->property("Address").value<quint16>();
    const auto type = timer->property("PointType").value<QModbusDataUnit::RegisterType>();
    const auto params = timer->property("SimulationParams").value<ModbusSimulationParams>();

    switch(params.Mode)
    {
        case SimulationMode::Random:
            randomSimulation(type, addr, params.RandomParams);
        break;

        case SimulationMode::Increment:
            incrementSimulation(type, addr, params.IncrementParams);
        break;

        case SimulationMode::Decrement:
            decrementSimailation(type, addr, params.DecrementParams);
        break;

        case SimulationMode::Toggle:
            toggleSimulation(type, addr);
        break;

        default:
        break;
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
