#ifndef IMODBUSSERVER_H
#define IMODBUSSERVER_H

#include <deque>
#include <array>
#include <QLoggingCategory>
#include <QModbusServer>
#include "qcountedset.h"
#include "modbusdataunitmap.h"
#include "qmodbuscommevent.h"
#include "modbusdefinitions.h"
#include "modbusmessage.h"

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

///
/// \brief The ModbusServer class
///
class ModbusServer : public QObject
{
    Q_OBJECT

public:
    enum Option {
        DiagnosticRegister,
        ExceptionStatusOffset,
        DeviceBusy,
        AsciiInputDelimiter,
        ListenOnlyMode,
        ServerIdentifier,
        RunIndicatorStatus,
        AdditionalData,
        DeviceIdentification,
        // Reserved
        UserOption = 0x100
    };
    Q_ENUM(Option)

    QList<int> serverAddresses() const;
    void addServerAddress(int serverAddress);
    void removeServerAddress(int serverAddress);
    void removeAllServerAddresses();
    bool hasServerAddress(int serverAddress);

    virtual bool setMap(const ModbusDataUnitMap &map, int serverAddress);
    virtual bool processesBroadcast() const { return false; }

    ModbusDefinitions getDefinitions() const { return _definitions; }
    void setDefinitions(const ModbusDefinitions& defs) {
        _definitions = defs;
        processDefinitionsChanges();
    }

    QVariant value(int option, int serverAddress) const;
    bool setValue(int option, const QVariant &value, int serverAddress);

    bool data(QModbusDataUnit *newData, int serverAddress) const;
    bool setData(const QModbusDataUnit &unit, int serverAddress);

    bool setData(QModbusDataUnit::RegisterType table, quint16 address, quint16 data, int serverAddress);
    bool data(QModbusDataUnit::RegisterType table, quint16 address, quint16 *data, int serverAddress) const;

    virtual QVariant connectionParameter(QModbusDevice::ConnectionParameter parameter) const = 0;
    virtual void setConnectionParameter(QModbusDevice::ConnectionParameter parameter, const QVariant &value) = 0;

    bool connectDevice();
    void disconnectDevice();

    QModbusDevice::State state() const;

    QModbusDevice::Error error(int serverAddress) const;
    QString errorString(int serverAddress) const;

    virtual QIODevice *device() const = 0;

signals:
    void rawDataReceived(const QDateTime& time, const QByteArray& data);
    void rawDataSended(const QDateTime& time, const QByteArray& data);
    void modbusRequest(QSharedPointer<const ModbusMessage> msg);
    void modbusResponse(QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp);

    void stateChanged(QModbusDevice::State state);
    void errorOccurred(QModbusDevice::Error error, int serverAddress);
    void dataWritten(int serverAddress, QModbusDataUnit::RegisterType table, int address, int size);

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
    void setError(const QString &errorText, QModbusDevice::Error error, int serverAddress = 0);

    virtual bool open() = 0;
    virtual void close() = 0;

    virtual bool writeData(const QModbusDataUnit &unit, int serverAddress);
    virtual bool readData(QModbusDataUnit *newData, int serverAddress) const;

    virtual bool matchingServerAddress(quint8 unitId) const;

    virtual void processDefinitionsChanges();

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
    ModbusDefinitions _definitions;
    QCountedSet<int> _serverAddresses;
    QHash<int, std::array<quint16, 20>> _counters;
    QHash<int, QHash<int, QVariant>> _serversOptions;
    QHash<int, ModbusDataUnitMap> _modbusDataUnitMaps;
    std::deque<quint8> _commEventLog;

    QModbusDevice::State _state = QModbusDevice::UnconnectedState;
    QHash<int, QModbusDevice::Error> _errors;
    QHash<int, QString> _errorsString;
};

Q_DECLARE_METATYPE(QModbusRequest)
Q_DECLARE_METATYPE(QModbusResponse)
Q_DECLARE_METATYPE(QModbusDataUnit)
DECLARE_ENUM_STRINGS(QModbusDataUnit::RegisterType,
                     {   QModbusDataUnit::Invalid,          "Invalid"           },
                     {   QModbusDataUnit::DiscreteInputs,   "DiscreteInputs"    },
                     {   QModbusDataUnit::Coils,            "Coils"             },
                     {   QModbusDataUnit::InputRegisters,   "InputRegisters"    },
                     {   QModbusDataUnit::HoldingRegisters, "HoldingRegisters"  }
)

Q_DECLARE_LOGGING_CATEGORY(QT_MODBUS)
Q_DECLARE_LOGGING_CATEGORY(QT_MODBUS_LOW)

#endif // IMODBUSSERVER_H
