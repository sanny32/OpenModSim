#ifndef MODBUSTCPSERVER_H
#define MODBUSTCPSERVER_H

#include <QModbusTcpServer>

///
/// \brief The ModbusTcpServer class
///
class ModbusTcpServer : public QModbusTcpServer
{
    Q_OBJECT

    friend class ModbusTcpConnection;

public:
    explicit ModbusTcpServer(QObject *parent = nullptr)
        : QModbusTcpServer(parent)
    {
    }

signals:
    void request(const QModbusRequest& req, int transactionId);
    void response(const QModbusRequest& req, const QModbusResponse& resp, int transactionId);

protected:
    QModbusResponse processRequest(const QModbusPdu &req) override
    {
        _transactionId++;

        emit request(req, _transactionId);
        auto resp = QModbusTcpServer::processRequest(req);
        emit response(req, resp, _transactionId);
        return resp;
    }

private:
    int _transactionId = 0;
};


#endif // MODBUSTCPSERVER_H
