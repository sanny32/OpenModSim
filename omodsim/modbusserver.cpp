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

    QModbusDataUnit data;
    data.setRegisterType(dd.PointType);
    data.setStartAddress(dd.PointAddress - 1);
    data.setValueCount(dd.Length);
    _modbusServer->setData(data);
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

    QModbusDataUnit data;
    _modbusServer->data(&data);

    return data;
}
