#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QJSValue>
#include "modbusmultiserver.h"

namespace Register
{
    Q_NAMESPACE
    enum class Type {
        DiscreteInputs = 1,
        Coils,
        Input,
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
    explicit Server(ModbusMultiServer* server, const ByteOrder* order);
    ~Server() override;

    Q_INVOKABLE quint16 readHolding(quint16 address) const;
    Q_INVOKABLE void writeHolding(quint16 address, quint16 value);

    Q_INVOKABLE quint16 readInput(quint16 address) const;
    Q_INVOKABLE void writeInput(quint16 address, quint16 value);

    Q_INVOKABLE bool readDiscrete(quint16 address) const;
    Q_INVOKABLE void writeDiscrete(quint16 address, bool value);

    Q_INVOKABLE bool readCoil(quint16 address) const;
    Q_INVOKABLE void writeCoil(quint16 address, bool value);

    Q_INVOKABLE qint32 readLong(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeLong(Register::Type reg, quint16 address, qint32 value, bool swapped);

    Q_INVOKABLE quint32 readUnsignedLong(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeUnsignedLong(Register::Type reg, quint16 address, quint32 value, bool swapped);

    Q_INVOKABLE float readFloat(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeFloat(Register::Type reg, quint16 address, float value, bool swapped);

    Q_INVOKABLE double readDouble(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeDouble(Register::Type reg, quint16 address, double value, bool swapped);

    Q_INVOKABLE void onChange(Register::Type reg, quint16 address, const QJSValue& func);

private slots:
    void on_dataChanged(const QModbusDataUnit& data);

private:
    const ByteOrder* _byteOrder;
    ModbusMultiServer* _mbMultiServer;
    QMap<QPair<Register::Type, quint16>, QJSValue> _mapOnChange;
};

#endif // SERVER_H
