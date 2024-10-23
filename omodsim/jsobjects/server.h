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

namespace Address
{
    Q_NAMESPACE
    enum class Base {
        Base0 = 0,
        Base1
    };
    Q_ENUM_NS(Base)
}
Q_DECLARE_METATYPE(Address::Base)

///
/// \brief The Modbus Server class
///
class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(ModbusMultiServer* server, const ByteOrder* order, AddressBase base);
    ~Server() override;

    Q_PROPERTY(Address::Base addressBase READ addressBase WRITE setAddressBase);

    Address::Base addressBase() const;

    Q_INVOKABLE quint16 readHolding(quint16 address) const;
    Q_INVOKABLE void writeHolding(quint16 address, quint16 value);

    Q_INVOKABLE quint16 readInput(quint16 address) const;
    Q_INVOKABLE void writeInput(quint16 address, quint16 value);

    Q_INVOKABLE bool readDiscrete(quint16 address) const;
    Q_INVOKABLE void writeDiscrete(quint16 address, bool value);

    Q_INVOKABLE bool readCoil(quint16 address) const;
    Q_INVOKABLE void writeCoil(quint16 address, bool value);

    Q_INVOKABLE qint32 readInt32(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeInt32(Register::Type reg, quint16 address, qint32 value, bool swapped);

    Q_INVOKABLE quint32 readUInt32(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeUInt32(Register::Type reg, quint16 address, quint32 value, bool swapped);

    Q_INVOKABLE qint64 readInt64(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeInt64(Register::Type reg, quint16 address, qint64 value, bool swapped);

    Q_INVOKABLE quint64 readUInt64(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeUInt64(Register::Type reg, quint16 address, quint64 value, bool swapped);

    Q_INVOKABLE float readFloat(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeFloat(Register::Type reg, quint16 address, float value, bool swapped);

    Q_INVOKABLE double readDouble(Register::Type reg, quint16 address, bool swapped) const;
    Q_INVOKABLE void writeDouble(Register::Type reg, quint16 address, double value, bool swapped);

    Q_INVOKABLE void onChange(Register::Type reg, quint16 address, const QJSValue& func);

public slots:
    void setAddressBase(Address::Base base);

private slots:
    void on_dataChanged(const QModbusDataUnit& data);

private:
    Address::Base _addressBase;
    const ByteOrder* _byteOrder;
    ModbusMultiServer* _mbMultiServer;
    QMap<QPair<Register::Type, quint16>, QJSValue> _mapOnChange;
};

#endif // SERVER_H
