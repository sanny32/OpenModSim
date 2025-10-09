#include <QtGlobal>
#include "modbuserrorsimulations.h"

///
/// \brief ModbusErrorSimulations::ModbusErrorSimulations
///
ModbusErrorSimulations::ModbusErrorSimulations()
{
}

///
/// \brief ModbusErrorSimulations::noResponse
/// \return
///
bool ModbusErrorSimulations::noResponse() const
{
    return _noResponse;
}

///
/// \brief ModbusErrorSimulations::setNoResponse
/// \param value
///
void ModbusErrorSimulations::setNoResponse(bool value)
{
    _noResponse = value;
}

///
/// \brief ModbusErrorSimulations::responseIncorrectId
/// \return
///
bool ModbusErrorSimulations::responseIncorrectId() const
{
    return _responseIncorrectId;
}

///
/// \brief ModbusErrorSimulations::setResponseIncorrectId
/// \param value
///
void ModbusErrorSimulations::setResponseIncorrectId(bool value)
{
    _responseIncorrectId = value;
}

///
/// \brief ModbusErrorSimulations::responseIllegalFunction
/// \return
///
bool ModbusErrorSimulations::responseIllegalFunction() const
{
    return _responseIllegalFunction;
}

///
/// \brief ModbusErrorSimulations::setResponseIllegalFunction
/// \param value
///
void ModbusErrorSimulations::setResponseIllegalFunction(bool value)
{
    _responseIllegalFunction = value;
}

///
/// \brief ModbusErrorSimulations::responseDeviceBusy
/// \return
///
bool ModbusErrorSimulations::responseDeviceBusy() const
{
    return _responseDeviceBusy;
}

///
/// \brief ModbusErrorSimulations::setResponseDeviceBusy
/// \param value
///
void ModbusErrorSimulations::setResponseDeviceBusy(bool value)
{
    _responseDeviceBusy = value;
}

///
/// \brief ModbusErrorSimulations::responseIncorrectCrc
/// \return
///
bool ModbusErrorSimulations::responseIncorrectCrc() const
{
    return _responseIncorrectCrc;
}

///
/// \brief ModbusErrorSimulations::setResponseIncorrectCrc
/// \param value
///
void ModbusErrorSimulations::setResponseIncorrectCrc(bool value)
{
    _responseIncorrectCrc = value;
}

///
/// \brief ModbusErrorSimulations::responseDelay
/// \return
///
bool ModbusErrorSimulations::responseDelay() const
{
    return _responseDelay;
}

///
/// \brief ModbusErrorSimulations::setResponseDelay
/// \param value
///
void ModbusErrorSimulations::setResponseDelay(bool value)
{
    _responseDelay = value;
}

///
/// \brief ModbusErrorSimulations::responseDelayTime
/// \return
///
int ModbusErrorSimulations::responseDelayTime() const
{
    return _responseDelayTime;
}

///
/// \brief ModbusErrorSimulations::setResponseDelayTime
/// \param value
///
void ModbusErrorSimulations::setResponseDelayTime(int value)
{
    _responseDelayTime = qMax(0, value);
}

///
/// \brief ModbusErrorSimulations::responseRandomDelay
/// \return
///
bool ModbusErrorSimulations::responseRandomDelay() const
{
    return _responseRandomDelay;
}

///
/// \brief ModbusErrorSimulations::setResponseRandomDelay
/// \param value
///
void ModbusErrorSimulations::setResponseRandomDelay(bool value)
{
    _responseRandomDelay = value;
}

///
/// \brief ModbusErrorSimulations::responseRandomDelayUpToTime
/// \return
///
int ModbusErrorSimulations::responseRandomDelayUpToTime() const
{
    return _responseRandomDelayUpToTime;
}

///
/// \brief ModbusErrorSimulations::setResponseRandomDelayUpToTime
/// \param value
///
void ModbusErrorSimulations::setResponseRandomDelayUpToTime(int value)
{
    _responseRandomDelayUpToTime = qMax(0, value);
}
