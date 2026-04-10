#ifndef APPLOGGER_H
#define APPLOGGER_H

#include "modbusmultiserver.h"
#include "modbuswriteparams.h"

class QObject;

///
/// \brief The AppLogger class
///
class AppLogger final
{
public:
    static void setupModbusMultiServerLogging(ModbusMultiServer& server, QObject* context);
    static void logConnectionError(const QString& error);
    static void logUserRegisterWrite(QModbusDataUnit::RegisterType pointType, const ModbusWriteParams& params);
    static void logUserSimulationChanged(quint8 deviceId, QModbusDataUnit::RegisterType pointType,
                                         quint16 address, bool zeroBasedAddress, SimulationMode mode);
};

#endif // APPLOGGER_H

