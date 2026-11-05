// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file modbusrtuserialserver.h
/// \brief Declares the modbusrtuserialserver interfaces.
///

#ifndef MODBUSRTUSERIALSERVER_H
#define MODBUSRTUSERIALSERVER_H

#include <QSerialPort>
#include <QElapsedTimer>
#include "modbusserver.h"
#include "rtusnifferparser.h"

class QTimer;

///
/// \brief The ModbusRtuSerialServer class
///
class ModbusRtuSerialServer : public ModbusServer
{
    Q_OBJECT
public:
    explicit ModbusRtuSerialServer(QObject *parent = nullptr);
    ~ModbusRtuSerialServer();

    bool processesBroadcast() const override;

    int interFrameDelay() const;
    void setInterFrameDelay(int microseconds);

    QVariant connectionParameter(QModbusDevice::ConnectionParameter parameter) const override;
    void setConnectionParameter(QModbusDevice::ConnectionParameter parameter, const QVariant &value) override;

    QIODevice *device() const override { return _serialPort; }

    ///
    /// \brief Returns true when the port passively captures bus traffic instead of serving requests.
    ///
    bool snifferMode() const { return _snifferMode; }

    ///
    /// \brief Switches between serving requests and passively capturing all bus traffic.
    /// \param on True to stop responding and report every frame seen on the bus.
    ///
    void setSnifferMode(bool on);

signals:
    void sniffedFrame(QSharedPointer<const ModbusMessage> msg);

private slots:
    void on_readyRead();
    void on_snifferIdleTimeout();
    void on_aboutToClose();
    void on_rawDataReceived(const QDateTime& time, const QByteArray& data);
    void on_errorOccurred(QSerialPort::SerialPortError);

protected:
    bool open() override;
    void close() override;

private:
    void setupEnvironment();
    void calculateInterFrameDelay();
    void processSniffedData();
    void emitSniffedFrames(const QVector<RtuSniffedFrame>& frames);
    QModbusResponse forwardProcessRequest(const QModbusPdu &req, int serverAddress);

private:
    QString _comPort;
    QSerialPort::DataBits _dataBits = QSerialPort::Data8;
    QSerialPort::Parity _parity = QSerialPort::EvenParity;
    QSerialPort::StopBits _stopBits = QSerialPort::OneStop;
    QSerialPort::BaudRate _baudRate = QSerialPort::Baud19200;

    QByteArray _requestBuffer;
    bool _processesBroadcast = false;
    QSerialPort* _serialPort = nullptr;
    QElapsedTimer _interFrameTimer;

    static constexpr int RecommendedDelay = 2; // A approximated value of 1.750 msec.
    int _interFrameDelayMilliseconds = RecommendedDelay;

    static constexpr int SnifferIdleFlushDelay = 50;
    bool _snifferMode = false;
    RtuSnifferParser _snifferParser;
    QTimer* _snifferIdleTimer = nullptr;
    QDateTime _lastSniffedChunkTime;
};

#endif // MODBUSRTUSERIALSERVER_H

