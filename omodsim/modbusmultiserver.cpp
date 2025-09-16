#include "numericutils.h"
#include "modbusmultiserver.h"

///
/// \brief ModbusServer::ModbusServer
/// \param parent
///
ModbusMultiServer::ModbusMultiServer(QObject *parent)
    : QObject{parent}
    ,_deviceId(1)
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

    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, deviceId]() {
            setDeviceId(deviceId);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    _deviceId = deviceId;

    for(auto&& s : _modbusServerList)
        s->setServerAddress(deviceId);

    emit deviceIdChanged(deviceId);
}

///
/// \brief ModbusMultiServer::isBusy
/// \return
///
bool ModbusMultiServer::isBusy() const
{
    if(_modbusServerList.isEmpty())
        return false;

    return _modbusServerList.first()->value(QModbusServer::DeviceBusy) == 0xffff;
}

///
/// \brief ModbusMultiServer::setBusy
/// \param busy
///
void ModbusMultiServer::setBusy(bool busy)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, busy]() {
            setBusy(busy);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    for(auto&& s : _modbusServerList)
        s->setValue(QModbusServer::DeviceBusy, busy ? 0xffff : 0x0000);
}

///
/// \brief ModbusMultiServer::useGlobalUnitMap
/// \return
///
bool ModbusMultiServer::useGlobalUnitMap() const
{
    return _modbusDataUnitMap.isGlobalMap();
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

    _modbusDataUnitMap.setGlobalMap(use);
    reconfigureServers();
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
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, id, pointType, pointAddress, length]() {
            addUnitMap(id, pointType, pointAddress, length);
        }, Qt::BlockingQueuedConnection);
        return;
    }

    _modbusDataUnitMap.addUnitMap(id, pointType, pointAddress, length);
    reconfigureServers();
}

///
/// \brief ModbusMultiServer::removeUnitMap
/// \param id
///
void ModbusMultiServer::removeUnitMap(int id)
{
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, id]() {
            removeUnitMap(id);
        }, Qt::BlockingQueuedConnection);
        return;
    }

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
QSharedPointer<QModbusServer> ModbusMultiServer::createModbusServer(const ConnectionDetails& cd)
{
    if(QThread::currentThread() != _workerThread)
    {
        QSharedPointer<QModbusServer> result;
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
                modbusServer = QSharedPointer<QModbusServer>(new ModbusTcpServer());
                modbusServer->setProperty("ConnectionDetails", QVariant::fromValue(cd));
                modbusServer->setConnectionParameter(QModbusDevice::NetworkPortParameter, cd.TcpParams.ServicePort);
                modbusServer->setConnectionParameter(QModbusDevice::NetworkAddressParameter, cd.TcpParams.IPAddress);

                connect((ModbusTcpServer*)modbusServer.get(), &ModbusTcpServer::request, this, [&](const QModbusRequest& req, int transactionId)
                {
                    emit request(req, ModbusMessage::Tcp, transactionId);
                });
                connect((ModbusTcpServer*)modbusServer.get(), &ModbusTcpServer::response, this, [&](const QModbusRequest& req, const QModbusResponse& resp, int transactionId)
                {
                    emit response(req, resp, ModbusMessage::Tcp, transactionId);
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
                    emit request(req, ModbusMessage::Rtu, 0);
                });
                connect((ModbusRtuServer*)modbusServer.get(), &ModbusRtuServer::response, this, [&](const QModbusRequest& req, const QModbusResponse& resp)
                {
                    emit response(req, resp, ModbusMessage::Rtu, 0);
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
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this]() {
            reconfigureServers();
        }, Qt::BlockingQueuedConnection);
        return;
    }

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
    if(QThread::currentThread() != _workerThread)
    {
        QMetaObject::invokeMethod(this, [this, data]() {
            setData(data);
        }, Qt::BlockingQueuedConnection);
        return;
    }

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
void ModbusMultiServer::writeValue(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 value, ByteOrder order)
{
    auto data = QModbusDataUnit(pointType, pointAddress, 1);
    data.setValue(0, toByteOrderValue(value, order));
    setData(data);
}

///
/// \brief ModbusMultiServer::readInt32
/// \param pointType
/// \param pointAddress
/// \param order
/// \param swapped
/// \return
///
qint32 ModbusMultiServer::readInt32(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    const auto data = this->data(pointType, pointAddress, 2);
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
void ModbusMultiServer::writeInt32(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, qint32 value, ByteOrder order, bool swapped)
{
    setData(createInt32DataUnit(pointType, pointAddress, value, order, swapped));
}

///
/// \brief ModbusMultiServer::readUInt32
/// \param pointType
/// \param pointAddress
/// \param order
/// \param swapped
/// \return
///
quint32 ModbusMultiServer::readUInt32(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    return (quint32)readInt32(pointType, pointAddress, order, swapped);
}

///
/// \brief ModbusMultiServer::writeUInt32
/// \param pointType
/// \param pointAddress
/// \param value
/// \param order
/// \param swapped
///
void ModbusMultiServer::writeUInt32(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint32 value, ByteOrder order, bool swapped)
{
    writeInt32(pointType, pointAddress, value, order, swapped);
}

///
/// \brief ModbusMultiServer::readInt64
/// \param pointType
/// \param pointAddress
/// \param order
/// \param swapped
/// \return
///
qint64 ModbusMultiServer::readInt64(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    const auto data = this->data(pointType, pointAddress, 4);
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
void ModbusMultiServer::writeInt64(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, qint64 value, ByteOrder order, bool swapped)
{
    setData(createInt64DataUnit(pointType, pointAddress, value, order, swapped));
}

///
/// \brief ModbusMultiServer::readUInt64
/// \param pointType
/// \param pointAddress
/// \param order
/// \param swapped
/// \return
///
quint64 ModbusMultiServer::readUInt64(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped)
{
    return (quint64)readInt64(pointType, pointAddress, order, swapped);
}

///
/// \brief ModbusMultiServer::writeUInt64
/// \param pointType
/// \param pointAddress
/// \param value
/// \param order
/// \param swapped
///
void ModbusMultiServer::writeUInt64(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint64 value, ByteOrder order, bool swapped)
{
    writeInt64(pointType, pointAddress, value, order, swapped);
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
        setData(data);
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
