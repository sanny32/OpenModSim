#ifndef MODBUSSERVER_H
#define MODBUSSERVER_H

#include <QObject>
#include <QModbusServer>

class ModbusServer : public QObject
{
    Q_OBJECT
public:
    explicit ModbusServer(QObject *parent = nullptr);

signals:

private:
    QModbusServer* _modbusServer;
};

#endif // MODBUSSERVER_H
