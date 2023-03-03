#include "floatutils.h"
#include "modbusmultiserver.h"

///
/// \brief ModbusServer::ModbusServer
/// \param parent
///
ModbusMultiServer::ModbusMultiServer(QObject *parent)
    : QObject{parent}
    ,_deviceId(1)
    ,_simulator(new DataSimulator(this))
{
    connect(_simulator.get(), &DataSimulator::dataSimulated, this, &ModbusMultiServer::on_dataSimulated);
}

///
/// \brief ModbusServer::~ModbusServer
///
ModbusMultiServer::~ModbusMultiServer()
{
   for(auto&& s : _modbusServerList)
       s->disconnectDevice();
}

///
/// \brief ModbusServer::deviceId
/// \return
///
quint8 ModbusMultiServer::deviceId() const
{
    return _deviceId;
}

///
/// \brief ModbusServer::setDeviceId
/// \param deviceId
///
void ModbusMultiServer::setDeviceId(quint8 deviceId)
{
    if(deviceId == _deviceId)
        return;

    _deviceId = deviceId;

    for(auto&& s : _modbusServerList)
        s->setServerAddress(deviceId);

    emit deviceIdChanged(deviceId);
}

///
/// \brief ModbusMultiServer::addUnitMap
/// \param id
/// \param pointType
/// \param pointAddress
/// \param length
///
void ModbusMultiServer::addUnitMap(int id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length)
{
    _modbusDataUnitMap.addUnitMap(id, pointType, pointAddress, length);
    reconfigureServers();
}

///
/// \brief ModbusMultiServer::removeUnitMap
/// \param id
///
void ModbusMultiServer::removeUnitMap(int id)
{
    _modbusDataUnitMap.removeUnitMap(id);
    reconfigureServers();
}

///
/// \brief ModbusMultiServer::findModbusServer
/// \param cd
/// \return
///
QSharedPointer<QModbusServer> ModbusMultiServer::findModbusServer(const ConnectionDetails& cd) const
{
    for(auto&& s : _modbusServerList)
        if(s->property("ConnectionDetails").value<ConnectionDetails>() == cd)
            return s;

    return nullptr;
}

///
/// \brief ModbusMultiServer::findModbusServer
/// \param type
/// \param port
/// \return
///
QSharedPointer<QModbusServer> ModbusMultiServer::findModbusServer(ConnectionType type, const QString& port) const
{
    for(auto&& s : _modbusServerList)
    {
        const auto cd = s->property("ConnectionDetails").value<ConnectionDetails>();
        if((cd.Type == ConnectionType::Tcp && type == ConnectionType::Tcp) ||
           (cd.Type == ConnectionType::Serial && type == ConnectionType::Serial && cd.SerialParams.PortName == port))
        {
            return s;
        }
    }

    return nullptr;
}

