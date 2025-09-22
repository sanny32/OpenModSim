#ifndef IMODBUSSERVER_H
#define IMODBUSSERVER_H

#include <deque>
#include <QLoggingCategory>
#include <QModbusServer>

enum Coil {
    On = 0xff00,
    Off = 0x0000
};

namespace Diagnostics {
enum SubFunctionCode {
    ReturnQueryData = 0x0000,
    RestartCommunicationsOption = 0x0001,
    ReturnDiagnosticRegister = 0x0002,
    ChangeAsciiInputDelimiter = 0x0003,
    ForceListenOnlyMode = 0x0004,
    ClearCountersAndDiagnosticRegister = 0x000a,
    ReturnBusMessageCount = 0x000b,
    ReturnBusCommunicationErrorCount = 0x000c,  // CRC error counter
    ReturnBusExceptionErrorCount = 0x000d,
    ReturnServerMessageCount = 0x000e,
    ReturnServerNoResponseCount = 0x000f,
    ReturnServerNAKCount = 0x0010,
    ReturnServerBusyCount = 0x0011,
    ReturnBusCharacterOverrunCount = 0x0012,
    ClearOverrunCounterAndFlag = 0x0014
};
}

namespace EncapsulatedInterfaceTransport {
enum SubFunctionCode {
    CanOpenGeneralReference = 0x0D,
    ReadDeviceIdentification = 0x0E
};
}

class QModbusCommEvent;

///
/// \brief The QCountedSet class
template<typename T>
class QCountedSet
{
public:
    void insert(int value) {
        _counts[value]++;
    }

    bool remove(int value) {
        auto it = _counts.find(value);
        if (it == _counts.end())
            return false;

        it.value()--;
        if (it.value() == 0)
            _counts.erase(it);
        return true;
    }

    int count(int value) const {
        return _counts.value(value, 0);
    }

    bool contains(int value) const {
        return _counts.contains(value);
    }

    QList<int> values() const {
        return _counts.keys();
    }

    int size() const {
        return _counts.size();
    }

    bool isEmpty() const {
        return _counts.isEmpty();
    }

private:
    QHash<T, int> _counts;
};

///
/// \brief The ModbusServer class
///
class ModbusServer : public QModbusServer
{
    Q_OBJECT

public:
    QList<int> serverAddresses() const;
    void addServerAddress(int serverAddress);
    void removeServerAddress(int serverAddress);

    bool setMap(const QModbusDataUnitMap &map, int serverAddress);

    QVariant value(int option, int serverAddress) const;
    bool setValue(int option, const QVariant &value, int serverAddress);

    bool data(QModbusDataUnit *newData, int serverAddress) const;
    bool setData(const QModbusDataUnit &unit, int serverAddress);

    bool setData(QModbusDataUnit::RegisterType table, quint16 address, quint16 data, int serverAddress);
    bool data(QModbusDataUnit::RegisterType table, quint16 address, quint16 *data, int serverAddress) const;

    virtual QVariant connectionParameter(ConnectionParameter parameter) const = 0;
    virtual void setConnectionParameter(ConnectionParameter parameter, const QVariant &value) = 0;

    bool connectDevice();
    void disconnectDevice();

    State state() const;

    Error error(int serverAddress) const;
    QString errorString(int serverAddress) const;

    virtual QIODevice *device() const = 0;

signals:
    void errorOccurred(QModbusDevice::Error error, int serverAddress);

protected:
    enum Counter {
        CommEvent = 0x0001,
        BusMessage = Diagnostics::ReturnBusMessageCount,
        BusCommunicationError = Diagnostics::ReturnBusCommunicationErrorCount,
        BusExceptionError = Diagnostics::ReturnBusExceptionErrorCount,
        ServerMessage = Diagnostics::ReturnServerMessageCount,
        ServerNoResponse = Diagnostics::ReturnServerNoResponseCount,
        ServerNAK = Diagnostics::ReturnServerNAKCount,
        ServerBusy = Diagnostics::ReturnServerBusyCount,
        BusCharacterOverrun = Diagnostics::ReturnBusCharacterOverrunCount
    };

    ///
    /// \brief ModbusServer
    /// \param parent
    ///
    explicit ModbusServer(QObject *parent = nullptr);

    void setState(QModbusDevice::State newState);
    void setError(const QString &errorText, QModbusDevice::Error error);
    void setError(const QString &errorText, QModbusDevice::Error error, int serverAddress);

    virtual bool writeData(const QModbusDataUnit &unit, int serverAddress);
    virtual bool readData(QModbusDataUnit *newData, int serverAddress) const;

