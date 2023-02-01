#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include "formmodsim.h"
#include "modbusserver.h"
#include "connectiondetails.h"

///
/// \brief The ConnectionManager class
///
class ConnectionManager : public QObject
{
    Q_OBJECT

    struct Connection
    {
        ConnectionDetails Details;
        QList<FormModSim*> Forms;
        QSharedPointer<ModbusServer> Server;

        friend bool operator==(const Connection& c1, const Connection& c2)
        {
            return c1.Details == c2.Details &&
                   c1.Forms == c2.Forms &&
                   c1.Server == c2.Server;
        }
    };

public:
    ConnectionManager(QObject* parent = nullptr);

    void connectDevice(FormModSim* frm, const ConnectionDetails& cd);
    void disconnectDevice(FormModSim* frm);

private:
    void removeForm(FormModSim* frm);
    QList<Connection>::Iterator findConnection(FormModSim* frm);
    QList<Connection>::Iterator findConnection(const ConnectionDetails& cd);

private:
    QList<Connection> _connList;
};

#endif // CONNECTIONMANAGER_H
