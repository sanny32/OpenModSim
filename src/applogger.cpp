#include "applogger.h"

#include <QCoreApplication>
#include <QWidget>
#include "apppreferences.h"
#include "appproject.h"
#include "applogoutput.h"
#include "formdatamapview.h"
#include "formdataview.h"
#include "formscriptview.h"
#include "formtrafficview.h"
#include "jscriptcontrol.h"

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
/// \brief formatWrittenValues
/// \param data
/// \param previewLimit
/// \return
///
QString formatWrittenValues(const QModbusDataUnit& data, int previewLimit = 16)
{
    if(data.valueCount() <= 0)
        return QStringLiteral("[]");

    if(data.valueCount() == 1)
        return QString::number(data.value(0));

    QStringList values;
    values.reserve(qMin<int>(data.valueCount(), previewLimit));

    const int previewCount = qMin<int>(data.valueCount(), previewLimit);
    for(int i = 0; i < previewCount; ++i)
        values << QString::number(data.value(i));

    const QString suffix = (data.valueCount() > previewLimit)
        ? QString(", ...")
        : QString();

    return QCoreApplication::translate("MainWindow", "[%1 values: %2%3]")
        .arg(data.valueCount())
        .arg(values.join(", "))
        .arg(suffix);
}

///
/// \brief formTypeName
/// \param form
/// \return
///
QString formTypeName(const QWidget* form)
{
    if (qobject_cast<const FormDataView*>(form))
        return QCoreApplication::translate("MainWindow", "Data");
    if (qobject_cast<const FormTrafficView*>(form))
        return QCoreApplication::translate("MainWindow", "Traffic");
    if (qobject_cast<const FormScriptView*>(form))
        return QCoreApplication::translate("MainWindow", "Script");
    if (qobject_cast<const FormDataMapView*>(form))
        return QCoreApplication::translate("MainWindow", "Map");
    return QCoreApplication::translate("MainWindow", "Form");
}

///
/// \brief formDisplayName
/// \param form
/// \return
///
QString formDisplayName(const QWidget* form)
{
    if (!form)
        return QCoreApplication::translate("MainWindow", "<null>");

    const QString title = form->windowTitle().isEmpty()
        ? QCoreApplication::translate("MainWindow", "<untitled>")
        : form->windowTitle();

    return QCoreApplication::translate("MainWindow", "%1 '%2'")
        .arg(formTypeName(form), title);
}

///
/// \brief preferenceLabel
/// \param key
/// \return
///
QString preferenceLabel(const QString& key)
{
    if (key == QLatin1String("Font")) return QCoreApplication::translate("MainWindow", "Font");
    if (key == QLatin1String("FontZoom")) return QCoreApplication::translate("MainWindow", "FontZoom");
    if (key == QLatin1String("BackgroundColor")) return QCoreApplication::translate("MainWindow", "BackgroundColor");
    if (key == QLatin1String("ForegroundColor")) return QCoreApplication::translate("MainWindow", "ForegroundColor");
    if (key == QLatin1String("AddressColor")) return QCoreApplication::translate("MainWindow", "AddressColor");
    if (key == QLatin1String("CommentColor")) return QCoreApplication::translate("MainWindow", "CommentColor");
    if (key == QLatin1String("CheckForUpdates")) return QCoreApplication::translate("MainWindow", "CheckForUpdates");
    if (key == QLatin1String("ShowWelcomeDialog")) return QCoreApplication::translate("MainWindow", "ShowWelcomeDialog");
    if (key == QLatin1String("Language")) return QCoreApplication::translate("MainWindow", "Language");
    if (key == QLatin1String("ScriptFont")) return QCoreApplication::translate("MainWindow", "ScriptFont");
    if (key == QLatin1String("CodeAutoComplete")) return QCoreApplication::translate("MainWindow", "CodeAutoComplete");
    if (key == QLatin1String("AutoShowConsoleOutput")) return QCoreApplication::translate("MainWindow", "AutoShowConsoleOutput");
    if (key == QLatin1String("ConsoleMaxLines")) return QCoreApplication::translate("MainWindow", "ConsoleMaxLines");
    if (key == QLatin1String("DataView.ColumnsDistance")) return QCoreApplication::translate("MainWindow", "DataView.ColumnsDistance");
    if (key == QLatin1String("DataView.LeadingZeros")) return QCoreApplication::translate("MainWindow", "DataView.LeadingZeros");
    if (key == QLatin1String("TrafficView.LogLimit")) return QCoreApplication::translate("MainWindow", "TrafficView.LogLimit");
    if (key == QLatin1String("TrafficView.AutoScroll")) return QCoreApplication::translate("MainWindow", "TrafficView.AutoScroll");
    if (key == QLatin1String("GlobalAddressBase")) return QCoreApplication::translate("MainWindow", "Address Base");
    if (key == QLatin1String("GlobalHexView")) return QCoreApplication::translate("MainWindow", "Hex View");
    return key;
}

