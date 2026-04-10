#include "applogger.h"

#include <QCoreApplication>
#include <QMetaType>
#include <QObject>
#include "controls/applogoutput.h"

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
                            "Address space added: unit %1, %2, starting address %3 (0-based), length %4")
                            .arg(deviceId).arg(registerTypeName(type)).arg(addr).arg(len);
    });

    QObject::connect(&server, &ModbusMultiServer::unitMapRemoved, context,
                     [](QUuid /*id*/, quint8 deviceId) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Address space removed: unit %1")
                            .arg(deviceId);
    });
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
                                                "Manual write: unit %1, %2, address %3 (0-based), value %4")
                        .arg(params.DeviceId)
                        .arg(registerTypeName(pointType))
                        .arg(address0Based)
                        .arg(formatWriteValue(params.Value));
}

///
/// \brief AppLogger::logUserSimulationChanged
/// \param deviceId
/// \param pointType
/// \param address
/// \param zeroBasedAddress
/// \param mode
///
void AppLogger::logUserSimulationChanged(quint8 deviceId, QModbusDataUnit::RegisterType pointType,
                                         quint16 address, bool zeroBasedAddress, SimulationMode mode)
{
    const int address0Based = zeroBasedAddress
        ? static_cast<int>(address)
        : static_cast<int>(address) - 1;

    const QString state = (mode == SimulationMode::Off)
        ? QCoreApplication::translate("MainWindow", "disabled")
        : QCoreApplication::translate("MainWindow", "enabled (%1)").arg(enumToString(mode));

    qInfo(lcApp) << QCoreApplication::translate("MainWindow",
                                                "Auto simulation %1: unit %2, %3, address %4 (0-based)")
                        .arg(state)
                        .arg(deviceId)
                        .arg(registerTypeName(pointType))
                        .arg(address0Based);
}

