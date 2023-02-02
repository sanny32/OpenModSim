#ifndef MODBUSMULTISERVER_H
#define MODBUSMULTISERVER_H

#include <QObject>
#include <QModbusServer>
#include "connectiondetails.h"

class ModbusMultiServer : public QObject
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
    void disconnectDevices();

    bool isConnected() const;
    bool isConnected(ConnectionType type, const QString& port) const;
    QModbusDevice::State state(ConnectionType type, const QString& port) const;

    QModbusDataUnit data(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;

signals:
    void connected(const ConnectionDetails& cd);
    void disconnected(const ConnectionDetails& cd);

private slots:
    void on_stateChanged(QModbusDevice::State state);

private:
    QModbusDataUnitMap createDataUnitMap();
    QModbusServer* findModbusServer(const ConnectionDetails& cd) const;
    QModbusServer* findModbusServer(ConnectionType type, const QString& port) const;
    QModbusServer* createModbusServer(const ConnectionDetails& cd);

    void addModbusServer(QModbusServer* server);
    void removeModbusServer(QModbusServer* server);

private:
    QMap<int, QModbusDataUnit> _modbusMap;
    QList<QModbusServer*> _modbusServerList;
};

#endif // MODBUSMULTISERVER_H
