// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file rtusnifferparser.cpp
/// \brief Implements the passive Modbus RTU bus frame splitter.
///

#include <QModbusPdu>
#include "qmodbusadurtu.h"
#include "rtusnifferparser.h"

namespace {

///
/// \brief Converts a PDU data size reported by Qt into a full RTU frame length.
/// \param dataSize Size of the PDU data without the function code, or -1 when unknown.
/// \return Frame length including address, function code and CRC; 0 when unknown yet, -1 when impossible.
///
int rtuFrameLength(int dataSize)
{
    if (dataSize < 0)
        return 0;

    const int length = 2 + dataSize + 2;
    return length <= RtuSnifferParser::MaxRtuAduSize ? length : -1;
}

///
/// \brief Checks whether \a data starts with a frame of the candidate \a length.
/// \param incomplete Set to true when more bytes are needed to decide.
/// \return True when the candidate frame is complete and its CRC matches.
///
bool candidateFits(const QByteArray& data, int length, bool* incomplete)
{
    if (length < 0)
        return false;

    if (length == 0 || length > data.size()) {
        *incomplete = true;
        return false;
    }

    return QModbusAduRtu(data.left(length)).matchingChecksum();
}

}

///
/// \brief Appends received bytes and returns every frame completed by them.
/// \param chunk Bytes read from the port.
/// \param afterSilence True when the bus was idle for at least 3.5 characters before this chunk.
/// \return Frames completed by this chunk, in bus order.
///
QVector<RtuSniffedFrame> RtuSnifferParser::feed(const QByteArray& chunk, bool afterSilence)
{
    QVector<RtuSniffedFrame> frames;

    if (afterSilence && !_buffer.isEmpty()) {
        int length = 0;
        bool isRequest = true;
        // Serial drivers may deliver one frame in delayed chunks, so silence alone does not end it.
        const Match joined = matchFrame(_buffer + chunk, &length, &isRequest);
        if (joined == Match::Mismatch
            || (joined == Match::Incomplete && matchFrame(chunk, &length, &isRequest) == Match::Frame)) {
            frames += flush();
        }
    }

    _buffer += chunk;
    extractFrames(frames);
    return frames;
}

///
/// \brief Emits the buffered bytes that do not form a recognizable frame.
/// \return One frame with the leftover bytes, or nothing when the buffer is empty.
///
QVector<RtuSniffedFrame> RtuSnifferParser::flush()
{
    if (_buffer.isEmpty())
        return {};

    RtuSniffedFrame frame;
    frame.Data = _buffer;
    frame.IsRequest = !isResponseToPendingRequest(_buffer);
    if (!frame.IsRequest)
        _hasPendingRequest = false;

    _buffer.clear();
    return { frame };
}

///
/// \brief Discards buffered bytes and the request/response pairing state.
///
void RtuSnifferParser::reset()
{
    _buffer.clear();
    _hasPendingRequest = false;
}

///
/// \brief Checks whether \a data starts with a complete RTU request or response.
/// \param data Bytes starting at a supposed frame boundary.
/// \param length Receives the frame length on Match::Frame.
/// \param isRequest Receives the frame direction on Match::Frame.
/// \return Match::Incomplete when more bytes may still complete a frame, Match::Mismatch when they cannot.
///
RtuSnifferParser::Match RtuSnifferParser::matchFrame(const QByteArray& data, int* length, bool* isRequest) const
{
    if (data.size() < QModbusAduRtu::MinRtuFrameSize)
        return Match::Incomplete;

    const quint8 rawFunction = quint8(data.at(1));
    const auto functionCode = QModbusPdu::FunctionCode(rawFunction);
    const QByteArray pduData = data.mid(2);
    const bool isException = rawFunction & QModbusPdu::ExceptionByte;

    const int requestLength = isException
        ? -1
        : rtuFrameLength(QModbusRequest::calculateDataSize(QModbusRequest(functionCode, pduData)));
    const int responseLength =
        rtuFrameLength(QModbusResponse::calculateDataSize(QModbusResponse(functionCode, pduData)));

    bool incomplete = false;
    const bool requestFits = candidateFits(data, requestLength, &incomplete);
    const bool responseFits = candidateFits(data, responseLength, &incomplete);

    if (requestFits && responseFits) {
        // Same-sized request and response (e.g. FC05/FC06 echo) differ only by bus order.
        *isRequest = !isResponseToPendingRequest(data);
        *length = *isRequest ? requestLength : responseLength;
        return Match::Frame;
    }

    if (requestFits || responseFits) {
        *isRequest = requestFits;
        *length = requestFits ? requestLength : responseLength;
        return Match::Frame;
    }

    return incomplete ? Match::Incomplete : Match::Mismatch;
}

///
/// \brief Returns true when \a data comes from the unit the last request was addressed to.
///
bool RtuSnifferParser::isResponseToPendingRequest(const QByteArray& data) const
{
    if (!_hasPendingRequest || data.size() < 2)
        return false;

    const quint8 function = quint8(data.at(1)) & ~QModbusPdu::ExceptionByte;
    return quint8(data.at(0)) == _pendingAddress && function == _pendingFunction;
}

///
/// \brief Removes a recognized frame from the buffer and updates the request/response pairing.
///
RtuSniffedFrame RtuSnifferParser::takeFrame(int length, bool isRequest)
{
    RtuSniffedFrame frame;
    frame.Data = _buffer.left(length);
    frame.IsRequest = isRequest;
    _buffer.remove(0, length);

    if (isRequest) {
        _pendingAddress = quint8(frame.Data.at(0));
        _pendingFunction = quint8(frame.Data.at(1));
        // Broadcast requests are never answered.
        _hasPendingRequest = (_pendingAddress != 0);
    } else {
        _hasPendingRequest = false;
    }

    return frame;
}

///
/// \brief Moves every complete frame at the start of the buffer into \a frames.
///
void RtuSnifferParser::extractFrames(QVector<RtuSniffedFrame>& frames)
{
    int length = 0;
    bool isRequest = true;
    while (matchFrame(_buffer, &length, &isRequest) == Match::Frame)
        frames.push_back(takeFrame(length, isRequest));

    if (_buffer.size() > MaxRtuAduSize)
        frames += flush();
}
