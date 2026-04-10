#include "applogger.h"

#include <QCoreApplication>
#include <QMetaType>
#include "applogoutput.h"

namespace {

///
/// \brief connectionAddress
/// \param cd
/// \return
///
QString connectionAddress(const ConnectionDetails& cd)
{
    return (cd.Type == ConnectionType::Tcp)
        ? QString("%1:%2").arg(cd.TcpParams.IPAddress).arg(cd.TcpParams.ServicePort)
        : cd.SerialParams.PortName;
}

///
/// \brief formatWriteValue
/// \param value
/// \return
///
QString formatWriteValue(const QVariant& value)
{
    if(value.userType() == qMetaTypeId<QVector<quint16>>()) {
        const auto values = value.value<QVector<quint16>>();
        return QString("[%1 values]").arg(values.size());
    }

    if(value.userType() == QMetaType::Bool)
        return value.toBool() ? QStringLiteral("1") : QStringLiteral("0");

    return value.toString();
}

}

///
/// \brief AppLogger::setupModbusMultiServerLogging
/// \param server
/// \param context
///
void AppLogger::setupModbusMultiServerLogging(ModbusMultiServer& server, QObject* context)
{
    Q_ASSERT(context != nullptr);

    QObject::connect(&server, &ModbusMultiServer::connected, context,
                     [](const ConnectionDetails& cd) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Server connected: %1")
                            .arg(connectionAddress(cd));
    });

    QObject::connect(&server, &ModbusMultiServer::disconnected, context,
                     [](const ConnectionDetails& cd) {
        qWarning(lcApp) << QCoreApplication::translate("MainWindow", "Server disconnected: %1")
                               .arg(connectionAddress(cd));
    });

    QObject::connect(&server, &ModbusMultiServer::clientConnected, context,
                     [](const ConnectionDetails& cd, const QString& clientAddress, quint16 clientPort) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Modbus client connected: %1 -> %2")
                            .arg(QString("%1:%2").arg(clientAddress).arg(clientPort))
                            .arg(connectionAddress(cd));
    });

    QObject::connect(&server, &ModbusMultiServer::clientDisconnected, context,
                     [](const ConnectionDetails& cd, const QString& clientAddress, quint16 clientPort) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Modbus client disconnected: %1 -> %2")
                            .arg(QString("%1:%2").arg(clientAddress).arg(clientPort))
                            .arg(connectionAddress(cd));
    });

    QObject::connect(&server, &ModbusMultiServer::errorOccured, context,
                     [](quint8 deviceId, const QString& error) {
        qCritical(lcApp) << QCoreApplication::translate("MainWindow", "[Unit %1] %2")
                                .arg(deviceId).arg(error);
    });

    QObject::connect(&server, &ModbusMultiServer::deviceIdAdded, context,
                     [](quint8 deviceId) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Unit ID added: %1")
                            .arg(deviceId);
    });

    QObject::connect(&server, &ModbusMultiServer::deviceIdRemoved, context,
                     [](quint8 deviceId) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Unit ID removed: %1")
                            .arg(deviceId);
    });

    QObject::connect(&server, &ModbusMultiServer::unitMapAdded, context,
                     [](QUuid /*id*/, quint8 deviceId, QModbusDataUnit::RegisterType type,
                        quint16 addr, quint16 len) {
        qInfo(lcApp) << QCoreApplication::translate(
                            "MainWindow",
                            "Address space added: unit %1, %2, starting address %3, length %4")
                            .arg(deviceId).arg(registerTypeName(type)).arg(addr).arg(len);
    });

    QObject::connect(&server, &ModbusMultiServer::unitMapRemoved, context,
                     [](QUuid /*id*/, quint8 deviceId) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Address space removed: unit %1")
                            .arg(deviceId);
    });
}

///
/// \brief AppLogger::setupDataSimulatorLogging
/// \param simulator
/// \param context
///
void AppLogger::setupDataSimulatorLogging(DataSimulator& simulator, QObject* context)
{
    Q_ASSERT(context != nullptr);

    QObject::connect(&simulator, &DataSimulator::simulationStarted, context,
                     [&simulator](DataType /*type*/, RegisterOrder /*order*/, quint8 deviceId,
                                  QModbusDataUnit::RegisterType regType, const QVector<quint16>& addresses) {
        if (addresses.isEmpty())
            return;

        const quint16 address = addresses.first();
        const auto mode = simulator.simulationParams(deviceId, regType, address).Mode;
        qInfo(lcApp) << QCoreApplication::translate("MainWindow",
                                                    "Auto simulation enabled (%1): unit %2, %3, address %4")
                            .arg(enumToString(mode))
                            .arg(deviceId)
                            .arg(registerTypeName(regType))
                            .arg(address);
    }, Qt::QueuedConnection);

    QObject::connect(&simulator, &DataSimulator::simulationStopped, context,
                     [](DataType /*type*/, RegisterOrder /*order*/, quint8 deviceId,
                        QModbusDataUnit::RegisterType regType, const QVector<quint16>& addresses) {
        if (addresses.isEmpty())
            return;

        qInfo(lcApp) << QCoreApplication::translate("MainWindow",
                                                    "Auto simulation disabled: unit %1, %2, address %3")
                            .arg(deviceId)
                            .arg(registerTypeName(regType))
                            .arg(addresses.first());
    }, Qt::QueuedConnection);
}

///
/// \brief AppLogger::logConnectionError
/// \param error
///
void AppLogger::logConnectionError(const QString& error)
{
    qCritical(lcApp) << error;
}

///
/// \brief AppLogger::logUserRegisterWrite
/// \param pointType
/// \param params
///
void AppLogger::logUserRegisterWrite(QModbusDataUnit::RegisterType pointType, const ModbusWriteParams& params)
{
    const int address0Based = params.ZeroBasedAddress
        ? static_cast<int>(params.Address)
        : static_cast<int>(params.Address) - 1;

    qInfo(lcApp) << QCoreApplication::translate("MainWindow",
                                                "Manual write: unit %1, %2, address %3, value %4")
                        .arg(params.DeviceId)
                        .arg(registerTypeName(pointType))
                        .arg(address0Based)
                        .arg(formatWriteValue(params.Value));
}