    virtual QModbusResponse processRequest(const QModbusPdu &request, int serverAddress);
    virtual QModbusResponse processPrivateRequest(const QModbusPdu &request, int serverAddress);

    void resetCommunicationCounters(int serverAddress) { _counters[serverAddress].fill(0u); }
    void incrementCounter(ModbusServer::Counter counter, int serverAddress) { _counters[serverAddress][counter]++; }

    QModbusResponse processReadCoilsRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processReadDiscreteInputsRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse readBits(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType, int serverAddress);

    QModbusResponse processReadHoldingRegistersRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processReadInputRegistersRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse readBytes(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType, int serverAddress);

    QModbusResponse processWriteSingleCoilRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processWriteSingleRegisterRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse writeSingle(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType, int serverAddress);

    QModbusResponse processReadExceptionStatusRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processDiagnosticsRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processGetCommEventCounterRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processGetCommEventLogRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processWriteMultipleCoilsRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processWriteMultipleRegistersRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processReportServerIdRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processMaskWriteRegisterRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processReadWriteMultipleRegistersRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processReadFifoQueueRequest(const QModbusRequest &request, int serverAddress);
    QModbusResponse processEncapsulatedInterfaceTransportRequest(const QModbusRequest &request, int serverAddress);

    void storeModbusCommEvent(const QModbusCommEvent &eventByte);

private:
    QCountedSet<int> _serverAddresses;
    QHash<int, std::array<quint16, 20>> _counters;
    QHash<int, QHash<int, QVariant>> _serversOptions;
    QHash<int, QModbusDataUnitMap> _modbusDataUnitMaps;
    std::deque<quint8> _commEventLog;

    QModbusDevice::State _state = QModbusDevice::UnconnectedState;
    QHash<int, QModbusDevice::Error> _errors;
    QHash<int, QString> _errorsString;
};

///
/// \brief The QModbusCommEvent class
///
class QModbusCommEvent
{
public:
    enum struct SendFlag : quint8 {
        ReadExceptionSent = 0x01,
        ServerAbortExceptionSent = 0x02,
        ServerBusyExceptionSent = 0x04,
        ServerProgramNAKExceptionSent = 0x08,
        WriteTimeoutErrorOccurred = 0x10,
        CurrentlyInListenOnlyMode = 0x20,
    };

    enum struct ReceiveFlag : quint8 {
        /* Unused */
        CommunicationError = 0x02,
        /* Unused */
        /* Unused */
        CharacterOverrun = 0x10,
        CurrentlyInListenOnlyMode = 0x20,
        BroadcastReceived = 0x40
    };

    enum EventByte {
        SentEvent = 0x40,
        ReceiveEvent = 0x80,
        EnteredListenOnlyMode = 0x04,
        InitiatedCommunicationRestart = 0x00
    };

    constexpr QModbusCommEvent(QModbusCommEvent::EventByte byte) noexcept
        : _eventByte(byte) {}

    operator quint8() const { return _eventByte; }
    operator QModbusCommEvent::EventByte() const {
        return static_cast<QModbusCommEvent::EventByte> (_eventByte);
    }

    inline QModbusCommEvent &operator=(QModbusCommEvent::EventByte byte) {
        _eventByte = byte;
        return *this;
    }
    inline QModbusCommEvent &operator|=(QModbusCommEvent::SendFlag sf) {
        _eventByte |= quint8(sf);
        return *this;
    }
    inline QModbusCommEvent &operator|=(QModbusCommEvent::ReceiveFlag rf) {
        _eventByte |= quint8(rf);
        return *this;
    }

private:
    quint8 _eventByte;
};

inline QModbusCommEvent::EventByte operator|(QModbusCommEvent::EventByte b,
                                             QModbusCommEvent::SendFlag sf) { return QModbusCommEvent::EventByte(quint8(b) | quint8(sf)); }
inline QModbusCommEvent::EventByte operator|(QModbusCommEvent::SendFlag sf,
                                             QModbusCommEvent::EventByte b) { return operator|(b, sf); }
inline QModbusCommEvent::EventByte operator|(QModbusCommEvent::EventByte b,
                                             QModbusCommEvent::ReceiveFlag rf) { return QModbusCommEvent::EventByte(quint8(b) | quint8(rf)); }
inline QModbusCommEvent::EventByte operator|(QModbusCommEvent::ReceiveFlag rf,
                                             QModbusCommEvent::EventByte b) { return operator|(b, rf); }

Q_DECLARE_LOGGING_CATEGORY(QT_MODBUS)
Q_DECLARE_LOGGING_CATEGORY(QT_MODBUS_LOW)

#endif // IMODBUSSERVER_H
