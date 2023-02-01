#include "connectionmanager.h"

///
/// \brief ConnectionManager::ConnectionManager
///
ConnectionManager::ConnectionManager()
{
}

///
/// \brief ConnectionManager::connect
/// \param frm
/// \param cd
///
void ConnectionManager::connect(FormModSim* frm, const ConnectionDetails& cd)
{
    Q_ASSERT(frm != nullptr);

    QList<Connection>::Iterator itCn = _connList.end();
    for(auto it = _connList.begin(); it != _connList.end(); ++it)
    {
        if(it->Details == cd)
        {
            itCn = it;
            break;
        }
    }

    auto server = (itCn == _connList.end()) ?
                    QSharedPointer<ModbusServer>(new ModbusServer(cd)) :
                    itCn->Server;

    if(itCn == _connList.end())
    {
        Connection cn;
        cn.Details = cd;
        cn.Forms.append(frm);
        cn.Server = server;
        _connList.append(cn);
    }
    else
    {
        if(!itCn->Forms.contains(frm))
            itCn->Forms.append(frm);
    }

    frm->setMbServer(server);

    if(server->state() == QModbusDevice::UnconnectedState)
        server->connectDevice();
}

///
/// \brief ConnectionManager::disconnect
/// \param frm
///
void ConnectionManager::disconnect(FormModSim* frm)
{
    Q_ASSERT(frm != nullptr);

    QList<Connection>::Iterator itCn = _connList.end();
    for(auto it = _connList.begin(); it != _connList.end(); ++it)
    {
        if(it->Forms.contains(frm))
        {
            itCn = it;
            break;
        }
    }

    if(itCn != _connList.end())
    {
        itCn->Server->disconnectDevice();
        itCn->Forms.removeOne(frm);
        if(itCn->Forms.isEmpty())
        {
            _connList.removeOne(*itCn);
        }
    }
}
