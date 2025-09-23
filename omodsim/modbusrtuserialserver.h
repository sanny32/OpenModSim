#ifndef MODBUSRTUSERIALSERVER_H
#define MODBUSRTUSERIALSERVER_H

#include <QSerialPort>
#include <QElapsedTimer>
#include "modbusserver.h"

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

signals:
    void request(int serverAddress, const QModbusRequest& req);
    void response(int serverAddress, const QModbusRequest& req, const QModbusResponse& resp);

private slots:
    void on_readyRead();
    void on_aboutToClose();
    void on_errorOccurred(QSerialPort::SerialPortError);

protected:
    bool open() override;
    void close() override;

private:
    void setupEnvironment();
    void calculateInterFrameDelay();
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
};

#endif // MODBUSRTUSERIALSERVER_H
