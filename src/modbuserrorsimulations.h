#ifndef MODBUSERRORSIMULATIONS_H
#define MODBUSERRORSIMULATIONS_H

#include <QSettings>
#include <QDataStream>
#include <QVersionNumber>
#include <QXmlStreamReader>
#include "enums.h"

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

///
/// \brief operator <<
/// \param xml
/// \param errors
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const ModbusErrorSimulations& errors)
{
    xml.writeStartElement("ModbusErrorSimulations");

    xml.writeAttribute("NoResponse", boolToString(errors.noResponse()));
    xml.writeAttribute("ResponseIncorrectId", boolToString(errors.responseIncorrectId()));
    xml.writeAttribute("ResponseIllegalFunction", boolToString(errors.responseIllegalFunction()));
    xml.writeAttribute("ResponseDeviceBusy", boolToString(errors.responseDeviceBusy()));
    xml.writeAttribute("ResponseIncorrectCrc", boolToString(errors.responseIncorrectCrc()));
    xml.writeAttribute("ResponseDelay", boolToString(errors.responseDelay()));
    xml.writeAttribute("ResponseRandomDelay", boolToString(errors.responseRandomDelay()));
    xml.writeAttribute("ResponseDelayTime", QString::number(errors.responseDelayTime()));
    xml.writeAttribute("ResponseRandomDelayUpToTime", QString::number(errors.responseRandomDelayUpToTime()));

    xml.writeEndElement();
    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param errors
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, ModbusErrorSimulations& errors)
{
    if (xml.isStartElement() && xml.name() == QLatin1String("ModbusErrorSimulations")) {
        QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("NoResponse")) {
            errors.setNoResponse(stringToBool(attributes.value("NoResponse").toString()));
        }

        if (attributes.hasAttribute("ResponseIncorrectId")) {
            errors.setResponseIncorrectId(stringToBool(attributes.value("ResponseIncorrectId").toString()));
        }

        if (attributes.hasAttribute("ResponseIllegalFunction")) {
            errors.setResponseIllegalFunction(stringToBool(attributes.value("ResponseIllegalFunction").toString()));
        }

        if (attributes.hasAttribute("ResponseDeviceBusy")) {
            errors.setResponseDeviceBusy(stringToBool(attributes.value("ResponseDeviceBusy").toString()));
        }

        if (attributes.hasAttribute("ResponseIncorrectCrc")) {
            errors.setResponseIncorrectCrc(stringToBool(attributes.value("ResponseIncorrectCrc").toString()));
        }

        if (attributes.hasAttribute("ResponseDelay")) {
            errors.setResponseDelay(stringToBool(attributes.value("ResponseDelay").toString()));
        }

        if (attributes.hasAttribute("ResponseRandomDelay")) {
            errors.setResponseRandomDelay(stringToBool(attributes.value("ResponseRandomDelay").toString()));
        }

        if (attributes.hasAttribute("ResponseDelayTime")) {
            bool ok;
            const int delayTime = attributes.value("ResponseDelayTime").toInt(&ok);
            if (ok && delayTime >= 0) {
                errors.setResponseDelayTime(delayTime);
            }
        }

        if (attributes.hasAttribute("ResponseRandomDelayUpToTime")) {
            bool ok;
            const int randomDelayTime = attributes.value("ResponseRandomDelayUpToTime").toInt(&ok);
            if (ok && randomDelayTime >= 0) {
                errors.setResponseRandomDelayUpToTime(randomDelayTime);
            }
        }

        xml.skipCurrentElement();
    }

    return xml;
}

#endif // MODBUSERRORSIMULATIONS_H