///
/// \brief ModbusMultiServer::createModbusServer
/// \param cd
/// \return
///
QSharedPointer<QModbusServer> ModbusMultiServer::createModbusServer(const ConnectionDetails& cd)
{
    auto modbusServer = findModbusServer(cd);
    if(modbusServer == nullptr)
    {
        switch(cd.Type)
        {
            case ConnectionType::Tcp:
            {
                modbusServer = QSharedPointer<QModbusServer>(new ModbusTcpServer(this));
                modbusServer->setProperty("ConnectionDetails", QVariant::fromValue(cd));
                modbusServer->setConnectionParameter(QModbusDevice::NetworkPortParameter, cd.TcpParams.ServicePort);
                modbusServer->setConnectionParameter(QModbusDevice::NetworkAddressParameter, cd.TcpParams.IPAddress);

                connect((ModbusTcpServer*)modbusServer.get(), &ModbusTcpServer::request, this, [&](const QModbusRequest& req)
                {
                    emit request(req);
                });
                connect((ModbusTcpServer*)modbusServer.get(), &ModbusTcpServer::response, this, [&](const QModbusResponse& resp)
                {
                    emit response(resp);
                });
            }
            break;

            case ConnectionType::Serial:
            {
                modbusServer = QSharedPointer<QModbusServer>(new ModbusRtuServer(this));
                modbusServer->setProperty("ConnectionDetails", QVariant::fromValue(cd));
                modbusServer->setProperty("DTRControl", cd.SerialParams.SetDTR);
                modbusServer->setProperty("RTSControl", cd.SerialParams.SetRTS);
                modbusServer->setConnectionParameter(QModbusDevice::SerialPortNameParameter, cd.SerialParams.PortName);
                modbusServer->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, cd.SerialParams.BaudRate);
                modbusServer->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, cd.SerialParams.WordLength);
                modbusServer->setConnectionParameter(QModbusDevice::SerialParityParameter, cd.SerialParams.Parity);
                modbusServer->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, cd.SerialParams.StopBits);
                qobject_cast<QSerialPort*>(modbusServer->device())->setFlowControl(cd.SerialParams.FlowControl);

                connect((ModbusRtuServer*)modbusServer.get(), &ModbusRtuServer::request, this, [&](const QModbusRequest& req)
                {
                    emit request(req);
                });
                connect((ModbusRtuServer*)modbusServer.get(), &ModbusRtuServer::response, this, [&](const QModbusResponse& resp)
                {
                    emit response(resp);
                });
            }
            break;
        }
    }

    if(modbusServer)
    {
        connect(modbusServer.get(), &QModbusServer::dataWritten, this, &ModbusMultiServer::on_dataWritten);
        connect(modbusServer.get(), &QModbusDevice::stateChanged, this, &ModbusMultiServer::on_stateChanged);
        connect(modbusServer.get(), &QModbusDevice::errorOccurred, this, &ModbusMultiServer::on_errorOccurred);
    }

    return modbusServer;
}

///
/// \brief ModbusServer::connectDevice
/// \param cd
///
void ModbusMultiServer::connectDevice(const ConnectionDetails& cd)
{
    auto modbusServer = findModbusServer(cd);
    if(modbusServer == nullptr)
    {
        modbusServer = createModbusServer(cd);
        addModbusServer(modbusServer);
    }

    modbusServer->setServerAddress(_deviceId);
    modbusServer->setMap(_modbusDataUnitMap);

    for(auto data : _modbusDataUnitMap)
    {
        _modbusServerList.first()->data(&data);
        modbusServer->setData(data);
    }

    modbusServer->connectDevice();
}

///
/// \brief ModbusMultiServer::disconnectDevice
/// \param type
/// \param port
///
void ModbusMultiServer::disconnectDevice(ConnectionType type, const QString& port)
{
    auto modbusServer = findModbusServer(type, port);
    if(modbusServer != nullptr)
    {
        modbusServer->disconnectDevice();
        removeModbusServer(modbusServer);
    }
}

///
/// \brief ModbusMultiServer::connections
/// \return
///
QList<ConnectionDetails> ModbusMultiServer::connections() const
{
    QList<ConnectionDetails> conns;
    for(auto&& s : _modbusServerList)
    {
        if(s->state() == QModbusDevice::ConnectedState)
            conns.append(s->property("ConnectionDetails").value<ConnectionDetails>());
    }
    return conns;
}

///
/// \brief ModbusMultiServer::addModbusServer
/// \param server
///
void ModbusMultiServer::addModbusServer(QSharedPointer<QModbusServer> server)
{
    if(server && !_modbusServerList.contains(server))
        _modbusServerList.push_back(server);
}

///
/// \brief ModbusMultiServer::removeModbusServer
/// \param server
///
void ModbusMultiServer::removeModbusServer(QSharedPointer<QModbusServer> server)
{
    if(server)
        _modbusServerList.removeOne(server);
}

