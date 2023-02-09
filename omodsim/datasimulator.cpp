#include <QRandomGenerator>
#include "floatutils.h"
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
/// \param mode
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::startSimulation(DataDisplayMode mode, QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params)
{
    switch (params.Mode)
    {
        case SimulationMode::Increment:
            initializeValue(mode, type, addr, params.IncrementParams.Range.from());
        break;

        case SimulationMode::Decrement:
            initializeValue(mode, type, addr, params.IncrementParams.Range.to());
        break;

        default:
        break;
    }

    _simulationMap[{ type, addr}] = { mode, params };
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
        const auto mode = _simulationMap[key].first;
        const auto params = _simulationMap[key].second;
        const auto interval = params.Interval;

        if(_elapsed % interval) continue;

        switch(params.Mode)
        {
            case SimulationMode::Random:
                randomSimulation(mode, key.first, key.second, params.RandomParams);
            break;

            case SimulationMode::Increment:
                incrementSimulation(mode, key.first, key.second, params.IncrementParams);
            break;

            case SimulationMode::Decrement:
                decrementSimailation(mode, key.first, key.second, params.DecrementParams);
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
/// \brief DataSimulator::initializeValue
/// \param mode
/// \param type
/// \param addr
/// \param value
///
void DataSimulator::initializeValue(DataDisplayMode mode, QModbusDataUnit::RegisterType type, quint16 addr, double value)
{
    switch(mode)
    {
        case DataDisplayMode::Integer:
            _mbMultiServer->writeValue(type, addr, static_cast<qint16>(value));
        break;

        case DataDisplayMode::Binary:
        case DataDisplayMode::Decimal:
        case DataDisplayMode::Hex:
            _mbMultiServer->writeValue(type, addr, static_cast<quint16>(value));
        break;

        case DataDisplayMode::FloatingPt:
            _mbMultiServer->writeFloat(type, addr, static_cast<float>(value), false);
        break;

        case DataDisplayMode::SwappedFP:
            _mbMultiServer->writeFloat(type, addr, static_cast<float>(value), true);
        break;

        case DataDisplayMode::DblFloat:
            _mbMultiServer->writeDouble(type, addr, value, false);
        break;

        case DataDisplayMode::SwappedDbl:
            _mbMultiServer->writeDouble(type, addr, value, true);
        break;
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
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::randomSimulation(DataDisplayMode mode, QModbusDataUnit::RegisterType type, quint16 addr, const RandomSimulationParams& params)
{
    QVector<quint16> values;
    switch(type)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            values.push_back(generateRandom<quint16>(params.Range.from(), params.Range.to() + 1));
        break;

        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
            switch(mode)
            {
                case DataDisplayMode::Binary:
                case DataDisplayMode::Integer:
                case DataDisplayMode::Decimal:
                case DataDisplayMode::Hex:
                    values.push_back(generateRandom<quint16>(params.Range.from(), params.Range.to() + 1));
                break;

                case DataDisplayMode::FloatingPt:
                    values.resize(2);
                    breakFloat(generateRandom<float>(params.Range), values[0], values[1]);
                break;

                case DataDisplayMode::SwappedFP:
                    values.resize(2);
                    breakFloat(generateRandom<float>(params.Range), values[1], values[0]);
                break;

                case DataDisplayMode::DblFloat:
                    values.resize(4);
                    breakDouble(generateRandom<double>(params.Range), values[0], values[1], values[2], values[3]);
                break;

                case DataDisplayMode::SwappedDbl:
                    values.resize(4);
                    breakDouble(generateRandom<double>(params.Range), values[3], values[2], values[1], values[0]);
                break;
            }
        break;

        default:
        break;
    }

   if(!values.isEmpty())
   {
        auto data = QModbusDataUnit(type, addr, values.size());
        data.setValues(values);
        _mbMultiServer->setData(data);
   }
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
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::incrementSimulation(DataDisplayMode mode, QModbusDataUnit::RegisterType type, quint16 addr, const IncrementSimulationParams& params)
{
    switch(mode)
    {
        case DataDisplayMode::Integer:
        {
            const auto data = _mbMultiServer->data(type, addr, 1);
            _mbMultiServer->writeValue(type, addr, incrementValue<qint16>(data.value(0), params.Step, params.Range));
        }
        break;

        case DataDisplayMode::Binary:
        case DataDisplayMode::Decimal:
        case DataDisplayMode::Hex:
        {
            const auto data = _mbMultiServer->data(type, addr, 1);
            _mbMultiServer->writeValue(type, addr, incrementValue<quint16>(data.value(0), params.Step, params.Range));
        }
        break;

        case DataDisplayMode::FloatingPt:
        {
            const auto value = _mbMultiServer->readFloat(type, addr, false);
            _mbMultiServer->writeFloat(type, addr, incrementValue<float>(value, params.Step, params.Range), false);
        }
        break;

        case DataDisplayMode::SwappedFP:
        {
            const auto value = _mbMultiServer->readFloat(type, addr, true);
            _mbMultiServer->writeFloat(type, addr, incrementValue<float>(value, params.Step, params.Range), true);
        }
        break;

        case DataDisplayMode::DblFloat:
        {
            const auto value = _mbMultiServer->readDouble(type, addr, false);
            _mbMultiServer->writeDouble(type, addr, incrementValue<double>(value, params.Step, params.Range), false);
        }
        break;

        case DataDisplayMode::SwappedDbl:
        {
            const auto value = _mbMultiServer->readDouble(type, addr, true);
            _mbMultiServer->writeDouble(type, addr, incrementValue<double>(value, params.Step, params.Range), true);
        }
        break;
    }
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
/// \param type
/// \param addr
/// \param params
///
void DataSimulator::decrementSimailation(DataDisplayMode mode, QModbusDataUnit::RegisterType type, quint16 addr, const DecrementSimulationParams& params)
{
    switch(mode)
    {
        case DataDisplayMode::Integer:
        {
            const auto data = _mbMultiServer->data(type, addr, 1);
            _mbMultiServer->writeValue(type, addr, decrementValue<qint16>(data.value(0), params.Step, params.Range));
        }
        break;

        case DataDisplayMode::Binary:
        case DataDisplayMode::Decimal:
        case DataDisplayMode::Hex:
        {
            const auto data = _mbMultiServer->data(type, addr, 1);
            _mbMultiServer->writeValue(type, addr, decrementValue<quint16>(data.value(0), params.Step, params.Range));
        }
        break;

        case DataDisplayMode::FloatingPt:
        {
            const auto value = _mbMultiServer->readFloat(type, addr, false);
            _mbMultiServer->writeFloat(type, addr, decrementValue<float>(value, params.Step, params.Range), false);
        }
        break;

        case DataDisplayMode::SwappedFP:
        {
            const auto value = _mbMultiServer->readFloat(type, addr, true);
            _mbMultiServer->writeFloat(type, addr, decrementValue<float>(value, params.Step, params.Range), true);
        }
        break;

        case DataDisplayMode::DblFloat:
        {
            const auto value = _mbMultiServer->readDouble(type, addr, false);
            _mbMultiServer->writeDouble(type, addr, decrementValue<double>(value, params.Step, params.Range), false);
        }
        break;

        case DataDisplayMode::SwappedDbl:
        {
            const auto value = _mbMultiServer->readDouble(type, addr, true);
            _mbMultiServer->writeDouble(type, addr, decrementValue<double>(value, params.Step, params.Range), true);
        }
        break;
    }
}

///
/// \brief DataSimulator::toggleSimulation
/// \param type
/// \param addr
///
void DataSimulator::toggleSimulation(QModbusDataUnit::RegisterType type, quint16 addr)
{
    const auto data = _mbMultiServer->data(type, addr, 1);
    _mbMultiServer->writeValue(type, addr, !data.value(0));
}
