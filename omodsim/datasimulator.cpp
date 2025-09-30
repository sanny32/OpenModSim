#include <QRandomGenerator>
#include "datasimulator.h"

///
/// \brief DataSimulator::DataSimulator
/// \param server
///
DataSimulator::DataSimulator(QObject* parent)
    : QObject{parent}
    ,_elapsed(0)
{
    connect(&_timer, &QTimer::timeout, this, &DataSimulator::on_timeout);
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
/// \param mode
/// \param deviceId
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::startSimulation(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params)
{
    QVariant value;
    switch (params.Mode)
    {
        case SimulationMode::Increment:
            value = params.IncrementParams.Range.from();
            emit dataSimulated(mode, deviceId, type, addr, value);
        break;

        case SimulationMode::Decrement:
            value = params.DecrementParams.Range.to();
            emit dataSimulated(mode, deviceId, type, addr, value);
        break;

        default:
        break;
    }

    _simulationMap[{ deviceId, type, addr}] = { mode, params, value };
    resumeSimulations();

    emit simulationStarted(deviceId, type, addr);
}

///
/// \brief DataSimulator::stopSimulation
/// \param deviceId
/// \param type
/// \param addr
///
void DataSimulator::stopSimulation(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr)
{
    _simulationMap.remove({ deviceId, type, addr});
     emit simulationStopped(deviceId, type, addr);
}

///
/// \brief DataSimulator::stopSimulations
///
void DataSimulator::stopSimulations()
{
    pauseSimulations();
    _simulationMap.clear();
}

///
/// \brief DataSimulator::pauseSimulations
///
void DataSimulator::pauseSimulations()
{
    _timer.stop();
}

///
/// \brief DataSimulator::resumeSimulations
///
void DataSimulator::resumeSimulations()
{
    if(!_timer.isActive())
        _timer.start(_interval);
}

///
/// \brief DataSimulator::restartSimulations
///
void DataSimulator::restartSimulations()
{
    pauseSimulations();
    for(auto&& key : _simulationMap.keys())
    {
        const auto mode = _simulationMap[key].Mode;
        const auto params = _simulationMap[key].Params;
        startSimulation(mode, key.DeviceId, key.Type, key.Address, params);
    }
}

///
/// \brief DataSimulator::simulationParams
/// \param type
/// \param addr
/// \return
///
ModbusSimulationParams DataSimulator::simulationParams(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr) const
{
    const auto it = _simulationMap.find({ deviceId, type, addr});
    return (it != _simulationMap.end()) ? it->Params : ModbusSimulationParams();
}

///
/// \brief DataSimulator::simulationMap
/// \return
///
ModbusSimulationMap2 DataSimulator::simulationMap() const
{
    ModbusSimulationMap2 map;
    for(auto&& key : _simulationMap.keys())
        map[key] = _simulationMap[key].Params;

    return map;
}

///
/// \brief DataSimulator::on_timeout
///
void DataSimulator::on_timeout()
{
    _elapsed++;
    for(auto&& key : _simulationMap.keys())
    {
        const auto mode = _simulationMap[key].Mode;
        const auto params = _simulationMap[key].Params;
        const auto interval = params.Interval;

        if((_elapsed * _interval) % interval) continue;

        switch(params.Mode)
        {
            case SimulationMode::Random:
                randomSimulation(mode, key.DeviceId, key.Type, key.Address, params.RandomParams);
            break;

            case SimulationMode::Increment:
                incrementSimulation(mode, key.DeviceId, key.Type, key.Address, params.IncrementParams);
            break;

            case SimulationMode::Decrement:
                decrementSimailation(mode, key.DeviceId, key.Type, key.Address, params.DecrementParams);
            break;

            case SimulationMode::Toggle:
                toggleSimulation(key.DeviceId, key.Type, key.Address);
            break;

            default:
            break;
        }
    }
}


template<typename T>
T generateRandom(double from, double to)
{
    return static_cast<T>(from + QRandomGenerator::global()->bounded(to - from));
}

template<typename T>
T generateRandom(const QRange<double>& range)
{
    return generateRandom<T>(range.from(), range.to());
}

///
/// \brief DataSimulator::randomSimulation
/// \param mode
/// \param deviceId
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::randomSimulation(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const RandomSimulationParams& params)
{
    auto&& value = _simulationMap[{ deviceId, type, addr}].CurrentValue;
    switch(type)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            value = generateRandom<quint16>(params.Range.from(), params.Range.to() + 1);
        break;

        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
            switch(mode)
            {
                case DataDisplayMode::Binary:
                case DataDisplayMode::Int16:
                case DataDisplayMode::UInt16:
                case DataDisplayMode::Hex:
                case DataDisplayMode::Ansi:
                    value = generateRandom<quint16>(params.Range.from(), params.Range.to() + 1);
                break;
                    
                case DataDisplayMode::Int32:
                case DataDisplayMode::SwappedInt32:
                    value = generateRandom<qint32>(params.Range);
                break;

                case DataDisplayMode::UInt32:
                case DataDisplayMode::SwappedUInt32:
                    value = generateRandom<quint32>(params.Range);
                break;

                case DataDisplayMode::FloatingPt:
                case DataDisplayMode::SwappedFP:
                   value = generateRandom<float>(params.Range);
                break;

                case DataDisplayMode::DblFloat:
                case DataDisplayMode::SwappedDbl:
                   value = generateRandom<double>(params.Range);
                break;

                case DataDisplayMode::Int64:
                case DataDisplayMode::SwappedInt64:
                    value = generateRandom<qint64>(params.Range);
                    break;

                case DataDisplayMode::UInt64:
                case DataDisplayMode::SwappedUInt64:
                    value = generateRandom<quint64>(params.Range);
                break;
            }
        break;

        default:
        break;
    }

    if(value.isValid())
        emit dataSimulated(mode, deviceId, type, addr, value);
}

