#include <QModbusTcpServer>
#include "modbusmultiserver.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    #include <QModbusRtuSerialSlave>
    typedef QModbusRtuSerialSlave QModbusRtuSerialServer;
#else
    #include <QModbusRtuSerialServer>
#endif

///
/// \brief ModbusServer::ModbusServer
/// \param parent
///
ModbusMultiServer::ModbusMultiServer(QObject *parent)
    : QObject{parent}
    ,_deviceId(1)
{
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
    _deviceId = deviceId;

    for(auto&& s : _modbusServerList)
        s->setServerAddress(deviceId);

    emit deviceIdChanged(deviceId);
}

///
/// \brief ModbusServer::addUnitMap
/// \param pointType
/// \param pointAddress
/// \param length
///
void ModbusMultiServer::addUnitMap(int id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length)
{
    _modbusDataUnitMap.insert(id, {pointType, pointAddress, length});
    reconfigureServers();
}

///
/// \brief ModbusServer::removeUnitMap
/// \param pointType
///
void ModbusMultiServer::removeUnitMap(int id)
{
    _modbusDataUnitMap.remove(id);
    reconfigureServers();
}

///
/// \brief ModbusServer::createDataUnitMap
/// \return
///
QModbusDataUnitMap ModbusMultiServer::createDataUnitMap()
{
    QMultiMap<QModbusDataUnit::RegisterType, QModbusDataUnit> multimap;
    for(auto&& id : _modbusDataUnitMap.keys())
    {
        const auto unit = _modbusDataUnitMap[id];
        multimap.insert(unit.registerType(), unit);
    }

    QModbusDataUnitMap modbusMap;
    for(auto&& type: multimap.uniqueKeys())
    {
        quint16 startAddress = 65535;
        quint16 endAddress = 0;
        for(auto&& unit : multimap.values(type))
        {
            startAddress = qMin<quint16>(startAddress, unit.startAddress());
            endAddress = qMax<quint16>(endAddress, unit.startAddress() + unit.valueCount());
        }

        const quint16 length = endAddress - startAddress + 1;
        if(length > 0) modbusMap.insert(type, {type, startAddress, length});
    }
    return modbusMap;
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
                modbusServer = QSharedPointer<QModbusServer>(new QModbusTcpServer(this));
                modbusServer->setProperty("ConnectionDetails", QVariant::fromValue(cd));
                modbusServer->setConnectionParameter(QModbusDevice::NetworkPortParameter, cd.TcpParams.ServicePort);
                modbusServer->setConnectionParameter(QModbusDevice::NetworkAddressParameter, cd.TcpParams.IPAddress);
            }
            break;

            case ConnectionType::Serial:
            {
                modbusServer = QSharedPointer<QModbusServer>(new QModbusRtuSerialServer(this));
                modbusServer->setProperty("ConnectionDetails", QVariant::fromValue(cd));
                modbusServer->setProperty("DTRControl", cd.SerialParams.SetDTR);
                modbusServer->setProperty("RTSControl", cd.SerialParams.SetRTS);
                modbusServer->setConnectionParameter(QModbusDevice::SerialPortNameParameter, cd.SerialParams.PortName);
                modbusServer->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, cd.SerialParams.BaudRate);
                modbusServer->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, cd.SerialParams.WordLength);
                modbusServer->setConnectionParameter(QModbusDevice::SerialParityParameter, cd.SerialParams.Parity);
                modbusServer->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, cd.SerialParams.StopBits);
                qobject_cast<QSerialPort*>(modbusServer->device())->setFlowControl(cd.SerialParams.FlowControl);
            }
            break;
        }
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
    modbusServer->setMap(createDataUnitMap());
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
/// \brief ModbusMultiServer::addModbusServer
/// \param server
///
void ModbusMultiServer::addModbusServer(QSharedPointer<QModbusServer> server)
{
    if(server && !_modbusServerList.contains(server))
    {
        _modbusServerList.push_back(server);
        connect(server.get(), &QModbusDevice::stateChanged, this, &ModbusMultiServer::on_stateChanged);
    }
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
    if(!_modbusServerList.isEmpty())
    {
        const auto dataUintMap = createDataUnitMap();
        for(auto&& s : _modbusServerList)
            s->setMap(dataUintMap);
    }
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
    QModbusDataUnit data;
    data.setRegisterType(pointType);
    data.setStartAddress(pointAddress);
    data.setValueCount(length);

    if(!_modbusServerList.isEmpty())
        _modbusServerList.first()->data(&data);

    return data;
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
            emit connected(cd);
        break;

        case QModbusDevice::UnconnectedState:
            emit disconnected(cd);
        break;

        default:
        break;
    }
}
