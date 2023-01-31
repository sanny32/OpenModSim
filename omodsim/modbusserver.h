#ifndef MODBUSSERVER_H
#define MODBUSSERVER_H

#include <QObject>
#include <QModbusServer>

class ModbusServer : public QObject
{
    Q_OBJECT
public:
    explicit ModbusServer(QObject *parent = nullptr);
    ~ModbusServer() override;

    bool isValid() const;
    QModbusDevice::State state() const;

signals:

private:
    QModbusServer* _modbusServer;
};

#endif // MODBUSSERVER_H
