#ifndef APPLOGGER_H
#define APPLOGGER_H

#include "modbusmultiserver.h"

class QObject;

///
/// \brief The AppLogger class
///
class AppLogger final
{
public:
    static void setupModbusMultiServerLogging(ModbusMultiServer& server, QObject* context);
    static void logConnectionError(const QString& error);
};

#endif // APPLOGGER_H
