#include "modbusserver.h"

///
/// \brief ModbusServer::ModbusServer
/// \param parent
///
ModbusServer::ModbusServer(QObject *parent)
    : QObject{parent}
    ,_modbusServer(nullptr)
{
}

///
/// \brief ModbusServer::~ModbusServer
///
ModbusServer::~ModbusServer()
{
    if(_modbusServer)
        delete _modbusServer;
}

///
/// \brief ModbusServer::isValid
/// \return
///
bool ModbusServer::isValid() const
{
     return _modbusServer != nullptr;
}

///
/// \brief ModbusServer::state
/// \return
///
QModbusDevice::State ModbusServer::state() const
{
    if(_modbusServer)
        return _modbusServer->state();

    return QModbusDevice::UnconnectedState;
}
