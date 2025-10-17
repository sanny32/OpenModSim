#include "numericutils.h"
#include "modbustcpserver.h"
#include "modbusrtuserialserver.h"
#include "modbusmultiserver.h"

///
/// \brief ModbusServer::ModbusServer
/// \param parent
///
ModbusMultiServer::ModbusMultiServer(QObject *parent)
    : QObject{parent}
    ,_workerThread(new QThread(this))
{
    moveToThread(_workerThread);
    _workerThread->start();

    connect(this, &QObject::destroyed, _workerThread, &QThread::quit);
}

///
/// \brief ModbusServer::~ModbusServer
///
ModbusMultiServer::~ModbusMultiServer()
{
    closeConnections();
    if(_workerThread && _workerThread->isRunning())
    {
       _workerThread->quit();
       _workerThread->wait();
    }
}

///
/// \brief ModbusServer::setDeviceId
/// \param deviceId
///
void ModbusMultiServer::addDeviceId(quint8 deviceId)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, deviceId]() {
            addDeviceId(deviceId);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    _deviceIds.append(deviceId);
    for(auto&& s : _modbusServerList)
        s->addServerAddress(deviceId);
}

///
/// \brief ModbusMultiServer::removeDeviceId
/// \param deviceId
///
void ModbusMultiServer::removeDeviceId(quint8 deviceId)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, deviceId]() {
            removeDeviceId(deviceId);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    _deviceIds.removeOne(deviceId);
    for(auto&& s : _modbusServerList) {
        s->removeServerAddress(deviceId);
    }
}

///
/// \brief ModbusMultiServer::useGlobalUnitMap
/// \return
///
bool ModbusMultiServer::useGlobalUnitMap() const
{
    if(_modbusDataUnitMaps.isEmpty())
        return false;

    return _modbusDataUnitMaps.first().isGlobalMap();
}

///
/// \brief ModbusMultiServer::setUseGlobalUnitMap
/// \param use
///
void ModbusMultiServer::setUseGlobalUnitMap(bool use)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, use]() {
            setUseGlobalUnitMap(use);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    QMapIterator it(_modbusDataUnitMaps);
    while(it.hasNext()) {
        auto item = it.next();
        _modbusDataUnitMaps[item.key()].setGlobalMap(use);
    }
    reconfigureServers();
}

///
/// \brief ModbusMultiServer::addUnitMap
/// \param id
/// \param pointType
/// \param pointAddress
/// \param length
///
void ModbusMultiServer::addUnitMap(int id, quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, id, deviceId, pointType, pointAddress, length]() {
            addUnitMap(id, deviceId, pointType, pointAddress, length);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    _modbusDataUnitMaps[deviceId].addUnitMap(id, pointType, pointAddress, length);
    reconfigureServers();
}

///
/// \brief ModbusMultiServer::removeUnitMap
/// \param id
///
void ModbusMultiServer::removeUnitMap(int id, quint8 deviceId)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, id, deviceId]() {
            removeUnitMap(id, deviceId);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    _modbusDataUnitMaps[deviceId].removeUnitMap(id);
    reconfigureServers();
}

