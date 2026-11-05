// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file rtusnifferparser.h
/// \brief Declares the passive Modbus RTU bus frame splitter.
///

#ifndef RTUSNIFFERPARSER_H
#define RTUSNIFFERPARSER_H

#include <QByteArray>
#include <QVector>

///
/// \brief A frame captured from a Modbus RTU bus.
///
struct RtuSniffedFrame
{
    QByteArray Data;
    bool IsRequest = true;
};

///
/// \brief The RtuSnifferParser class splits a passively captured RTU byte stream into frames.
///
class RtuSnifferParser
{
public:
    static constexpr int MaxRtuAduSize = 256;

    ///
    /// \brief Appends received bytes and returns every frame completed by them.
    /// \param chunk Bytes read from the port.
    /// \param afterSilence True when the bus was idle for at least 3.5 characters before this chunk.
    /// \return Frames completed by this chunk, in bus order.
    ///
    QVector<RtuSniffedFrame> feed(const QByteArray& chunk, bool afterSilence);

    ///
    /// \brief Emits the buffered bytes that do not form a recognizable frame.
    /// \return One frame with the leftover bytes, or nothing when the buffer is empty.
    ///
    QVector<RtuSniffedFrame> flush();

    ///
    /// \brief Discards buffered bytes and the request/response pairing state.
    ///
    void reset();

    ///
    /// \brief Returns true when bytes are waiting for the rest of a frame.
    ///
    bool hasPendingData() const { return !_buffer.isEmpty(); }

private:
    enum class Match {
        Frame,
        Incomplete,
        Mismatch
    };

    Match matchFrame(const QByteArray& data, int* length, bool* isRequest) const;
    bool isResponseToPendingRequest(const QByteArray& data) const;
    RtuSniffedFrame takeFrame(int length, bool isRequest);
    void extractFrames(QVector<RtuSniffedFrame>& frames);

private:
    QByteArray _buffer;
    bool _hasPendingRequest = false;
    quint8 _pendingAddress = 0;
    quint8 _pendingFunction = 0;
};

#endif // RTUSNIFFERPARSER_H
