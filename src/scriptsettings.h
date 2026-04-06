#ifndef SCRIPTSETTINGS_H
#define SCRIPTSETTINGS_H

#include <QDataStream>
#include <QXmlStreamWriter>
#include "enums.h"

///
/// \brief The ScriptSettings struct
///
struct ScriptSettings
{
    RunMode Mode = RunMode::Periodically;
    uint Interval = 1000;
    bool RunOnStartup = false;

    void normalize()
    {
        Mode = qBound(RunMode::Once, Mode, RunMode::Periodically);
        Interval = qBound(500U, Interval, 10000U);
    }

    bool operator==(const ScriptSettings& o) const
    {
        return Mode == o.Mode && Interval == o.Interval && RunOnStartup == o.RunOnStartup;
    }
    bool operator!=(const ScriptSettings& o) const { return !(*this == o); }
};
Q_DECLARE_METATYPE(ScriptSettings)

///
/// \brief operator <<
/// \param out
/// \param ss
/// \return
///
inline QSettings& operator <<(QSettings& out, const ScriptSettings& ss)
{
    out.setValue("ScriptSettings/RunMode",          (int)ss.Mode);
    out.setValue("ScriptSettings/Interval",         ss.Interval);
    out.setValue("ScriptSettings/RunOnStartup",     ss.RunOnStartup);

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param ss
/// \return
///
inline QSettings& operator >>(QSettings& in, ScriptSettings& ss)
{
    ss.Mode = (RunMode)in.value("ScriptSettings/RunMode").toInt();
    ss.Interval = in.value("ScriptSettings/Interval", 1000).toUInt();
    ss.RunOnStartup = in.value("ScriptSettings/RunOnStartup", false).toBool();

    ss.normalize();
    return in;
}

///
/// \brief operator <<
/// \param xml
/// \param settings
/// \return
///
inline QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const ScriptSettings& settings)
{
    xml.writeStartElement("ScriptSettings");
    xml.writeAttribute("Mode", enumToString<RunMode>(settings.Mode));
    xml.writeAttribute("Interval", QString::number(settings.Interval));
    xml.writeAttribute("RunOnStartup", boolToString(settings.RunOnStartup));
    xml.writeEndElement();
    return xml;
}

///
/// \brief operator >>
/// \param xml
/// \param settings
/// \return
///
inline QXmlStreamReader& operator >>(QXmlStreamReader& xml, ScriptSettings& settings)
{
    if (xml.isStartElement() && xml.name() == QLatin1String("ScriptSettings")) {
        const QXmlStreamAttributes attributes = xml.attributes();

        if (attributes.hasAttribute("Mode")) {
            settings.Mode = enumFromString<RunMode>(attributes.value("Mode").toString());
        }

        if (attributes.hasAttribute("Interval")) {
            bool ok; const uint interval = attributes.value("Interval").toUInt(&ok);
            if (ok) settings.Interval = interval;
        }

        if (attributes.hasAttribute("RunOnStartup")) {
            settings.RunOnStartup = stringToBool(attributes.value("RunOnStartup").toString());
        }

        xml.skipCurrentElement();

        settings.normalize();
    }

    return xml;
}

#endif // SCRIPTSETTINGS_H

