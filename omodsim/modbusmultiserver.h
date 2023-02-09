#ifndef MODBUSMULTISERVER_H
#define MODBUSMULTISERVER_H

#include <QObject>
#include <QModbusServer>
#include <QModbusTcpServer>
#include "datasimulator.h"
#include "modbuswriteparams.h"
#include "connectiondetails.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    #include <QModbusRtuSerialSlave>
    typedef QModbusRtuSerialSlave QModbusRtuSerialServer;
#else
    #include <QModbusRtuSerialServer>
#endif

///
/// \brief The ModbusTcpServer class
///
class ModbusTcpServer : public QModbusTcpServer
{
    Q_OBJECT

public:
    explicit ModbusTcpServer(QObject *parent = nullptr)
        : QModbusTcpServer(parent)
    {
    }

signals:
    void request(const QModbusRequest& req);
    void response(const QModbusResponse& resp);

protected:
    QModbusResponse processRequest(const QModbusPdu &req) override
    {
        emit request(req);
        auto resp = QModbusTcpServer::processRequest(req);
        emit response(resp);
        return resp;
    }
    QModbusResponse processPrivateRequest(const QModbusPdu &req) override
    {
        emit request(req);
        auto resp = QModbusTcpServer::processPrivateRequest(req);
        emit response(resp);
        return resp;
    }
};

///
/// \brief The ModbusRtuServer class
///
class ModbusRtuServer : public QModbusRtuSerialServer
{
    Q_OBJECT

public:
    explicit ModbusRtuServer(QObject *parent = nullptr)
        : QModbusRtuSerialServer(parent)
    {
    }

signals:
    void request(const QModbusRequest& req);
    void response(const QModbusResponse& resp);

protected:
    QModbusResponse processRequest(const QModbusPdu &req) override
    {
        emit request(req);
        auto resp = QModbusRtuSerialServer::processRequest(req);
        emit response(resp);
        return resp;
    }
    QModbusResponse processPrivateRequest(const QModbusPdu &req) override
    {
        emit request(req);
        auto resp = QModbusRtuSerialServer::processPrivateRequest(req);
        emit response(resp);
        return resp;
    }
};

///
/// \brief The ModbusMultiServer class
///
class ModbusMultiServer final : public QObject
{
    Q_OBJECT
public:
    explicit ModbusMultiServer(QObject *parent = nullptr);
    ~ModbusMultiServer() override;

    quint8 deviceId() const;
    void setDeviceId(quint8 deviceId);

    void addUnitMap(int id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length);
    void removeUnitMap(int id);

    void connectDevice(const ConnectionDetails& cd);
    void disconnectDevice(ConnectionType type, const QString& port);

    QList<ConnectionDetails> connections() const;

    bool isConnected() const;
    bool isConnected(ConnectionType type, const QString& port) const;
    QModbusDevice::State state(ConnectionType type, const QString& port) const;

    QModbusDataUnit data(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;
    void setData(const QModbusDataUnit& data);

    void writeValue(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 value);

    float readFloat(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, bool swapped);
    void writeFloat(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, float value, bool swapped);

    double readDouble(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, bool swapped);
    void writeDouble(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, double value, bool swapped);

    void writeRegister(QModbusDataUnit::RegisterType pointType, const ModbusWriteParams& params);
    void simulateRegister(DataDisplayMode mode, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, const ModbusSimulationParams& params);
    void stopSimulations();

signals:
    void connected(const ConnectionDetails& cd);
    void disconnected(const ConnectionDetails& cd);
    void deviceIdChanged(quint8 deviceId);
    void request(const QModbusRequest& req);
    void response(const QModbusResponse& resp);
    void connectionError(const QString& error);

private slots:
    void on_stateChanged(QModbusDevice::State state);
    void on_errorOccurred(QModbusDevice::Error error);
    void on_dataWritten(QModbusDataUnit::RegisterType table, int address, int size);

private:
    QModbusDataUnitMap createDataUnitMap();
    QSharedPointer<QModbusServer> findModbusServer(const ConnectionDetails& cd) const;
    QSharedPointer<QModbusServer> findModbusServer(ConnectionType type, const QString& port) const;
    QSharedPointer<QModbusServer> createModbusServer(const ConnectionDetails& cd);

    void reconfigureServers();
    void addModbusServer(QSharedPointer<QModbusServer> server);
    void removeModbusServer(QSharedPointer<QModbusServer> server);

private:
    quint8 _deviceId;
    QMap<int, QModbusDataUnit> _modbusDataUnitMap;
    QList<QSharedPointer<QModbusServer>> _modbusServerList;
    QSharedPointer<DataSimulator> _simulator;
};

#endif // MODBUSMULTISERVER_H
