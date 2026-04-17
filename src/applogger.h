#ifndef APPLOGGER_H
#define APPLOGGER_H

#include <QObject>
#include "modbusmultiserver.h"
#include "datasimulator.h"

class QWidget;
class AppProject;
class AppPreferences;
class JScriptControl;

///
/// \brief The AppLogger class
///
class AppLogger final
{
public:
    static void setupModbusMultiServerLogging(ModbusMultiServer& server, QObject* context);
    static void setupDataSimulatorLogging(DataSimulator& simulator, QObject* context);
    static void setupAppProjectLogging(AppProject& project, QObject* context);
    static void setupAppPreferencesLogging(AppPreferences& preferences, QObject* context);
    static void setupScriptControlLogging(JScriptControl& control, QObject* context);
    static void clear();
};

#endif // APPLOGGER_H
