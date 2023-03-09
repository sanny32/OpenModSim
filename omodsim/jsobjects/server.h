#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include "modbusmultiserver.h"

///
/// \brief The Modbus Server class
///
class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(ModbusMultiServer& server);

    Q_INVOKABLE quint16 readHolding(quint16 address);
    Q_INVOKABLE void writeHolding(quint16 address, quint16 value);

    Q_INVOKABLE quint16 readInput(quint16 address);
    Q_INVOKABLE void writeInput(quint16 address, quint16 value);

    Q_INVOKABLE bool readDiscrete(quint16 address);
    Q_INVOKABLE void writeDiscrete(quint16 address, bool value);

    Q_INVOKABLE bool readCoil(quint16 address);
    Q_INVOKABLE void writeCoil(quint16 address, bool value);

private:
    ModbusMultiServer& _mbMultiServer;
};

#endif // SERVER_H