///
/// \brief ModbusMultiServer::findModbusServer
/// \param cd
/// \return
///
QSharedPointer<ModbusServer> ModbusMultiServer::findModbusServer(const ConnectionDetails& cd) const
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
QSharedPointer<ModbusServer> ModbusMultiServer::findModbusServer(ConnectionType type, const QString& port) const
{
    for(auto&& s : _modbusServerList)
    {
        const auto cd = s->property("ConnectionDetails").value<ConnectionDetails>();
        if((cd.Type == ConnectionType::Tcp && type == ConnectionType::Tcp && port == QString("%1:%2").arg(cd.TcpParams.IPAddress, QString::number(cd.TcpParams.ServicePort))) ||
           (cd.Type == ConnectionType::Serial && type == ConnectionType::Serial && port == cd.SerialParams.PortName))
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
QSharedPointer<ModbusServer> ModbusMultiServer::createModbusServer(const ConnectionDetails& cd)
{
    if(QThread::currentThread() != _workerThread)
    {
        QSharedPointer<ModbusServer> result;
        QMetaObject::invokeMethod(this, [&]() {
            result = createModbusServer(cd);
        }, Qt::BlockingQueuedConnection);

        return result;
    }

    auto modbusServer = findModbusServer(cd);
    if(modbusServer == nullptr)
    {
        switch(cd.Type)
        {
            case ConnectionType::Tcp:
            {
                modbusServer = QSharedPointer<ModbusServer>(new ModbusTcpServer());
                modbusServer->setProperty("ConnectionDetails", QVariant::fromValue(cd));
                modbusServer->setConnectionParameter(QModbusDevice::NetworkPortParameter, cd.TcpParams.ServicePort);
                modbusServer->setConnectionParameter(QModbusDevice::NetworkAddressParameter, cd.TcpParams.IPAddress);
            }
            break;

            case ConnectionType::Serial:
            {
                modbusServer = QSharedPointer<ModbusServer>(new ModbusRtuSerialServer());
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

    if(modbusServer)
    {
        connect(modbusServer.get(), &ModbusServer::modbusRequest, this, &ModbusMultiServer::request);
        connect(modbusServer.get(), &ModbusServer::modbusResponse, this, &ModbusMultiServer::response);
        connect(modbusServer.get(), &ModbusServer::dataWritten, this, &ModbusMultiServer::on_dataWritten);
        connect(modbusServer.get(), &ModbusServer::stateChanged, this, &ModbusMultiServer::on_stateChanged);
        connect(modbusServer.get(), &ModbusServer::errorOccurred, this, &ModbusMultiServer::on_errorOccurred);
    }

    return modbusServer;
}

///
/// \brief ModbusServer::connectDevice
/// \param cd
///
void ModbusMultiServer::connectDevice(const ConnectionDetails& cd)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, cd]() {
            connectDevice(cd);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    auto modbusServer = findModbusServer(cd);
    if(modbusServer == nullptr)
    {
        modbusServer = createModbusServer(cd);
        addModbusServer(modbusServer);
    }

    modbusServer->removeAllServerAddresses();
    for(auto deviceId: _deviceIds) {
        modbusServer->addServerAddress(deviceId);
        modbusServer->setMap(_modbusDataUnitMaps[deviceId], deviceId);

        for(auto data : _modbusDataUnitMaps[deviceId]) {
            _modbusServerList.first()->data(&data, deviceId);
            modbusServer->setData(data, deviceId);
        }
    }

    modbusServer->setDefinitions(_definitions);
    modbusServer->connectDevice();
}

///
/// \brief ModbusMultiServer::disconnectDevice
/// \param type
/// \param port
///
void ModbusMultiServer::disconnectDevice(ConnectionType type, const QString& port)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, type, port]() {
            disconnectDevice(type, port);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    auto modbusServer = findModbusServer(type, port);
    if(modbusServer != nullptr)
    {
        modbusServer->disconnectDevice();
        removeModbusServer(modbusServer);
    }
}

///
/// \brief ModbusMultiServer::closeConnections
///
void ModbusMultiServer::closeConnections()
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this]() {
            closeConnections();
        }, Qt::BlockingQueuedConnection);
        return;
    }

    for(auto&& s : _modbusServerList)
        s->disconnectDevice();
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
/// \brief ModbusMultiServer::getModbusDefinitions
/// \return
///
ModbusDefinitions ModbusMultiServer::getModbusDefinitions() const
{
    return _definitions;
}

///
/// \brief ModbusMultiServer::setModbusDefinitions
/// \param defs
///
void ModbusMultiServer::setModbusDefinitions(const ModbusDefinitions& defs)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, defs]() {
            setModbusDefinitions(defs);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    _definitions = defs;

    for(auto&& s : _modbusServerList) {
        s->setDefinitions(defs);
    }

    emit definitionsChanged(_definitions);
}

///
/// \brief ModbusMultiServer::addModbusServer
/// \param server
///
void ModbusMultiServer::addModbusServer(QSharedPointer<ModbusServer> server)
{
    if(server && !_modbusServerList.contains(server)) {
        _modbusServerList.push_back(server);
    }
}

///
/// \brief ModbusMultiServer::removeModbusServer
/// \param server
///
void ModbusMultiServer::removeModbusServer(QSharedPointer<ModbusServer> server)
{
    if(server) {
        _modbusServerList.removeOne(server);
    }
}

