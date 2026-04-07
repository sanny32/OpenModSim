#include "applogger.h"

#include <QCoreApplication>
#include <QObject>
#include "controls/applogoutput.h"

namespace {

QString connectionAddress(const ConnectionDetails& cd)
{
    return (cd.Type == ConnectionType::Tcp)
        ? QString("%1:%2").arg(cd.TcpParams.IPAddress).arg(cd.TcpParams.ServicePort)
        : cd.SerialParams.PortName;
}

}

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

void AppLogger::logConnectionError(const QString& error)
{
    qCritical(lcApp) << error;
}

