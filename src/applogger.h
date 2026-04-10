#ifndef APPLOGGER_H
#define APPLOGGER_H

#include <QObject>
#include "modbusmultiserver.h"
#include "datasimulator.h"

///
/// \brief The AppLogger class
///
class AppLogger final
{
public:
    static void setupModbusMultiServerLogging(ModbusMultiServer& server, QObject* context);
    static void setupDataSimulatorLogging(DataSimulator& simulator, QObject* context);
    static void logConnectionError(const QString& error);
};

#endif // APPLOGGER_H