///
/// \brief ModbusMultiServer::reconfigureServers
///
void ModbusMultiServer::reconfigureServers()
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this]() {
            reconfigureServers();
        }, Qt::BlockingQueuedConnection);
        return;
    }

    for(auto&& s : _modbusServerList) {
        s->setDefinitions(_definitions);
        for(auto addr : s->serverAddresses()) {
            s->setMap(_modbusDataUnitMaps[addr], addr);
        }
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
QModbusDataUnit ModbusMultiServer::data(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const
{
    return _modbusDataUnitMaps[deviceId].getData(pointType, pointAddress, length);
}

///
/// \brief ModbusMultiServer::setData
/// \param data
///
void ModbusMultiServer::setData(quint8 deviceId, const QModbusDataUnit& data)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, deviceId, data]() {
            setData(deviceId, data);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    if(!_modbusDataUnitMaps.contains(deviceId)) {
        emit errorOccured(deviceId, tr("An incorrect device id was specified (%1)").arg(deviceId));
        return;
    }

    _modbusDataUnitMaps[deviceId].setData(data);

    QString error;
    for(auto&& s : _modbusServerList)
    {
        s->blockSignals(true);

        if(!s->setData(data, deviceId))
            error = s->errorString(deviceId);

        s->blockSignals(false);
    }

    if(error.isEmpty())
        emit dataChanged(deviceId, data);
    else
        emit errorOccured(deviceId, error);
}

///
/// \brief createDataUnit
/// \param type
/// \param newStartAddress
/// \param values
/// \param order
/// \return
///
QModbusDataUnit createDataUnit(QModbusDataUnit::RegisterType type, int newStartAddress, const QVector<quint16>& values, ByteOrder order)
{
    auto data = QModbusDataUnit(type, newStartAddress, values.count());

    if(!values.isEmpty())
    {
        QVector<quint16> vv(values.size());
        for(int i = 0; i < vv.size(); i++)
            vv[i] = toByteOrderValue(values[i], order);

        data.setValues(values);
    }

    return data;
}

///
/// \brief createDataUnit
/// \param type
/// \param newStartAddress
/// \param value
/// \param order
/// \return
///
QModbusDataUnit createDataUnit(QModbusDataUnit::RegisterType type, int newStartAddress, quint16 value, ByteOrder order)
{
    auto data = QModbusDataUnit(type, newStartAddress, 1);
    data.setValue(0, toByteOrderValue(value, order));

    return data;
}

///
/// \brief createInt32DataUnit
/// \param type
/// \param newStartAddress
/// \param value
/// \param order
/// \param swapped
/// \return
///
QModbusDataUnit createInt32DataUnit(QModbusDataUnit::RegisterType type, int newStartAddress, qint32 value, ByteOrder order, bool swapped)
{
    Q_ASSERT(type == QModbusDataUnit::HoldingRegisters
             || type == QModbusDataUnit::InputRegisters);

    QVector<quint16> values(2);
    auto data = QModbusDataUnit(type, newStartAddress, 2);

    if(swapped)
        breakInt32(value, values[1], values[0], order);
    else
        breakInt32(value, values[0], values[1], order);

    data.setValues(values);
    return data;
}

///
/// \brief createInt64DataUnit
/// \param type
/// \param newStartAddress
/// \param value
/// \param order
/// \param swapped
/// \return
///
QModbusDataUnit createInt64DataUnit(QModbusDataUnit::RegisterType type, int newStartAddress, qint64 value, ByteOrder order, bool swapped)
{
    QVector<quint16> values(4);
    auto data = QModbusDataUnit(type, newStartAddress, 4);

    if(swapped)
        breakInt64(value, values[3], values[2], values[1], values[0], order);
    else
        breakInt64(value, values[0], values[1], values[2], values[3], order);

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
/// \param order
///
void ModbusMultiServer::writeValue(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 value, ByteOrder order)
{
    auto data = QModbusDataUnit(pointType, pointAddress, 1);
    data.setValue(0, toByteOrderValue(value, order));
    setData(deviceId, data);
}

///
/// \brief ModbusMultiServer::readInt32
/// \param pointType
/// \param pointAddress
/// \param order
/// \param swapped
/// \return
///
qint32 ModbusMultiServer::readInt32(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    const auto data = this->data(deviceId, pointType, pointAddress, 2);
    return swapped ?  makeInt32(data.value(1), data.value(0), order): makeInt32(data.value(0), data.value(1), order);
}

///
/// \brief ModbusMultiServer::writeInt32
/// \param pointType
/// \param pointAddress
/// \param value
/// \param order
/// \param swapped
///
void ModbusMultiServer::writeInt32(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, qint32 value, ByteOrder order, bool swapped)
{
    setData(deviceId, createInt32DataUnit(pointType, pointAddress, value, order, swapped));
}

///
/// \brief ModbusMultiServer::readUInt32
/// \param pointType
/// \param pointAddress
/// \param order
/// \param swapped
/// \return
///
quint32 ModbusMultiServer::readUInt32(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    return (quint32)readInt32(deviceId, pointType, pointAddress, order, swapped);
}

///
/// \brief ModbusMultiServer::writeUInt32
/// \param pointType
/// \param pointAddress
/// \param value
/// \param order
/// \param swapped
///
void ModbusMultiServer::writeUInt32(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint32 value, ByteOrder order, bool swapped)
{
    writeInt32(deviceId, pointType, pointAddress, value, order, swapped);
}

///
/// \brief ModbusMultiServer::readInt64
/// \param pointType
/// \param pointAddress
/// \param order
/// \param swapped
/// \return
///
qint64 ModbusMultiServer::readInt64(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    const auto data = this->data(deviceId, pointType, pointAddress, 4);
    return swapped ?  makeInt64(data.value(3), data.value(2), data.value(1), data.value(0), order):
               makeInt64(data.value(0), data.value(1), data.value(2), data.value(3), order);
}

///
/// \brief ModbusMultiServer::writeInt64
/// \param pointType
/// \param pointAddress
/// \param value
/// \param order
/// \param swapped
///
void ModbusMultiServer::writeInt64(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, qint64 value, ByteOrder order, bool swapped)
{
    setData(deviceId, createInt64DataUnit(pointType, pointAddress, value, order, swapped));
}

///
/// \brief ModbusMultiServer::readUInt64
/// \param pointType
/// \param pointAddress
/// \param order
/// \param swapped
/// \return
///
quint64 ModbusMultiServer::readUInt64(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    return (quint64)readInt64(deviceId, pointType, pointAddress, order, swapped);
}

///
/// \brief ModbusMultiServer::writeUInt64
/// \param pointType
/// \param pointAddress
/// \param value
/// \param order
/// \param swapped
///
void ModbusMultiServer::writeUInt64(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint64 value, ByteOrder order, bool swapped)
{
    writeInt64(deviceId, pointType, pointAddress, value, order, swapped);
}

///
/// \brief ModbusMultiServer::readFloat
/// \param pointType
/// \param pointAddress
/// \param order
/// \param swapped
/// \return
///
float ModbusMultiServer::readFloat(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    const auto data = this->data(deviceId, pointType, pointAddress, 2);
    return swapped ?  makeFloat(data.value(1), data.value(0), order): makeFloat(data.value(0), data.value(1), order);
}

///
/// \brief ModbusMultiServer::writeFloat
/// \param pointType
/// \param pointAddress
/// \param value
/// \param swapped
///
void ModbusMultiServer::writeFloat(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, float value, ByteOrder order, bool swapped)
{
    setData(deviceId, createFloatDataUnit(pointType, pointAddress, value, order, swapped));
}

///
/// \brief ModbusMultiServer::readDouble
/// \param pointType
/// \param pointAddress
/// \param swapped
/// \return
///
double ModbusMultiServer::readDouble(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    const auto data = this->data(deviceId, pointType, pointAddress, 4);
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
void ModbusMultiServer::writeDouble(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, double value, ByteOrder order, bool swapped)
{
    setData(deviceId, createDoubleDataUnit(pointType, pointAddress, value, order, swapped));
}

///
/// \brief ModbusMultiServer::writeRegister
/// \param pointType
/// \param params
///
void ModbusMultiServer::writeRegister(quint8 deviceId, QModbusDataUnit::RegisterType pointType, const ModbusWriteParams& params)
{
    QModbusDataUnit data;
    const auto addr = params.Address - (params.ZeroBasedAddress ? 0 : 1);
    if(params.Value.userType() == qMetaTypeId<QVector<quint16>>())
    {
        switch (pointType)
        {
            case QModbusDataUnit::Coils:
            case QModbusDataUnit::DiscreteInputs:
                data = createDataUnit(pointType, addr, params.Value.value<QVector<quint16>>(), params.Order);
            break;

            case QModbusDataUnit::InputRegisters:
            case QModbusDataUnit::HoldingRegisters:
                data = createDataUnit(pointType, addr, params.Value.value<QVector<quint16>>(), params.Order);
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
                data = createDataUnit(pointType, addr, params.Value.toBool(), params.Order);
            break;

            case QModbusDataUnit::InputRegisters:
            case QModbusDataUnit::HoldingRegisters:
                switch(params.DisplayMode)
                {
                    case DataDisplayMode::Binary:
                    case DataDisplayMode::UInt16:
                    case DataDisplayMode::Int16:
                    case DataDisplayMode::Hex:
                    case DataDisplayMode::Ansi:
                        data = createDataUnit(pointType, addr, params.Value.toUInt(), params.Order);
                    break;
                    case DataDisplayMode::FloatingPt:
                        data = createFloatDataUnit(pointType, addr, params.Value.toFloat(), params.Order, false);
                    break;
                    case DataDisplayMode::SwappedFP:
                        data = createFloatDataUnit(pointType, addr, params.Value.toFloat(), params.Order, true);
                    break;
                    case DataDisplayMode::DblFloat:
                        data = createDoubleDataUnit(pointType, addr, params.Value.toDouble(), params.Order, false);
                    break;
                    case DataDisplayMode::SwappedDbl:
                        data = createDoubleDataUnit(pointType, addr, params.Value.toDouble(), params.Order, true);
                    break;
                        
                    case DataDisplayMode::Int32:
                    case DataDisplayMode::UInt32:
                        data = createInt32DataUnit(pointType, addr, params.Value.toInt(), params.Order, false);
                    break;

                    case DataDisplayMode::SwappedInt32:
                    case DataDisplayMode::SwappedUInt32:
                        data = createInt32DataUnit(pointType, addr, params.Value.toInt(), params.Order, true);
                    break;

                    case DataDisplayMode::Int64:
                    case DataDisplayMode::UInt64:
                        data = createInt64DataUnit(pointType, addr, params.Value.toLongLong(), params.Order, false);
                        break;

                    case DataDisplayMode::SwappedInt64:
                    case DataDisplayMode::SwappedUInt64:
                        data = createInt64DataUnit(pointType, addr, params.Value.toLongLong(), params.Order, true);
                    break;
                }
            break;

            default:
            break;
        }
    }

    if(data.isValid())
        setData(deviceId, data);
}

///
/// \brief ModbusServer::on_stateChanged
/// \param state
///
void ModbusMultiServer::on_stateChanged(QModbusDevice::State state)
{
    auto server = qobject_cast<ModbusServer*>(sender());
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

///
/// \brief ModbusMultiServer::on_errorOccurred
/// \param error
/// \param deviceId
///
void ModbusMultiServer::on_errorOccurred(QModbusDevice::Error error, int deviceId)
{
    auto server = qobject_cast<ModbusServer*>(sender());
    const auto errorString = server->errorString(deviceId);

    if(error == QModbusDevice::ConnectionError) {
        server->disconnectDevice();
        emit connectionError(QString(tr("Connection error. %1")).arg(errorString));
    }
    else {
        emit errorOccured(deviceId, errorString);
    }
}

///
/// \brief ModbusMultiServer::on_dataWritten
/// \param deviceId
/// \param table
/// \param address
/// \param size
///
void ModbusMultiServer::on_dataWritten(int deviceId, QModbusDataUnit::RegisterType table, int address, int size)
{
    auto server = qobject_cast<ModbusServer*>(sender());

    QModbusDataUnit data;
    data.setRegisterType(table);
    data.setStartAddress(address);
    data.setValueCount(size);
    server->data(&data, deviceId);

    if(table == QModbusDataUnit::Coils ||
       table == QModbusDataUnit::DiscreteInputs)
    {
        auto values = data.values();
        for(auto& v : values) v = !!v; // force values to bit
        data.setValues(values);
    }

    setData(deviceId, data);
}
