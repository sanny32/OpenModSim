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
    Q_PROPERTY(bool useGlobalUnitMap READ useGlobalUnitMap WRITE setUseGlobalUnitMap);

    Address::Base addressBase() const;
    bool useGlobalUnitMap() const;

    Q_INVOKABLE quint16 readHolding(quint16 address, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeHolding(quint16 address, quint16 value, quint8 deviceId = 1);

    Q_INVOKABLE quint16 readInput(quint16 address, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeInput(quint16 address, quint16 value, quint8 deviceId = 1);

    Q_INVOKABLE bool readDiscrete(quint16 address, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeDiscrete(quint16 address, bool value, quint8 deviceId = 1);

    Q_INVOKABLE bool readCoil(quint16 address, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeCoil(quint16 address, bool value, quint8 deviceId = 1);

    Q_INVOKABLE QString readAnsi(Register::Type reg, quint16 address, const QString& codepage, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeAnsi(Register::Type reg, quint16 address, const QString& value, const QString& codepage, quint8 deviceId = 1);

    Q_INVOKABLE qint32 readInt32(Register::Type reg, quint16 address, bool swapped, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeInt32(Register::Type reg, quint16 address, qint32 value, bool swapped, quint8 deviceId = 1);

    Q_INVOKABLE quint32 readUInt32(Register::Type reg, quint16 address, bool swapped, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeUInt32(Register::Type reg, quint16 address, quint32 value, bool swapped, quint8 deviceId = 1);

    Q_INVOKABLE qint64 readInt64(Register::Type reg, quint16 address, bool swapped, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeInt64(Register::Type reg, quint16 address, qint64 value, bool swapped, quint8 deviceId = 1);

    Q_INVOKABLE quint64 readUInt64(Register::Type reg, quint16 address, bool swapped, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeUInt64(Register::Type reg, quint16 address, quint64 value, bool swapped, quint8 deviceId = 1);

    Q_INVOKABLE float readFloat(Register::Type reg, quint16 address, bool swapped, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeFloat(Register::Type reg, quint16 address, float value, bool swapped, quint8 deviceId = 1);

    Q_INVOKABLE double readDouble(Register::Type reg, quint16 address, bool swapped, quint8 deviceId = 1) const;
    Q_INVOKABLE void writeDouble(Register::Type reg, quint16 address, double value, bool swapped, quint8 deviceId = 1);

    Q_INVOKABLE void onChange(quint8 deviceId, Register::Type reg, quint16 address, const QJSValue& func);

public slots:
    void setAddressBase(Address::Base base);
    void setUseGlobalUnitMap(bool value);

private slots:
    void on_dataChanged(quint8 deviceId, const QModbusDataUnit& data);

private:
    struct KeyOnChange {
        quint8 DeviceId;
        Register::Type Type;
        quint16 Address;
        bool operator==(const KeyOnChange &other) const {
            return DeviceId == other.DeviceId && Type == other.Type && Address == other.Address;
        }
    };

    Address::Base _addressBase;
    const ByteOrder* _byteOrder;
    ModbusMultiServer* _mbMultiServer;
    QHash<KeyOnChange, QJSValue> _mapOnChange;

    friend uint qHash(const KeyOnChange &key, uint seed);
};

#endif // SERVER_H
