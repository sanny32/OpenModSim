#include "errorsimulations.h"

///
/// \brief ErrorSimulations::ErrorSimulations
/// \param server
///
ErrorSimulations::ErrorSimulations(ModbusMultiServer* server)
    : _mbMultiServer(server)
{
    Q_ASSERT(_mbMultiServer != nullptr);

    const auto errSim = _mbMultiServer->getModbusDefinitions().ErrorSimulations;
    _responseDelayTime              = errSim.responseDelayTime();
    _responseRandomDelayUpToTime    = errSim.responseRandomDelayUpToTime();
    _noResponse                     = errSim.noResponse();
    _responseIncorrectId            = errSim.responseIncorrectId();
    _responseIllegalFunction        = errSim.responseIllegalFunction();
    _responseDeviceBusy             = errSim.responseDeviceBusy();
    _responseIncorrectCrc           = errSim.responseIncorrectCrc();
    _responseDelay                  = errSim.responseDelay();
    _responseRandomDelay            = errSim.responseRandomDelay();
}

///
/// \brief ErrorSimulations::setResponseDelayTime
/// \param value
///
void ErrorSimulations::setResponseDelayTime(int value)
{
    if (_responseDelayTime == value) return;
    _responseDelayTime = value;
    emit responseDelayTimeChanged(value);
    updateServerDefinitions();
}

///
/// \brief ErrorSimulations::setResponseRandomDelayUpToTime
/// \param value
///
void ErrorSimulations::setResponseRandomDelayUpToTime(int value)
{
    if (_responseRandomDelayUpToTime == value) return;
    _responseRandomDelayUpToTime = value;
    emit responseRandomDelayUpToTimeChanged(value);
    updateServerDefinitions();
}

///
/// \brief ErrorSimulations::setNoResponse
/// \param value
///
void ErrorSimulations::setNoResponse(bool value)
{
    if (_noResponse == value) return;
    _noResponse = value;
    emit noResponseChanged(value);
    updateServerDefinitions();
}

///
/// \brief ErrorSimulations::setResponseIncorrectId
/// \param value
///
void ErrorSimulations::setResponseIncorrectId(bool value)
{
    if (_responseIncorrectId == value) return;
    _responseIncorrectId = value;
    emit responseIncorrectIdChanged(value);
    updateServerDefinitions();
}

///
/// \brief ErrorSimulations::setResponseIllegalFunction
/// \param value
///
void ErrorSimulations::setResponseIllegalFunction(bool value)
{
    if (_responseIllegalFunction == value) return;
    _responseIllegalFunction = value;
    emit responseIllegalFunctionChanged(value);
    updateServerDefinitions();
}

///
/// \brief ErrorSimulations::setResponseDeviceBusy
/// \param value
///
void ErrorSimulations::setResponseDeviceBusy(bool value)
{
    if (_responseDeviceBusy == value) return;
    _responseDeviceBusy = value;
    emit responseDeviceBusyChanged(value);
    updateServerDefinitions();
}

///
/// \brief ErrorSimulations::setResponseIncorrectCrc
/// \param value
///
void ErrorSimulations::setResponseIncorrectCrc(bool value)
{
    if (_responseIncorrectCrc == value) return;
    _responseIncorrectCrc = value;
    emit responseIncorrectCrcChanged(value);
    updateServerDefinitions();
}

///
/// \brief ErrorSimulations::setResponseDelay
/// \param value
///
void ErrorSimulations::setResponseDelay(bool value)
{
    if (_responseDelay == value) return;
    _responseDelay = value;
    emit responseDelayChanged(value);
    updateServerDefinitions();
}

///
/// \brief ErrorSimulations::setResponseRandomDelay
/// \param value
///
void ErrorSimulations::setResponseRandomDelay(bool value)
{
    if (_responseRandomDelay == value) return;
    _responseRandomDelay = value;
    emit responseRandomDelayChanged(value);
    updateServerDefinitions();
}

///
/// \brief ErrorSimulations::updateServerDefinitions
///
void ErrorSimulations::updateServerDefinitions()
{
    if (!_mbMultiServer)
        return;

    ModbusDefinitions defs = _mbMultiServer->getModbusDefinitions();
    auto& errSim = defs.ErrorSimulations;

    errSim.setNoResponse(_noResponse);
    errSim.setResponseIncorrectId(_responseIncorrectId);
    errSim.setResponseIllegalFunction(_responseIllegalFunction);
    errSim.setResponseDeviceBusy(_responseDeviceBusy);
    errSim.setResponseIncorrectCrc(_responseIncorrectCrc);
    errSim.setResponseDelay(_responseDelay);
    errSim.setResponseRandomDelay(_responseRandomDelay);
    errSim.setResponseDelayTime(_responseDelayTime);
    errSim.setResponseRandomDelayUpToTime(_responseRandomDelayUpToTime);

    _mbMultiServer->setModbusDefinitions(defs);
}
