#ifndef MODBUSERRORSIMULATIONS_H
#define MODBUSERRORSIMULATIONS_H

#include <QSettings>
#include <QVersionNumber>

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
Q_DECLARE_METATYPE(ModbusErrorSimulations)

///
/// \brief operator <<
/// \param out
/// \param errsim
/// \return
///
inline QSettings& operator <<(QSettings& out, const ModbusErrorSimulations& errsim)
{
    out.beginGroup("ModbusErrorSimulations");
    out.setValue("NoResponse", errsim.noResponse());
    out.setValue("ResponseIncorrectId", errsim.responseIncorrectId());
    out.setValue("ResponseIllegalFunction", errsim.responseIllegalFunction());
    out.setValue("ResponseDeviceBusy", errsim.responseDeviceBusy());
    out.setValue("ResponseIncorrectCrc", errsim.responseIncorrectCrc());
    out.setValue("ResponseDelay", errsim.responseDelay());
    out.setValue("ResponseDelayTime", errsim.responseDelayTime());
    out.setValue("ResponseRandomDelay", errsim.responseRandomDelay());
    out.setValue("ResponseRandomDelayUpToTime", errsim.responseRandomDelayUpToTime());
    out.endGroup();

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param errsim
/// \return
///
inline QSettings& operator >>(QSettings& in, ModbusErrorSimulations& errsim)
{
    in.beginGroup("ModbusErrorSimulations");
    errsim.setNoResponse(in.value("NoResponse").toBool());
    errsim.setResponseIncorrectId(in.value("ResponseIncorrectId").toBool());
    errsim.setResponseIllegalFunction(in.value("ResponseIllegalFunction").toBool());
    errsim.setResponseDeviceBusy(in.value("ResponseDeviceBusy").toBool());
    errsim.setResponseIncorrectCrc(in.value("ResponseIncorrectCrc").toBool());
    errsim.setResponseDelay(in.value("ResponseDelay").toBool());
    errsim.setResponseDelayTime(in.value("ResponseDelayTime").toInt());
    errsim.setResponseRandomDelay(in.value("ResponseRandomDelay").toBool());
    errsim.setResponseRandomDelayUpToTime(in.value("ResponseRandomDelayUpToTime", 1000).toInt());
    in.endGroup();

    return in;
}

///
/// \brief operator <<
/// \param out
/// \param errsim
/// \return
///
inline QDataStream& operator <<(QDataStream& out, const ModbusErrorSimulations& errsim)
{
    out << QVersionNumber(1, 0);
    out << errsim.noResponse();
    out << errsim.responseIncorrectId();
    out << errsim.responseIllegalFunction();
    out << errsim.responseDeviceBusy();
    out << errsim.responseIncorrectCrc();
    out << errsim.responseDelay();
    out << errsim.responseDelayTime();
    out << errsim.responseRandomDelay();
    out << errsim.responseRandomDelayUpToTime();

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param errsim
/// \return
///
inline QDataStream& operator >>(QDataStream& in, ModbusErrorSimulations& errsim)
{
    QVersionNumber ver;
    in >> ver;

    if(ver < QVersionNumber(1, 0))
        return in;

    bool noResponse = false;
    in >> noResponse;
    errsim.setNoResponse(noResponse);

    bool responseIncorrectId = false;
    in >> responseIncorrectId;
    errsim.setResponseIncorrectId(responseIncorrectId);

    bool responseIllegalFunction = false;
    in >> responseIllegalFunction;
    errsim.setResponseIllegalFunction(responseIllegalFunction);

    bool responseDeviceBusy = false;
    in >> responseDeviceBusy;
    errsim.setResponseDeviceBusy(responseDeviceBusy);

    bool responseIncorrectCrc = false;
    in >> responseIncorrectCrc;
    errsim.setResponseIncorrectCrc(responseIncorrectCrc);

    bool responseDelay = false;
    in >> responseDelay;
    errsim.setResponseDelay(responseDelay);

    int responseDelayTime = 0;
    in >> responseDelayTime;
    errsim.setResponseDelayTime(responseDelayTime);

    bool responseRandomDelay = false;
    in >> responseRandomDelay;
    errsim.setResponseRandomDelay(responseRandomDelay);

    int responseRandomDelayUpToTime = 1000;
    in >> responseRandomDelayUpToTime;
    errsim.setResponseRandomDelayUpToTime(responseRandomDelayUpToTime);

    return in;
}

#endif // MODBUSERRORSIMULATIONS_H
