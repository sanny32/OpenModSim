#ifndef SCRIPTSETTINGS_H
#define SCRIPTSETTINGS_H

#include <QDataStream>
#include "enums.h"

///
/// \brief The ScriptSettings struct
///
struct ScriptSettings
{
    RunMode Mode = RunMode::Periodically;
    uint Interval = 1000;
    bool UseAutoComplete = true;
    bool RunOnStartup = false;

    void normalize()
    {
        Mode = qBound(RunMode::Once, Mode, RunMode::Periodically);
        Interval = qBound(500U, Interval, 10000U);
    }
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
    out.setValue("ScriptSettings/UseAutoComplete",  ss.UseAutoComplete);
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
    ss.UseAutoComplete = in.value("ScriptSettings/UseAutoComplete", true).toBool();
    ss.RunOnStartup = in.value("ScriptSettings/RunOnStartup", false).toBool();

    ss.normalize();
    return in;
}

///
/// \brief operator <<
/// \param out
/// \param ss
/// \return
///
inline QDataStream& operator <<(QDataStream& out, const ScriptSettings& ss)
{
    out << ss.Mode;
    out << ss.Interval;
    out << ss.UseAutoComplete;
    out << ss.RunOnStartup;

    return out;
}

///
/// \brief operator >>
/// \param in
/// \param ss
/// \return
///
inline QDataStream& operator >>(QDataStream& in, ScriptSettings& ss)
{
    in >> ss.Mode;
    in >> ss.Interval;
    in >> ss.UseAutoComplete;
    in >> ss.RunOnStartup;


    return in;
}

#endif // SCRIPTSETTINGS_H
