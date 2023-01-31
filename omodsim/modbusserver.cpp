#include <QModbusTcpServer>
#include "modbusserver.h"

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
ModbusServer::ModbusServer(QObject *parent)
    : QObject{parent}
    ,_modbusServer(nullptr)
{
}

///
/// \brief ModbusServer::~ModbusServer
///
ModbusServer::~ModbusServer()
{
    if(_modbusServer)
        delete _modbusServer;
}

///
/// \brief ModbusServer::create
/// \param dd
///
void ModbusServer::create(const ConnectionDetails& cd, const DisplayDefinition& dd)
{
    if(_modbusServer != nullptr)
    {
        delete _modbusServer;
        _modbusServer = nullptr;
    }

    switch(cd.Type)
    {
        case ConnectionType::Tcp:
        {
            _modbusServer = new QModbusTcpServer(this);
            _modbusServer->setServerAddress(dd.DeviceId);
            _modbusServer->setConnectionParameter(QModbusDevice::NetworkPortParameter, cd.TcpParams.ServicePort);
            _modbusServer->setConnectionParameter(QModbusDevice::NetworkAddressParameter, cd.TcpParams.IPAddress);
            connect(_modbusServer, &QModbusDevice::stateChanged, this, &ModbusServer::on_mbStateChanged);
        }
        break;

        case ConnectionType::Serial:
        {

        }
        break;
    }

    reconfigure(dd);
}

///
/// \brief ModbusServer::reconfigure
/// \param dd
///
void ModbusServer::reconfigure(const DisplayDefinition& dd)
{
    if(!_modbusServer)
        return;

    _modbusServer->setProperty("PointType", dd.PointType);
    _modbusServer->setProperty("PointAddress",  dd.PointAddress - 1);
    _modbusServer->setProperty("Length", dd.Length);

    QModbusDataUnitMap mbMap;
    mbMap.insert(dd.PointType, {dd.PointType, dd.PointAddress - 1, dd.Length});
    _modbusServer->setMap(mbMap);
}

///
/// \brief ModbusServer::connectDevice
/// \param cd
///
void ModbusServer::connectDevice()
{
    if(_modbusServer)
         _modbusServer->connectDevice();
}

///
/// \brief ModbusServer::disconnectDevice
///
void ModbusServer::disconnectDevice()
{
    if(_modbusServer)
        _modbusServer->disconnectDevice();
}

///
/// \brief ModbusServer::isValid
/// \return
///
bool ModbusServer::isValid() const
{
     return _modbusServer != nullptr;
}

///
/// \brief ModbusServer::state
/// \return
///
QModbusDevice::State ModbusServer::state() const
{
    if(_modbusServer)
        return _modbusServer->state();

    return QModbusDevice::UnconnectedState;
}

///
/// \brief ModbusServer::data
/// \return
///
QModbusDataUnit ModbusServer::data() const
{
    if(!_modbusServer)
        return QModbusDataUnit();

    const auto pointType = _modbusServer->property("PointType").value<QModbusDataUnit::RegisterType>();
    const auto pointAddress = _modbusServer->property("PointAddress").toUInt();
    const auto length = _modbusServer->property("Length").toUInt();

    QModbusDataUnit data;
    data.setRegisterType(pointType);
    data.setStartAddress(pointAddress);
    data.setValueCount(length);
    _modbusServer->data(&data);

    return data;
}

///
/// \brief ModbusServer::on_mbStateChanged
/// \param state
///
void ModbusServer::on_mbStateChanged(QModbusDevice::State state)
{
    switch(state)
    {
        case QModbusDevice::ConnectedState:
            emit connected();
        break;

        case QModbusDevice::UnconnectedState:
            emit disconnected();
        break;

        default:
        break;
    }
}
