#ifndef QMODBUSCOMMEVENT_H
#define QMODBUSCOMMEVENT_H

#include <QtGlobal>

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

#endif // QMODBUSCOMMEVENT_H
