#ifndef MODBUSERRORSIMULATIONS_H
#define MODBUSERRORSIMULATIONS_H

///
/// \brief The ModbusErrorSimulations class
///
class ModbusErrorSimulations
{
public:
    ModbusErrorSimulations();

    bool noResponse() const;
    void setNoResponse(bool value);

    bool responseIncorrectId() const;
    void setResponseIncorrectId(bool value);

    bool responseIllegalFunction() const;
    void setResponseIllegalFunction(bool value);

    bool responseDeviceBusy() const;
    void setResponseDeviceBusy(bool value);

    bool responseIncorrectCrc() const;
    void setResponseIncorrectCrc(bool value);

    bool responseDelay() const;
    void setResponseDelay(bool value);

    int responseDelayTime() const;
    void setResponseDelayTime(int value);

    bool responseRandomDelay() const;
    void setResponseRandomDelay(bool value);

    int responseRandomDelayUpToTime() const;
    void setResponseRandomDelayUpToTime(int value);

private:
    int _responseDelayTime = 0;
    int _responseRandomDelayUpToTime = 1000;

    bool _noResponse = false;
    bool _responseIncorrectId = false;
    bool _responseIllegalFunction = false;
    bool _responseDeviceBusy = false;
    bool _responseIncorrectCrc = false;
    bool _responseDelay = false;
    bool _responseRandomDelay = false;
};

#endif // MODBUSERRORSIMULATIONS_H
