#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include "modbusmultiserver.h"

namespace Register
{
    Q_NAMESPACE
    enum class Type {
        Input = 3,
        Holding
    };
    Q_ENUM_NS(Type)
}
Q_DECLARE_METATYPE(Register::Type)

///
/// \brief The Modbus Server class
///
class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(ModbusMultiServer& server, const ByteOrder& order);

    Q_INVOKABLE quint16 readHolding(quint16 address) const;
    Q_INVOKABLE void writeHolding(quint16 address, quint16 value);

    Q_INVOKABLE quint16 readInput(quint16 address) const;
    Q_INVOKABLE void writeInput(quint16 address, quint16 value);

    Q_INVOKABLE bool readDiscrete(quint16 address) const;
    Q_INVOKABLE void writeDiscrete(quint16 address, bool value);

    Q_INVOKABLE bool readCoil(quint16 address) const;
    Q_INVOKABLE void writeCoil(quint16 address, bool value);

    Q_INVOKABLE float readFloat(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeFloat(Register::Type reg, quint16 address, float value, bool swapped);

    Q_INVOKABLE double readDouble(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeDouble(Register::Type reg, quint16 address, double value, bool swapped);

private:
    const ByteOrder& _byteOrder;
    ModbusMultiServer& _mbMultiServer;
};

#endif // SERVER_H
