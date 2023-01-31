#ifndef MODBUSSERVER_H
#define MODBUSSERVER_H

#include <QObject>
#include <QModbusServer>
#include "displaydefinition.h"
#include "connectiondetails.h"

class ModbusServer : public QObject
{
    Q_OBJECT
public:
    explicit ModbusServer(QObject *parent = nullptr);
    ~ModbusServer() override;

    void create(const ConnectionDetails& cd, const DisplayDefinition& dd);
    void reconfigure(const DisplayDefinition& dd);

    void connectDevice();
    void disconnectDevice();

    bool isValid() const;
    QModbusDevice::State state() const;

    QModbusDataUnit data() const;

signals:
    void connected();
    void disconnected();

private slots:
    void on_mbStateChanged(QModbusDevice::State state);

private:
    QModbusServer* _modbusServer;
};

#endif // MODBUSSERVER_H
