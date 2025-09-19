#ifndef MODBUSMULTISERVER_H
#define MODBUSMULTISERVER_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include <QModbusServer>
#include <QModbusTcpServer>
#include "modbusdataunitmap.h"
#include "modbuswriteparams.h"
#include "connectiondetails.h"
#include "modbusmessage.h"

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
    void request(const QModbusRequest& req, int transactionId);
    void response(const QModbusRequest& req, const QModbusResponse& resp, int transactionId);

protected:
    QModbusResponse processRequest(const QModbusPdu &req) override
    {
        _transactionId++;

        emit request(req, _transactionId);
        auto resp = QModbusTcpServer::processRequest(req);
        emit response(req, resp, _transactionId);
        return resp;
    }
    QModbusResponse processPrivateRequest(const QModbusPdu &req) override
    {
        _transactionId++;

        emit request(req, _transactionId);
        auto resp = QModbusTcpServer::processPrivateRequest(req);
        emit response(req, resp, _transactionId);
        return resp;
    }

private:
    int _transactionId = 0;
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
    void response(const QModbusRequest& req, const QModbusResponse& resp);

protected:
    QModbusResponse processRequest(const QModbusPdu &req) override
    {
        emit request(req);
        auto resp = QModbusRtuSerialServer::processRequest(req);
        emit response(req, resp);
        return resp;
    }
    QModbusResponse processPrivateRequest(const QModbusPdu &req) override
    {
        emit request(req);
        auto resp = QModbusRtuSerialServer::processPrivateRequest(req);
        emit response(req, resp);
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

    bool isBusy() const;
    void setBusy(bool busy);

    bool useGlobalUnitMap() const;
    void setUseGlobalUnitMap(bool use);

    void addUnitMap(int id, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length);
    void removeUnitMap(int id);

    void connectDevice(const ConnectionDetails& cd);
    void disconnectDevice(ConnectionType type, const QString& port);
    void closeConnections();

    QList<ConnectionDetails> connections() const;

    bool isConnected() const;
    bool isConnected(ConnectionType type, const QString& port) const;
    QModbusDevice::State state(ConnectionType type, const QString& port) const;

    QModbusDataUnit data(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;
    void setData(const QModbusDataUnit& data);

    void writeValue(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 value, ByteOrder order);
    void writeRegister(QModbusDataUnit::RegisterType pointType, const ModbusWriteParams& params);

    qint32 readInt32(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeInt32(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, qint32 value, ByteOrder order, bool swapped);

    quint32 readUInt32(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeUInt32(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint32 value, ByteOrder order, bool swapped);

    qint64 readInt64(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeInt64(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, qint64 value, ByteOrder order, bool swapped);

    quint64 readUInt64(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeUInt64(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint64 value, ByteOrder order, bool swapped);

    float readFloat(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeFloat(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, float value, ByteOrder order, bool swapped);

    double readDouble(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeDouble(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, double value, ByteOrder order, bool swapped);

signals:
    void connected(const ConnectionDetails& cd);
    void disconnected(const ConnectionDetails& cd);
    void deviceIdChanged(quint8 deviceId);
    void request(const QModbusRequest& req, ModbusMessage::ProtocolType protocol, int transactionId);
    void response(const QModbusRequest& req, const QModbusResponse& resp, ModbusMessage::ProtocolType protocol, int transactionId);
    void connectionError(const QString& error);
    void dataChanged(const QModbusDataUnit& data);

private slots:
    void on_stateChanged(QModbusDevice::State state);
    void on_errorOccurred(QModbusDevice::Error error);
    void on_dataWritten(QModbusDataUnit::RegisterType table, int address, int size);

private:
    QSharedPointer<QModbusServer> findModbusServer(const ConnectionDetails& cd) const;
    QSharedPointer<QModbusServer> findModbusServer(ConnectionType type, const QString& port) const;
    QSharedPointer<QModbusServer> createModbusServer(const ConnectionDetails& cd);

    void reconfigureServers();
    void addModbusServer(QSharedPointer<QModbusServer> server);
    void removeModbusServer(QSharedPointer<QModbusServer> server);

private:
    quint8 _deviceId;
    QThread* _workerThread;
    ModbusDataUnitMap _modbusDataUnitMap;
    QList<QSharedPointer<QModbusServer>> _modbusServerList;
};
Q_DECLARE_METATYPE(QModbusRequest)
Q_DECLARE_METATYPE(QModbusResponse)

#endif // MODBUSMULTISERVER_H
