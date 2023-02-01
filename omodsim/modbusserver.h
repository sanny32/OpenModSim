#ifndef MODBUSSERVER_H
#define MODBUSSERVER_H

#include <QObject>
#include <QModbusServer>
#include "connectiondetails.h"

class ModbusServer : public QObject
{
    Q_OBJECT
public:
    explicit ModbusServer(const ConnectionDetails& cd, QObject *parent = nullptr);
    ~ModbusServer() override;

    quint8 deviceId() const;
    void setDeviceId(quint8 deviceId);

    void addUnitMap(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length);
    void removeUnitMap(QModbusDataUnit::RegisterType pointType);

    void connectDevice();
    void disconnectDevice();

    bool isValid() const;
    QModbusDevice::State state() const;

    QModbusDataUnit data(QModbusDataUnit::RegisterType pointType, quint16 pointAddress, quint16 length) const;

signals:
    void connected();
    void disconnected();

private slots:
    void on_mbStateChanged(QModbusDevice::State state);

private:
    QModbusDataUnitMap _modbusMap;
    QModbusServer* _modbusServer;
};

#endif // MODBUSSERVER_H
