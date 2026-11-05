// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file writesingleregister.h
/// \brief Declares the writesingleregister interfaces.
///

#ifndef WRITESINGLEREGISTER_H
#define WRITESINGLEREGISTER_H

#include "modbusmessage.h"
#include "enronmodbus.h"

///
/// \brief The WriteSingleRegisterRequest class
///
class WriteSingleRegisterRequest : public ModbusMessage
{
public:
    ///
    /// \brief WriteSingleRegisterRequest
    /// \param pdu
    /// \param protocol
    /// \param deviceId
    /// \param transactionId
    /// \param timestamp
    ///
    WriteSingleRegisterRequest(const QModbusPdu& pdu, ProtocolType protocol, int deviceId, int transactionId, const QDateTime& timestamp)
        : ModbusMessage(pdu, protocol, deviceId, transactionId, timestamp, true)
    {
        Q_ASSERT(functionCode() == QModbusPdu::WriteSingleRegister);
    }

    ///
    /// \brief WriteSingleRegisterRequest
    /// \param data
    /// \param protocol
    /// \param timestamp
    ///
    WriteSingleRegisterRequest(const QByteArray& data, ProtocolType protocol, const QDateTime& timestamp)
        : ModbusMessage(data, protocol, timestamp, true)
    {
        Q_ASSERT(functionCode() == QModbusPdu::WriteSingleRegister);
    }

    ///
    /// \brief isValid
    /// \return
    ///
    bool isValid() const override {
        return ModbusMessage::isValid() && (isException() || dataSize() == 4
            || (dataSize() == 6 && EnronModbus::is32BitAddress(address())));
    }

    ///
    /// \brief address
    /// \return
    ///
    quint16 address() const {
        return makeUInt16(at(1), at(0), ByteOrder::Direct);
    }

    ///
    /// \brief Returns the unsigned wire value, including both words of an Enron register.
    ///
    quint32 value() const {
        const quint16 high = makeUInt16(at(3), at(2), ByteOrder::Direct);
        if (dataSize() == 6)
            return (quint32(high) << 16) | makeUInt16(at(5), at(4), ByteOrder::Direct);
        return high;
    }
};

///
/// \brief The WriteSingleRegisterResponse class
///
class WriteSingleRegisterResponse : public ModbusMessage
{
public:
    ///
    /// \brief WriteSingleRegisterResponse
    /// \param pdu
    /// \param protocol
    /// \param deviceId
    /// \param transactionId
    /// \param timestamp
    ///
    WriteSingleRegisterResponse(const QModbusPdu& pdu, ProtocolType protocol, int deviceId, int transactionId, const QDateTime& timestamp)
        :ModbusMessage(pdu, protocol, deviceId, transactionId, timestamp, false)
    {
        Q_ASSERT(functionCode() == QModbusPdu::WriteSingleRegister);
    }

    ///
    /// \brief WriteSingleRegisterResponse
    /// \param data
    /// \param protocol
    /// \param timestamp
    ///
    WriteSingleRegisterResponse(const QByteArray& data, ProtocolType protocol, const QDateTime& timestamp)
        : ModbusMessage(data, protocol, timestamp, false)
    {
        Q_ASSERT(functionCode() == QModbusPdu::WriteSingleRegister);
    }

    ///
    /// \brief isValid
    /// \return
    ///
    bool isValid() const override {
        return ModbusMessage::isValid() && (dataSize() == 4
            || (dataSize() == 6 && EnronModbus::is32BitAddress(address())));
    }

    ///
    /// \brief address
    /// \return
    ///
    quint16 address() const {
        return makeUInt16(at(1), at(0), ByteOrder::Direct);
    }

    ///
    /// \brief Returns the unsigned wire value, including both words of an Enron register.
    ///
    quint32 value() const {
        const quint16 high = makeUInt16(at(3), at(2), ByteOrder::Direct);
        if (dataSize() == 6)
            return (quint32(high) << 16) | makeUInt16(at(5), at(4), ByteOrder::Direct);
        return high;
    }
};

#endif // WRITESINGLEREGISTER_H