///
/// \brief ModbusMultiServer::reconfigureServers
///
void ModbusMultiServer::reconfigureServers()
{
    for(auto&& s : _modbusServerList)
        s->setMap(_modbusDataUnitMap);
}

///
/// \brief ModbusMultiServer::isConnected
/// \return
///
bool ModbusMultiServer::isConnected() const
{
    for(auto&& s : _modbusServerList)
        if(s->state() == QModbusDevice::ConnectedState)
            return true;

    return false;
}

///
/// \brief ModbusMultiServer::isConnected
/// \param type
/// \param port
/// \return
///
bool ModbusMultiServer::isConnected(ConnectionType type, const QString& port) const
{
    return state(type, port) == QModbusDevice::ConnectedState;
}

///
/// \brief ModbusMultiServer::state
/// \param type
/// \param port
/// \return
///
QModbusDevice::State ModbusMultiServer::state(ConnectionType type, const QString& port) const
{
    const auto modbusServer = findModbusServer(type, port);
    return modbusServer ? modbusServer->state() : QModbusDevice::UnconnectedState;
}

///
/// \brief ModbusServer::data
/// \param dd
/// \return
///
QModbusDataUnit ModbusMultiServer::data(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const
{
    return _modbusDataUnitMap.getData(pointType, pointAddress, length);
}

///
/// \brief ModbusMultiServer::setData
/// \param data
///
void ModbusMultiServer::setData(const QModbusDataUnit& data)
{
    _modbusDataUnitMap.setData(data);
    for(auto&& s : _modbusServerList)
    {
        s->blockSignals(true);
        s->setData(data);
        s->blockSignals(false);
    }
    emit dataChanged(data);
}

///
/// \brief createDataUnit
/// \param type
/// \param newStartAddress
/// \param values
/// \return
///
QModbusDataUnit createDataUnit(QModbusDataUnit::RegisterType type, int newStartAddress, const QVector<quint16>& values)
{
    auto data = QModbusDataUnit(type, newStartAddress, values.count());
    data.setValues(values);

    return data;
}

///
/// \brief createFloatDataUnit
/// \param type
/// \param newStartAddress
/// \param value
/// \param order
/// \param swapped
/// \return
///
QModbusDataUnit createFloatDataUnit(QModbusDataUnit::RegisterType type, int newStartAddress, float value, ByteOrder order, bool swapped)
{
    Q_ASSERT(type == QModbusDataUnit::HoldingRegisters
             || type == QModbusDataUnit::InputRegisters);

    QVector<quint16> values(2);
    auto data = QModbusDataUnit(type, newStartAddress, 2);

    if(swapped)
        breakFloat(value, values[1], values[0], order);
    else
        breakFloat(value, values[0], values[1], order);

    data.setValues(values);
    return data;
}

///
/// \brief createDoubleDataUnit
/// \param type
/// \param newStartAddress
/// \param value
/// \param order
/// \param swapped
/// \return
///
QModbusDataUnit createDoubleDataUnit(QModbusDataUnit::RegisterType type, int newStartAddress, double value, ByteOrder order, bool swapped)
{
    Q_ASSERT(type == QModbusDataUnit::HoldingRegisters
             || type == QModbusDataUnit::InputRegisters);

    QVector<quint16> values(4);
    auto data = QModbusDataUnit(type, newStartAddress, 4);

    if(swapped)
        breakDouble(value, values[3], values[2], values[1], values[0], order);
    else
        breakDouble(value, values[0], values[1], values[2], values[3], order);

    data.setValues(values);
    return data;
}

///
/// \brief ModbusMultiServer::writeValue
/// \param pointType
/// \param pointAddress
/// \param value
///
void ModbusMultiServer::writeValue(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 value)
{
    auto data = QModbusDataUnit(pointType, pointAddress, 1);
    data.setValue(0, value);
    setData(data);
}

///
/// \brief ModbusMultiServer::readFloat
/// \param pointType
/// \param pointAddress
/// \param order
/// \param swapped
/// \return
///
float ModbusMultiServer::readFloat(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    const auto data = this->data(pointType, pointAddress, 2);
    return swapped ?  makeFloat(data.value(1), data.value(0), order): makeFloat(data.value(0), data.value(1), order);
}

///
/// \brief ModbusMultiServer::writeFloat
/// \param pointType
/// \param pointAddress
/// \param value
/// \param swapped
///
void ModbusMultiServer::writeFloat(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, float value, ByteOrder order, bool swapped)
{
    setData(createFloatDataUnit(pointType, pointAddress, value, order, swapped));
}

///
/// \brief ModbusMultiServer::readDouble
/// \param pointType
/// \param pointAddress
/// \param swapped
/// \return
///
double ModbusMultiServer::readDouble(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    const auto data = this->data(pointType, pointAddress, 4);
    return swapped ?  makeDouble(data.value(3), data.value(2), data.value(1), data.value(0), order):
                      makeDouble(data.value(0), data.value(1), data.value(2), data.value(3), order);
}

///
/// \brief ModbusMultiServer::writeDouble
/// \param pointType
/// \param pointAddress
/// \param value
/// \param swapped
///
void ModbusMultiServer::writeDouble(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, double value, ByteOrder order, bool swapped)
{
    setData(createDoubleDataUnit(pointType, pointAddress, value, order, swapped));
}

///
/// \brief ModbusMultiServer::writeRegister
/// \param pointType
/// \param params
///
void ModbusMultiServer::writeRegister(QModbusDataUnit::RegisterType pointType, const ModbusWriteParams& params)
{
    QModbusDataUnit data;
    if(params.Value.userType() == qMetaTypeId<QVector<quint16>>())
    {
        switch (pointType)
        {
            case QModbusDataUnit::Coils:
            case QModbusDataUnit::DiscreteInputs:
                data = createDataUnit(pointType, params.Address - 1, params.Value.value<QVector<quint16>>());
            break;

            case QModbusDataUnit::InputRegisters:
            case QModbusDataUnit::HoldingRegisters:
                data = createDataUnit(pointType, params.Address - 1, params.Value.value<QVector<quint16>>());
            break;

            default:
            break;
        }
    }
    else
    {
        switch (pointType)
        {
            case QModbusDataUnit::Coils:
            case QModbusDataUnit::DiscreteInputs:
                data = createDataUnit(pointType, params.Address - 1, QVector<quint16>() << params.Value.toBool());
            break;

            case QModbusDataUnit::InputRegisters:
            case QModbusDataUnit::HoldingRegisters:
                switch(params.DisplayMode)
                {
                    case DataDisplayMode::Binary:
                    case DataDisplayMode::Decimal:
                    case DataDisplayMode::Integer:
                    case DataDisplayMode::Hex:
                        data = createDataUnit(pointType, params.Address - 1, QVector<quint16>() << params.Value.toUInt());
                    break;
                    case DataDisplayMode::FloatingPt:
                        data = createFloatDataUnit(pointType, params.Address - 1, params.Value.toFloat(), params.Order, false);
                    break;
                    case DataDisplayMode::SwappedFP:
                        data = createFloatDataUnit(pointType, params.Address - 1, params.Value.toFloat(), params.Order, true);
                    break;
                    case DataDisplayMode::DblFloat:
                        data = createDoubleDataUnit(pointType, params.Address - 1, params.Value.toDouble(), params.Order, false);
                    break;
                    case DataDisplayMode::SwappedDbl:
                        data = createDoubleDataUnit(pointType, params.Address - 1, params.Value.toDouble(), params.Order, true);
                    break;
                }
            break;

            default:
            break;
        }
    }

    if(data.isValid())
        setData(data);
}

///
/// \brief ModbusMultiServer::simulateRegister
/// \param mode
/// \param pointType
/// \param pointAddress
/// \param params
///
void ModbusMultiServer::simulateRegister(DataDisplayMode mode, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, const ModbusSimulationParams& params)
{
    if(params.Mode != SimulationMode::No)
        _simulator->startSimulation(mode, pointType,pointAddress - 1, params);
    else
        _simulator->stopSimulation(pointType, pointAddress - 1);
}

///
/// \brief ModbusMultiServer::stopSimulation
/// \param pointType
/// \param pointAddress
///
void ModbusMultiServer::stopSimulation(QModbusDataUnit::RegisterType pointType, quint16 pointAddress)
{
    _simulator->stopSimulation(pointType, pointAddress - 1);
}

///
/// \brief ModbusMultiServer::stopSimulations
///
void ModbusMultiServer::stopSimulations()
{
    _simulator->stopSimulations();
}

///
/// \brief ModbusMultiServer::resumeSimulations
///
void ModbusMultiServer::resumeSimulations()
{
    _simulator->resumeSimulations();
}

///
/// \brief ModbusMultiServer::pauseSimulations
///
void ModbusMultiServer::pauseSimulations()
{
    _simulator->pauseSimulations();
}

///
/// \brief ModbusMultiServer::restartSimulations
///
void ModbusMultiServer::restartSimulations()
{
    _simulator->restartSimulations();
}

///
/// \brief ModbusServer::on_stateChanged
/// \param state
///
void ModbusMultiServer::on_stateChanged(QModbusDevice::State state)
{
    auto server = qobject_cast<QModbusServer*>(sender());
    const auto cd = server->property("ConnectionDetails").value<ConnectionDetails>();

    switch(state)
    {
        case QModbusDevice::ConnectedState:
            if(cd.Type == ConnectionType::Serial)
            {
                auto serialPort = qobject_cast<QSerialPort*>(server->device());

                const bool setDTR = server->property("DTRControl").toBool();
                serialPort->setDataTerminalReady(setDTR);

                if(serialPort->flowControl() != QSerialPort::HardwareControl)
                {
                    const bool setRTS = server->property("RTSControl").toBool();
                    serialPort->setRequestToSend(setRTS);
                }
            }
            _simulator->resumeSimulations();
            emit connected(cd);
        break;

        case QModbusDevice::UnconnectedState:
            if(!isConnected())
                _simulator->pauseSimulations();

            emit disconnected(cd);
        break;

        default:
        break;
    }
}

///
/// \brief ModbusMultiServer::on_errorOccured
/// \param error
///
void ModbusMultiServer::on_errorOccurred(QModbusDevice::Error error)
{
    if(error == QModbusDevice::ConnectionError)
    {
        auto server = qobject_cast<QModbusServer*>(sender());
        const auto errorString = server->errorString();

        server->disconnectDevice();
        emit connectionError(QString(tr("Connection error. %1")).arg(errorString));
    }
}

///
/// \brief ModbusMultiServer::on_dataWritten
/// \param table
/// \param address
/// \param size
///
void ModbusMultiServer::on_dataWritten(QModbusDataUnit::RegisterType table, int address, int size)
{
    auto server = qobject_cast<QModbusServer*>(sender());

    QModbusDataUnit data;
    data.setRegisterType(table);
    data.setStartAddress(address);
    data.setValueCount(size);
    server->data(&data);

    if(table == QModbusDataUnit::Coils ||
       table == QModbusDataUnit::DiscreteInputs)
    {
        auto values = data.values();
        for(auto& v : values) v = !!v; // force values to bit
        data.setValues(values);
    }

    setData(data);
}

///
/// \brief ModbusMultiServer::on_dataSimulated
/// \param mode
/// \param type
/// \param addr
/// \param value
///
void ModbusMultiServer::on_dataSimulated(DataDisplayMode mode, QModbusDataUnit::RegisterType type, quint16 addr, QVariant value)
{
    if(!isConnected())
        return;

    ModbusWriteParams params = { quint16(addr + 1), value, mode };
    writeRegister(type, params);
}