template<typename T>
T incrementValue(T value, T step, const QRange<double>& range)
{
    value = value + step;
    if(value > range.to() || value < range.from()) value = static_cast<T>(range.from());
    return value;
}

///
/// \brief DataSimulator::incrementSimulation
/// \param mode
/// \param deviceId
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::incrementSimulation(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const IncrementSimulationParams& params)
{
    auto&& value = _simulationMap[{ deviceId, type, addr}].CurrentValue;
    switch(mode)
    {
        case DataDisplayMode::Int16:
            value = incrementValue<qint16>(value.toInt(), params.Step, params.Range);
        break;

        case DataDisplayMode::Binary:
        case DataDisplayMode::UInt16:
        case DataDisplayMode::Hex:
        case DataDisplayMode::Ansi:
            value = incrementValue<quint16>(value.toUInt(), params.Step, params.Range);
        break;
            
        case DataDisplayMode::Int32:
        case DataDisplayMode::SwappedInt32:
            value = incrementValue<qint32>(value.toInt(),  params.Step, params.Range);
        break;

        case DataDisplayMode::UInt32:
        case DataDisplayMode::SwappedUInt32:
            value = incrementValue<quint32>(value.toUInt(),  params.Step, params.Range);
        break;

        case DataDisplayMode::FloatingPt:
        case DataDisplayMode::SwappedFP:
            value = incrementValue<float>(value.toFloat(), params.Step, params.Range);
        break;

        case DataDisplayMode::DblFloat:
        case DataDisplayMode::SwappedDbl:
            value = incrementValue<double>(value.toDouble(), params.Step, params.Range);
        break;

        case DataDisplayMode::Int64:
        case DataDisplayMode::SwappedInt64:
            value = incrementValue<qint64>(value.toLongLong(), params.Step, params.Range);
            break;

        case DataDisplayMode::UInt64:
        case DataDisplayMode::SwappedUInt64:
            value = incrementValue<quint64>(value.toULongLong(), params.Step, params.Range);
        break;
    }

    if(value.isValid())
        emit dataSimulated(mode, deviceId, type, addr, value);
}

template<typename T>
T decrementValue(T value, T step, const QRange<double>& range)
{
    value = value - step;
    if(value > range.to() || value < range.from()) value = static_cast<T>(range.to());
    return value;
}

///
/// \brief DataSimulator::decrementSimailation
/// \param mode
/// \param deviceId
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::decrementSimailation(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const DecrementSimulationParams& params)
{
    auto&& value = _simulationMap[{ deviceId, type, addr}].CurrentValue;
    switch(mode)
    {
        case DataDisplayMode::Int16:
            value = decrementValue<qint16>(value.toInt(), params.Step, params.Range);
        break;

        case DataDisplayMode::Binary:
        case DataDisplayMode::UInt16:
        case DataDisplayMode::Hex:
        case DataDisplayMode::Ansi:
            value = decrementValue<quint16>(value.toUInt(), params.Step, params.Range);
        break;
            
        case DataDisplayMode::Int32:
        case DataDisplayMode::SwappedInt32:
            value = decrementValue<qint32>(value.toInt(),  params.Step, params.Range);
        break;

        case DataDisplayMode::UInt32:
        case DataDisplayMode::SwappedUInt32:
            value = decrementValue<quint32>(value.toUInt(),  params.Step, params.Range);
        break;

        case DataDisplayMode::FloatingPt:
        case DataDisplayMode::SwappedFP:
            value = decrementValue<float>(value.toFloat(), params.Step, params.Range);
        break;

        case DataDisplayMode::DblFloat:
        case DataDisplayMode::SwappedDbl:
            value = decrementValue<double>(value.toDouble(), params.Step, params.Range);
        break;

        case DataDisplayMode::Int64:
        case DataDisplayMode::SwappedInt64:
            value = decrementValue<qint64>(value.toLongLong(), params.Step, params.Range);
            break;

        case DataDisplayMode::UInt64:
        case DataDisplayMode::SwappedUInt64:
            value = decrementValue<quint64>(value.toULongLong(), params.Step, params.Range);
        break;
    }

    if(value.isValid())
        emit dataSimulated(mode, deviceId, type, addr, value);
}

///
/// \brief DataSimulator::toggleSimulation
/// \param deviceId
/// \param type
/// \param addr
///
void DataSimulator::toggleSimulation(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr)
{
    auto&& value = _simulationMap[{ deviceId, type, addr}].CurrentValue;
    value = !value.toBool();

    emit dataSimulated(DataDisplayMode::Binary, deviceId, type, addr, value);
}
