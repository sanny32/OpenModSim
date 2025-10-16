#ifndef MODBUSMULTISERVER_H
#define MODBUSMULTISERVER_H

#include <QThread>
#include <QModbusServer>
#include "modbusdataunitmap.h"
#include "modbuswriteparams.h"
#include "connectiondetails.h"
#include "modbusmessage.h"
#include "modbusserver.h"

///
/// \brief The ModbusMultiServer class
///
class ModbusMultiServer final : public QObject
{
    Q_OBJECT
public:
    enum class GlobalUnitMapStatus {
        NotSet = 0,
        CompletelySet = 1,
        PartiallySet,
    };
    Q_ENUM(GlobalUnitMapStatus)

    explicit ModbusMultiServer(QObject *parent = nullptr);
    ~ModbusMultiServer() override;

    void addDeviceId(quint8 deviceId);
    void removeDeviceId(quint8 deviceId);

    GlobalUnitMapStatus useGlobalUnitMap() const;
    void setUseGlobalUnitMap(bool use);

    void addUnitMap(int id, quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length);
    void removeUnitMap(int id, quint8 deviceId);

    void connectDevice(const ConnectionDetails& cd);
    void disconnectDevice(ConnectionType type, const QString& port);
    void closeConnections();

    QList<ConnectionDetails> connections() const;

    ModbusDefinitions getModbusDefinitions(const ConnectionDetails& cd) const;
    void setModbusDefinitions(const ConnectionDetails& cd, const ModbusDefinitions& md);

    bool isConnected() const;
    bool isConnected(ConnectionType type, const QString& port) const;
    QModbusDevice::State state(ConnectionType type, const QString& port) const;

    QModbusDataUnit data(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;
    void setData(quint8 deviceId, const QModbusDataUnit& data);

    void writeValue(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 value, ByteOrder order);
    void writeRegister(quint8 deviceId, QModbusDataUnit::RegisterType pointType, const ModbusWriteParams& params);

    qint32 readInt32(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeInt32(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, qint32 value, ByteOrder order, bool swapped);

    quint32 readUInt32(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeUInt32(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint32 value, ByteOrder order, bool swapped);

    qint64 readInt64(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeInt64(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, qint64 value, ByteOrder order, bool swapped);

    quint64 readUInt64(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeUInt64(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint64 value, ByteOrder order, bool swapped);

    float readFloat(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeFloat(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, float value, ByteOrder order, bool swapped);

    double readDouble(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, ByteOrder order, bool swapped);
    void writeDouble(quint8 deviceId, QModbusDataUnit::RegisterType pointType, quint16 pointAddress, double value, ByteOrder order, bool swapped);

signals:
    void connected(const ConnectionDetails& cd);
    void disconnected(const ConnectionDetails& cd);
    void request(QSharedPointer<const ModbusMessage> msg);
    void response(QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp);
    void connectionError(const QString& error);
    void errorOccured(quint8 deviceId, const QString& error);
    void dataChanged(quint8 deviceId, const QModbusDataUnit& data);

private slots:
    void on_stateChanged(QModbusDevice::State state);
    void on_errorOccurred(QModbusDevice::Error error, int deviceId);
    void on_dataWritten(int deviceId, QModbusDataUnit::RegisterType table, int address, int size);

private:
    QSharedPointer<ModbusServer> findModbusServer(const ConnectionDetails& cd) const;
    QSharedPointer<ModbusServer> findModbusServer(ConnectionType type, const QString& port) const;
    QSharedPointer<ModbusServer> createModbusServer(const ConnectionDetails& cd);

    void reconfigureServers();
    void addModbusServer(QSharedPointer<ModbusServer> server);
    void removeModbusServer(QSharedPointer<ModbusServer> server);

private:
    QList<int> _deviceIds;
    QThread* _workerThread;
    QMap<int, ModbusDataUnitMap> _modbusDataUnitMaps;
    QList<QSharedPointer<ModbusServer>> _modbusServerList;
};

#endif // MODBUSMULTISERVER_H
