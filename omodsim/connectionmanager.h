#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include "formmodsim.h"
#include "modbusserver.h"
#include "connectiondetails.h"

///
/// \brief The ConnectionManager class
///
class ConnectionManager
{
public:
    ConnectionManager();

    void connect(FormModSim* frm, const ConnectionDetails& cd);
    void disconnect(FormModSim* frm);

private:
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
    QList<Connection> _connList;
};

#endif // CONNECTIONMANAGER_H
