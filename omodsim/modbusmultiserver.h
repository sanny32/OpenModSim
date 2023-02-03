#ifndef MODBUSMULTISERVER_H
#define MODBUSMULTISERVER_H

#include <QObject>
#include <QModbusServer>
#include "modbuswriteparams.h"
#include "connectiondetails.h"

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

    bool isConnected() const;
    bool isConnected(ConnectionType type, const QString& port) const;
    QModbusDevice::State state(ConnectionType type, const QString& port) const;

    QModbusDataUnit data(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;
    void setData(const QModbusDataUnit& data);

    void writeRegister(QModbusDataUnit::RegisterType pointType, const ModbusWriteParams& params);

signals:
    void connected(const ConnectionDetails& cd);
    void disconnected(const ConnectionDetails& cd);
    void deviceIdChanged(quint8 deviceId);

private slots:
    void on_stateChanged(QModbusDevice::State state);
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
};

#endif // MODBUSMULTISERVER_H
