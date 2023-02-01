#include "connectionmanager.h"

///
/// \brief ConnectionManager::ConnectionManager
///
ConnectionManager::ConnectionManager(QObject* parent)
    :QObject(parent)
{
}

///
/// \brief ConnectionManager::connectDevice
/// \param frm
/// \param cd
///
void ConnectionManager::connectDevice(FormModSim* frm, const ConnectionDetails& cd)
{
    Q_ASSERT(frm != nullptr);

    auto it = findConnection(cd);
    auto server = (it != _connList.end()) ? it->Server : QSharedPointer<ModbusServer>(new ModbusServer(cd, this));
    Q_ASSERT(server != nullptr);

    if(it == _connList.end())
    {
        Connection cn;
        cn.Details = cd;
        cn.Forms << frm;
        cn.Server = server;
        _connList.append(cn);
    }
    else
    {
        if(!it->Forms.contains(frm))
            it->Forms.append(frm);
    }

    frm->setMbServer(server);
    connect(frm, &FormModSim::closing, this, [this, frm]
    {
        removeForm(frm);
    });

    if(server->state() == QModbusDevice::UnconnectedState)
        server->connectDevice();
}

///
/// \brief ConnectionManager::disconnectDevice
/// \param frm
///
void ConnectionManager::disconnectDevice(FormModSim* frm)
{
    Q_ASSERT(frm != nullptr);

    auto it = findConnection(frm);
    if(it != _connList.end())
        it->Server->disconnectDevice();
}

///
/// \brief ConnectionManager::removeForm
/// \param frm
///
void ConnectionManager::removeForm(FormModSim* frm)
{
    auto it = findConnection(frm);
    if(it == _connList.end()) return;

    it->Forms.removeOne(frm);
    if(it->Forms.isEmpty())
    {
        it->Server->disconnectDevice();
        _connList.removeOne(*it);
    }
}

///
/// \brief ConnectionManager::findConnection
/// \param frm
/// \return
///
QList<ConnectionManager::Connection>::Iterator ConnectionManager::findConnection(FormModSim* frm)
{
    for(auto it = _connList.begin(); it != _connList.end(); ++it)
    {
        if(it->Forms.contains(frm))
        {
           return it;
        }
    }

    return _connList.end();
}

///
/// \brief ConnectionManager::findConnection
/// \param cd
/// \return
///
QList<ConnectionManager::Connection>::Iterator ConnectionManager::findConnection(const ConnectionDetails& cd)
{
    for(auto it = _connList.begin(); it != _connList.end(); ++it)
    {
        if(it->Details == cd)
        {
           return it;
        }
    }

    return _connList.end();
}