///
/// \brief normalizedBoolText
/// \param value
/// \return
///
QString normalizedBoolText(const QString& value)
{
    if (value == QLatin1String("true"))
        return QCoreApplication::translate("MainWindow", "true");
    if (value == QLatin1String("false"))
        return QCoreApplication::translate("MainWindow", "false");
    return value;
}

///
/// \brief addressBaseText
/// \param value
/// \return
///
QString addressBaseText(const QString& value)
{
    if (value == QLatin1String("true"))
        return QCoreApplication::translate("MainWindow", "0-based");
    if (value == QLatin1String("false"))
        return QCoreApplication::translate("MainWindow", "1-based");
    return value;
}

///
/// \brief enabledText
/// \param value
/// \return
///
QString enabledText(const QString& value)
{
    if (value == QLatin1String("true"))
        return QCoreApplication::translate("MainWindow", "enabled");
    if (value == QLatin1String("false"))
        return QCoreApplication::translate("MainWindow", "disabled");
    return value;
}

///
/// \brief settingValueText
/// \param key
/// \param value
/// \return
///
QString settingValueText(const QString& key, const QString& value)
{
    if (key == QLatin1String("GlobalAddressBase"))
        return addressBaseText(value);
    if (key == QLatin1String("GlobalHexView"))
        return enabledText(value);
    return normalizedBoolText(value);
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
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::disconnected, context,
                     [](const ConnectionDetails& cd) {
        qWarning(lcApp) << QCoreApplication::translate("MainWindow", "Server disconnected: %1")
                               .arg(connectionAddress(cd));
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::connectionError, context,
                     [](const QString& error) {
        qCritical(lcApp) << error;
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::clientConnected, context,
                     [](const ModbusClientInfo& client) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Modbus client connected: %1 -> %2")
                            .arg(QString("%1:%2").arg(client.Address).arg(client.Port))
                            .arg(connectionAddress(client.Connection));
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::clientDisconnected, context,
                     [](const ModbusClientInfo& client) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Modbus client disconnected: %1 -> %2")
                            .arg(QString("%1:%2").arg(client.Address).arg(client.Port))
                            .arg(connectionAddress(client.Connection));
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::errorOccured, context,
                     [](quint8 deviceId, const QString& error) {
        qCritical(lcApp) << QCoreApplication::translate("MainWindow", "[Unit %1] %2")
                                .arg(deviceId).arg(error);
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::deviceIdAdded, context,
                     [](quint8 deviceId) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Unit ID added: %1")
                            .arg(deviceId);
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::deviceIdRemoved, context,
                     [](quint8 deviceId) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Unit ID removed: %1")
                            .arg(deviceId);
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::unitMapAdded, context,
                     [](QUuid /*id*/, quint8 deviceId, QModbusDataUnit::RegisterType type,
                        quint16 addr, quint16 len) {
        qInfo(lcApp) << QCoreApplication::translate(
                            "MainWindow",
                            "Address space added: unit %1, %2, starting address %3, length %4")
                            .arg(deviceId).arg(registerTypeName(type)).arg(addr).arg(len);
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::unitMapRemoved, context,
                     [](QUuid /*id*/, quint8 deviceId, QModbusDataUnit::RegisterType type,
                        quint16 addr, quint16 len) {
        qInfo(lcApp) << QCoreApplication::translate(
                            "MainWindow",
                            "Address space removed: unit %1, %2, starting address %3, length %4")
                            .arg(deviceId).arg(registerTypeName(type)).arg(addr).arg(len);
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::descriptionChanged, context,
                     [](quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 address,
                        const QString& description, WriteSource source) {
        if (source != WriteSource::User)
            return;

        const QString text = description.isEmpty()
            ? QCoreApplication::translate("MainWindow", "<empty>")
            : description;
        qInfo(lcApp) << QCoreApplication::translate(
                            "MainWindow",
                            "Address comment changed: unit %1, %2, address %3: '%4'")
                            .arg(deviceId)
                            .arg(registerTypeName(type))
                            .arg(address)
                            .arg(text);
    }, Qt::DirectConnection);

    QObject::connect(&server, &ModbusMultiServer::dataChanged, context,
                     [](quint8 deviceId, const QModbusDataUnit& data, WriteSource source,
                        const ModbusClientInfo& client) {
        switch(source) {
            case WriteSource::User:
                qInfo(lcApp) << QCoreApplication::translate("MainWindow",
                                                        "Manual write: unit %1, %2, starting address %3, value %4")
                                .arg(deviceId)
                                .arg(registerTypeName(data.registerType()))
                                .arg(data.startAddress())
                                .arg(formatWrittenValues(data));
            break;
            case WriteSource::ModbusClient:
            {
                const QString endpoint = client.Address.isEmpty()
                    ? QCoreApplication::translate("MainWindow", "unknown client")
                    : QString("%1:%2").arg(client.Address).arg(client.Port);

                qInfo(lcApp) << QCoreApplication::translate("MainWindow",
                                                            "Client write: %1 -> unit %2, %3, starting address %4, value %5")
                                    .arg(endpoint)
                                    .arg(deviceId)
                                    .arg(registerTypeName(data.registerType()))
                                    .arg(data.startAddress())
                                    .arg(formatWrittenValues(data));
            }
            break;
            default: break;
        }
    }, Qt::DirectConnection);
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
    });

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
    });
}

