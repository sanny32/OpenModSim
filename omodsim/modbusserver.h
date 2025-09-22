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
/// \brief The ModbusServer class
///
class ModbusServer : public QModbusServer
{
    Q_OBJECT

public:
    int serverAddress() const;
    void setServerAddress(int serverAddress);

    bool setMap(const QModbusDataUnitMap &map) override;

    QVariant value(int option) const override;
    bool setValue(int option, const QVariant &value) override;

    bool data(QModbusDataUnit *newData) const;
    bool setData(const QModbusDataUnit &unit);

    bool setData(QModbusDataUnit::RegisterType table, quint16 address, quint16 data);
    bool data(QModbusDataUnit::RegisterType table, quint16 address, quint16 *data) const;

    virtual QVariant connectionParameter(ConnectionParameter parameter) const = 0;
    virtual void setConnectionParameter(ConnectionParameter parameter, const QVariant &value) = 0;

    bool connectDevice();
    void disconnectDevice();

    State state() const;

    Error error() const;
    QString errorString() const;

    virtual QIODevice *device() const = 0;

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

    bool writeData(const QModbusDataUnit &unit) override;
    bool readData(QModbusDataUnit *newData) const override;

    QModbusResponse processRequest(const QModbusPdu &request) override;
    QModbusResponse processPrivateRequest(const QModbusPdu &request) override;

    void resetCommunicationCounters() { _counters.fill(0u); }
    void incrementCounter(ModbusServer::Counter counter) { _counters[counter]++; }

    QModbusResponse processReadCoilsRequest(const QModbusRequest &request);
    QModbusResponse processReadDiscreteInputsRequest(const QModbusRequest &request);
    QModbusResponse readBits(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType);

    QModbusResponse processReadHoldingRegistersRequest(const QModbusRequest &request);
    QModbusResponse processReadInputRegistersRequest(const QModbusRequest &request);
    QModbusResponse readBytes(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType);

    QModbusResponse processWriteSingleCoilRequest(const QModbusRequest &request);
    QModbusResponse processWriteSingleRegisterRequest(const QModbusRequest &request);
    QModbusResponse writeSingle(const QModbusPdu &request, QModbusDataUnit::RegisterType unitType);

    QModbusResponse processReadExceptionStatusRequest(const QModbusRequest &request);
    QModbusResponse processDiagnosticsRequest(const QModbusRequest &request);
    QModbusResponse processGetCommEventCounterRequest(const QModbusRequest &request);
    QModbusResponse processGetCommEventLogRequest(const QModbusRequest &request);
    QModbusResponse processWriteMultipleCoilsRequest(const QModbusRequest &request);
    QModbusResponse processWriteMultipleRegistersRequest(const QModbusRequest &request);
    QModbusResponse processReportServerIdRequest(const QModbusRequest &request);
    QModbusResponse processMaskWriteRegisterRequest(const QModbusRequest &request);
    QModbusResponse processReadWriteMultipleRegistersRequest(const QModbusRequest &request);
    QModbusResponse processReadFifoQueueRequest(const QModbusRequest &request);
    QModbusResponse processEncapsulatedInterfaceTransportRequest(const QModbusRequest &request);

    void storeModbusCommEvent(const QModbusCommEvent &eventByte);

private:
    int _serverAddress = 1;
    std::array<quint16, 20> _counters;
    QHash<int, QVariant> _serverOptions;
    QModbusDataUnitMap _modbusDataUnitMap;
    std::deque<quint8> _commEventLog;

    QModbusDevice::State _state = QModbusDevice::UnconnectedState;
    QModbusDevice::Error _error = QModbusDevice::NoError;
    QString _errorString;
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
        : m_eventByte(byte) {}

    operator quint8() const { return m_eventByte; }
    operator QModbusCommEvent::EventByte() const {
        return static_cast<QModbusCommEvent::EventByte> (m_eventByte);
    }

    inline QModbusCommEvent &operator=(QModbusCommEvent::EventByte byte) {
        m_eventByte = byte;
        return *this;
    }
    inline QModbusCommEvent &operator|=(QModbusCommEvent::SendFlag sf) {
        m_eventByte |= quint8(sf);
        return *this;
    }
    inline QModbusCommEvent &operator|=(QModbusCommEvent::ReceiveFlag rf) {
        m_eventByte |= quint8(rf);
        return *this;
    }

private:
    quint8 m_eventByte;
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
