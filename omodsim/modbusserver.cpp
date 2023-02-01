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
ModbusServer::ModbusServer(const ConnectionDetails& cd, QObject *parent)
    : QObject{parent}
    ,_modbusServer(nullptr)
{
    switch(cd.Type)
    {
        case ConnectionType::Tcp:
        {
            _modbusServer = new QModbusTcpServer(this);
            _modbusServer->setConnectionParameter(QModbusDevice::NetworkPortParameter, cd.TcpParams.ServicePort);
            _modbusServer->setConnectionParameter(QModbusDevice::NetworkAddressParameter, cd.TcpParams.IPAddress);
            connect(_modbusServer, &QModbusDevice::stateChanged, this, &ModbusServer::on_mbStateChanged);
        }
        break;

        case ConnectionType::Serial:
        {
            _modbusServer = new QModbusRtuSerialServer(this);
            _modbusServer->setConnectionParameter(QModbusDevice::SerialPortNameParameter, cd.SerialParams.PortName);
            _modbusServer->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, cd.SerialParams.BaudRate);
            _modbusServer->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, cd.SerialParams.WordLength);
            _modbusServer->setConnectionParameter(QModbusDevice::SerialParityParameter, cd.SerialParams.Parity);
            _modbusServer->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, cd.SerialParams.StopBits);
            connect(_modbusServer, &QModbusDevice::stateChanged, this, &ModbusServer::on_mbStateChanged);
        }
        break;
    }

    Q_ASSERT(_modbusServer != nullptr);
}

///
/// \brief ModbusServer::~ModbusServer
///
ModbusServer::~ModbusServer()
{
    disconnectDevice();
    delete _modbusServer;
}

///
/// \brief ModbusServer::deviceId
/// \return
///
quint8 ModbusServer::deviceId() const
{
    return _modbusServer->serverAddress();
}

///
/// \brief ModbusServer::setDeviceId
/// \param deviceId
///
void ModbusServer::setDeviceId(quint8 deviceId)
{
    _modbusServer->setServerAddress(deviceId);
}

///
/// \brief ModbusServer::addUnitMap
/// \param pointType
/// \param pointAddress
/// \param length
///
void ModbusServer::addUnitMap(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length)
{
    _modbusMap.remove(pointType);
    _modbusMap.insert(pointType, {pointType, pointAddress, length});

    _modbusServer->setMap(_modbusMap);
}

///
/// \brief ModbusServer::removeUnitMap
/// \param pointType
///
void ModbusServer::removeUnitMap(QModbusDataUnit::RegisterType pointType)
{
    _modbusMap.remove(pointType);
    _modbusServer->setMap(_modbusMap);
}

///
/// \brief ModbusServer::connectDevice
/// \param cd
///
void ModbusServer::connectDevice()
{
    _modbusServer->connectDevice();
}

///
/// \brief ModbusServer::disconnectDevice
///
void ModbusServer::disconnectDevice()
{
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
    return _modbusServer->state();
}

///
/// \brief ModbusServer::data
/// \param dd
/// \return
///
QModbusDataUnit ModbusServer::data(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const
{
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