///
/// \brief AppLogger::setupAppProjectLogging
/// \param project
/// \param context
///
void AppLogger::setupAppProjectLogging(AppProject& project, QObject* context)
{
    Q_ASSERT(context != nullptr);

    QObject::connect(&project, &AppProject::projectOpened, context,
                     [](const QString& filename) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Project opened: %1")
                            .arg(filename);
    }, Qt::DirectConnection);

    QObject::connect(&project, &AppProject::projectClosed, context,
                     [](const QString& filename) {
        const QString displayName = filename.isEmpty()
            ? QCoreApplication::translate("MainWindow", "<unsaved>")
            : filename;

        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Project closed: %1")
                            .arg(displayName);
    }, Qt::DirectConnection);

    QObject::connect(&project, &AppProject::projectSaved, context,
                     [](const QString& filename) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Project saved: %1")
                            .arg(filename);
    }, Qt::DirectConnection);

    QObject::connect(&project, &AppProject::projectSaveFailed, context,
                     [](const QString& filename, const QString& error) {
        qWarning(lcApp) << QCoreApplication::translate("MainWindow",
                                                       "Project save failed: %1 (%2)")
                               .arg(filename, error);
    }, Qt::DirectConnection);

    QObject::connect(&project, &AppProject::formCreated, context,
                     [](QWidget* form) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Form created: %1")
                            .arg(formDisplayName(form));
    }, Qt::DirectConnection);

    QObject::connect(&project, &AppProject::formOpened, context,
                     [](QWidget* form) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Form opened: %1")
                            .arg(formDisplayName(form));
    }, Qt::DirectConnection);

    QObject::connect(&project, &AppProject::formClosed, context,
                     [](QWidget* form) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Form closed: %1")
                            .arg(formDisplayName(form));
    }, Qt::DirectConnection);

    QObject::connect(&project, &AppProject::formDeleted, context,
                     [](QWidget* form) {
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Form deleted: %1")
                            .arg(formDisplayName(form));
    }, Qt::DirectConnection);
}

///
/// \brief AppLogger::setupAppPreferencesLogging
/// \param preferences
/// \param context
///
void AppLogger::setupAppPreferencesLogging(AppPreferences& preferences, QObject* context)
{
    Q_ASSERT(context != nullptr);

    QObject::connect(&preferences, &AppPreferences::preferenceChanged, context,
                     [](const QString& name, const QString& oldValue, const QString& newValue) {
        if (oldValue == newValue)
            return;

        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Preference changed: %1: %2 -> %3")
                            .arg(preferenceLabel(name), normalizedBoolText(oldValue), normalizedBoolText(newValue));
    }, Qt::DirectConnection);

    QObject::connect(&preferences, &AppPreferences::settingChanged, context,
                     [](const QString& name, const QString& oldValue, const QString& newValue) {
        if (oldValue == newValue)
            return;

        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "%1 changed: %2 -> %3")
                            .arg(preferenceLabel(name), settingValueText(name, oldValue), settingValueText(name, newValue));
    }, Qt::DirectConnection);
}

///
/// \brief AppLogger::setupScriptControlLogging
/// \param control
/// \param context
///
void AppLogger::setupScriptControlLogging(JScriptControl& control, QObject* context)
{
    Q_ASSERT(context != nullptr);

    QObject::connect(&control, &JScriptControl::scriptStarted, context,
                     [&control]() {
        control.setProperty("_appLoggerScriptRunning", true);
        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Script started: %1")
                            .arg(control.scriptSource());
    }, Qt::DirectConnection);

    QObject::connect(&control, &JScriptControl::scriptStopped, context,
                     [&control]() {
        const bool wasRunning = control.property("_appLoggerScriptRunning").toBool();
        control.setProperty("_appLoggerScriptRunning", false);
        if (!wasRunning)
            return;

        qInfo(lcApp) << QCoreApplication::translate("MainWindow", "Script stopped: %1")
                            .arg(control.scriptSource());
    }, Qt::DirectConnection);
}

///
/// \brief AppLogger::clear
///
void AppLogger::clear()
{
    if (auto* log = AppLogOutput::instance())
        log->clear();
}
