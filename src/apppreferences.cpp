#include "fontutils.h"
#include "apppreferences.h"
#include "controls/applogoutput.h"
#include <QCoreApplication>

namespace {

///
/// \brief transl
/// \param source
/// \return
///
QString transl(const char* source)
{
    return QCoreApplication::translate("AppPreferences", source);
}

///
/// \brief boolToText
/// \param value
/// \return
///
QString boolToText(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

///
/// \brief addressBaseToText
/// \param zeroBased
/// \return
///
QString addressBaseToText(bool zeroBased)
{
    return zeroBased ? transl("0-based") : transl("1-based");
}

///
/// \brief enabledToText
/// \param enabled
/// \return
///
QString enabledToText(bool enabled)
{
    return enabled ? transl("enabled") : transl("disabled");
}

///
/// \brief fontToText
/// \param font
/// \return
///
QString fontToText(const QFont& font)
{
    const int pointSize = font.pointSize() > 0 ? font.pointSize() : 10;
    return QStringLiteral("%1, %2").arg(font.family()).arg(pointSize);
}

///
/// \brief logPreferenceChange
/// \param name
/// \param oldValue
/// \param newValue
///
void logPreferenceChange(const QString& name, const QString& oldValue, const QString& newValue)
{
    if (oldValue == newValue)
        return;

    qInfo(lcApp) << transl("Preference changed: %1: %2 -> %3").arg(name, oldValue, newValue);
}

///
/// \brief logSettingChange
/// \param name
/// \param oldValue
/// \param newValue
///
void logSettingChange(const QString& name, const QString& oldValue, const QString& newValue)
{
    if (oldValue == newValue)
        return;

    qInfo(lcApp) << transl("%1 changed: %2 -> %3").arg(name, oldValue, newValue);
}

}

///
/// \brief AppPreferences::AppPreferences
///
AppPreferences::AppPreferences()
    : _font(defaultMonospaceFont())
    , _scriptFont(defaultScriptFont())
{
}

///
/// \brief AppPreferences::instance
/// \return
///
AppPreferences& AppPreferences::instance()
{
    static AppPreferences inst;
    return inst;
}

///
/// \brief AppPreferences::setFont
/// \param f
///
void AppPreferences::setFont(const QFont& f)
{
    logPreferenceChange(transl("Font"), fontToText(_font), fontToText(f));
    _font = f;
}

///
/// \brief AppPreferences::setFontZoom
/// \param zoom
///
void AppPreferences::setFontZoom(int zoom)
{
    logPreferenceChange(transl("FontZoom"), QString::number(_fontZoom), QString::number(zoom));
    _fontZoom = zoom;
}

///
/// \brief AppPreferences::setBackgroundColor
/// \param c
///
void AppPreferences::setBackgroundColor(const QColor& c)
{
    logPreferenceChange(transl("BackgroundColor"), _backgroundColor.name(), c.name());
    _backgroundColor = c;
}

///
/// \brief AppPreferences::setForegroundColor
/// \param c
///
void AppPreferences::setForegroundColor(const QColor& c)
{
    logPreferenceChange(transl("ForegroundColor"), _foregroundColor.name(), c.name());
    _foregroundColor = c;
}

///
/// \brief AppPreferences::setStatusColor
/// \param c
///
void AppPreferences::setStatusColor(const QColor& c)
{
    logPreferenceChange(transl("StatusColor"), _statusColor.name(), c.name());
    _statusColor = c;
}

///
/// \brief AppPreferences::setAddressColor
/// \param c
///
void AppPreferences::setAddressColor(const QColor& c)
{
    logPreferenceChange(transl("AddressColor"), _addressColor.name(), c.name());
    _addressColor = c;
}

///
/// \brief AppPreferences::setCommentColor
/// \param c
///
void AppPreferences::setCommentColor(const QColor& c)
{
    logPreferenceChange(transl("CommentColor"), _commentColor.name(), c.name());
    _commentColor = c;
}

///
/// \brief AppPreferences::setCheckForUpdates
/// \param enable
///
void AppPreferences::setCheckForUpdates(bool enable)
{
    logPreferenceChange(transl("CheckForUpdates"), boolToText(_checkForUpdates), boolToText(enable));
    _checkForUpdates = enable;
}

///
/// \brief AppPreferences::setLanguage
/// \param lang
///
void AppPreferences::setLanguage(const QString& lang)
{
    logPreferenceChange(transl("Language"), _language, lang);
    _language = lang;
}

///
/// \brief AppPreferences::setDataViewDefinitions
/// \param def
///
void AppPreferences::setDataViewDefinitions(const DataViewDefinitions& def)
{
    logPreferenceChange(transl("DataView.FormName"),       _dataViewDefinitions.FormName, def.FormName);
    logPreferenceChange(transl("DataView.DeviceId"),       QString::number(_dataViewDefinitions.DeviceId), QString::number(def.DeviceId));
    logPreferenceChange(transl("DataView.PointAddress"),   QString::number(_dataViewDefinitions.PointAddress), QString::number(def.PointAddress));
    logPreferenceChange(transl("DataView.PointType"),      enumToString<QModbusDataUnit::RegisterType>(_dataViewDefinitions.PointType), enumToString<QModbusDataUnit::RegisterType>(def.PointType));
    logPreferenceChange(transl("DataView.Length"),         QString::number(_dataViewDefinitions.Length), QString::number(def.Length));
    logPreferenceChange(transl("DataView.ColumnsDistance"),QString::number(_dataViewDefinitions.DataViewColumnsDistance), QString::number(def.DataViewColumnsDistance));
    logPreferenceChange(transl("DataView.LeadingZeros"),   boolToText(_dataViewDefinitions.LeadingZeros),boolToText(def.LeadingZeros));

    _dataViewDefinitions = def;
}

///
/// \brief AppPreferences::setTrafficViewDefinitions
/// \param def
///
void AppPreferences::setTrafficViewDefinitions(const TrafficViewDefinitions& def)
{
    logPreferenceChange(transl("TrafficView.FormName"),            _trafficViewDefinitions.FormName, def.FormName);
    logPreferenceChange(transl("TrafficView.UnitFilter"),          QString::number(_trafficViewDefinitions.UnitFilter), QString::number(def.UnitFilter));
    logPreferenceChange(transl("TrafficView.FunctionCodeFilter"),  QString::number(_trafficViewDefinitions.FunctionCodeFilter), QString::number(def.FunctionCodeFilter));
    logPreferenceChange(transl("TrafficView.LogLimit"),            QString::number(_trafficViewDefinitions.LogViewLimit), QString::number(def.LogViewLimit));
    logPreferenceChange(transl("TrafficView.ExceptionsOnly"),      boolToText(_trafficViewDefinitions.ExceptionsOnly), boolToText(def.ExceptionsOnly));
    logPreferenceChange(transl("TrafficView.AutoScroll"),          boolToText(_trafficViewDefinitions.Autoscroll), boolToText(def.Autoscroll));
    logPreferenceChange(transl("TrafficView.HexView"),             boolToText(_trafficViewDefinitions.HexView), boolToText(def.HexView));

    _trafficViewDefinitions = def;
}

///
/// \brief AppPreferences::setScriptViewDefinitions
/// \param def
///
void AppPreferences::setScriptViewDefinitions(const ScriptViewDefinitions& def)
{
    logPreferenceChange(transl("ScriptView.FormName"),     _scriptViewDefinitions.FormName, def.FormName);
    logPreferenceChange(transl("ScriptView.RunMode"),      enumToString<RunMode>(_scriptViewDefinitions.ScriptCfg.Mode), enumToString<RunMode>(def.ScriptCfg.Mode));
    logPreferenceChange(transl("ScriptView.Interval"),     QString::number(_scriptViewDefinitions.ScriptCfg.Interval), QString::number(def.ScriptCfg.Interval));
    logPreferenceChange(transl("ScriptView.RunOnStartup"), boolToText(_scriptViewDefinitions.ScriptCfg.RunOnStartup), boolToText(def.ScriptCfg.RunOnStartup));

    _scriptViewDefinitions = def;
}

///
/// \brief AppPreferences::setGlobalZeroBasedAddress
/// \param value
///
void AppPreferences::setGlobalZeroBasedAddress(bool value)
{
    logSettingChange(transl("Address Base"), addressBaseToText(_globalZeroBasedAddress), addressBaseToText(value));
    _globalZeroBasedAddress = value;
}

///
/// \brief AppPreferences::setGlobalHexView
/// \param value
///
void AppPreferences::setGlobalHexView(bool value)
{
    logSettingChange(transl("Hex View"), enabledToText(_globalHexView), enabledToText(value));
    _globalHexView = value;
}

///
/// \brief AppPreferences::setScriptFont
/// \param f
///
void AppPreferences::setScriptFont(const QFont& f)
{
    logPreferenceChange(transl("ScriptFont"), fontToText(_scriptFont), fontToText(f));
    _scriptFont = f;
}

///
/// \brief AppPreferences::setCodeAutoComplete
/// \param enable
///
void AppPreferences::setCodeAutoComplete(bool enable)
{
    logPreferenceChange(transl("CodeAutoComplete"), boolToText(_codeAutoComplete), boolToText(enable));
    _codeAutoComplete = enable;
}

///
/// \brief AppPreferences::setAutoShowConsoleOutput
/// \param enable
///
void AppPreferences::setAutoShowConsoleOutput(bool enable)
{
    logPreferenceChange(transl("AutoShowConsoleOutput"), boolToText(_autoShowConsoleOutput), boolToText(enable));
    _autoShowConsoleOutput = enable;
}

///
/// \brief AppPreferences::setConsoleMaxLines
/// \param n
///
void AppPreferences::setConsoleMaxLines(int n)
{
    logPreferenceChange(transl("ConsoleMaxLines"), QString::number(_consoleMaxLines), QString::number(n));
    _consoleMaxLines = n;
}

///
/// \brief AppPreferences::load
/// \param settings
///
void AppPreferences::load(QSettings& settings)
{
    settings.beginGroup("AppPreferences");

    _font.fromString(settings.value("Font", _font.toString()).toString());
    _fontZoom        = settings.value("FontZoom",        _fontZoom).toInt();
    _backgroundColor = QColor(settings.value("BackgroundColor", _backgroundColor.name()).toString());
    _foregroundColor = QColor(settings.value("ForegroundColor", _foregroundColor.name()).toString());
    _statusColor     = QColor(settings.value("StatusColor",     _statusColor.name()).toString());
    _addressColor    = QColor(settings.value("AddressColor",    _addressColor.name()).toString());
    _commentColor    = QColor(settings.value("CommentColor",    _commentColor.name()).toString());
    _language        = settings.value("Language", _language).toString();
    _scriptFont.fromString(settings.value("ScriptFont", _scriptFont.toString()).toString());
    _codeAutoComplete = settings.value("CodeAutoComplete", _codeAutoComplete).toBool();
    _autoShowConsoleOutput = settings.value("AutoShowConsoleOutput", _autoShowConsoleOutput).toBool();
    _consoleMaxLines = settings.value("ConsoleMaxLines", _consoleMaxLines).toInt();
    _checkForUpdates  = settings.value("CheckForUpdates",  _checkForUpdates).toBool();

    settings >> _dataViewDefinitions;
    settings >> _trafficViewDefinitions;
    settings >> _scriptViewDefinitions;
    _globalZeroBasedAddress = settings.value("GlobalZeroBasedAddress", _globalZeroBasedAddress).toBool();
    _globalHexView = settings.value("GlobalHexView", false).toBool();

    settings.endGroup();
}

///
/// \brief AppPreferences::save
/// \param settings
///
void AppPreferences::save(QSettings& settings) const
{
    settings.beginGroup("AppPreferences");

    settings.setValue("Font",            _font.toString());
    settings.setValue("FontZoom",        _fontZoom);
    settings.setValue("BackgroundColor", _backgroundColor.name());
    settings.setValue("ForegroundColor", _foregroundColor.name());
    settings.setValue("StatusColor",     _statusColor.name());
    settings.setValue("AddressColor",    _addressColor.name());
    settings.setValue("CommentColor",    _commentColor.name());
    settings.setValue("Language",        _language);
    settings.setValue("ScriptFont",      _scriptFont.toString());
    settings.setValue("CodeAutoComplete",_codeAutoComplete);
    settings.setValue("AutoShowConsoleOutput", _autoShowConsoleOutput);
    settings.setValue("ConsoleMaxLines", _consoleMaxLines);
    settings.setValue("CheckForUpdates", _checkForUpdates);

    settings << _dataViewDefinitions;
    settings << _trafficViewDefinitions;
    settings << _scriptViewDefinitions;
    settings.setValue("GlobalZeroBasedAddress", _globalZeroBasedAddress);
    settings.setValue("GlobalHexView", _globalHexView);

    settings.endGroup();
}

///
/// \brief AppPreferences::saveXml
/// \param xml
///
void AppPreferences::saveXml(QXmlStreamWriter& xml) const
{
    xml.writeStartElement("AppPreferences");
    xml.writeAttribute("Font",            _font.toString());
    xml.writeAttribute("FontZoom",        QString::number(_fontZoom));
    xml.writeAttribute("BackgroundColor", _backgroundColor.name());
    xml.writeAttribute("ForegroundColor", _foregroundColor.name());
    xml.writeAttribute("StatusColor",     _statusColor.name());
    xml.writeAttribute("AddressColor",    _addressColor.name());
    xml.writeAttribute("CommentColor",    _commentColor.name());
    xml.writeAttribute("Language",        _language);
    xml.writeAttribute("ScriptFont",      _scriptFont.toString());
    xml.writeAttribute("CodeAutoComplete", boolToString(_codeAutoComplete));
    xml.writeAttribute("AutoShowConsoleOutput", boolToString(_autoShowConsoleOutput));
    xml.writeAttribute("ConsoleMaxLines", QString::number(_consoleMaxLines));
    xml.writeAttribute("GlobalZeroBasedAddress", boolToString(_globalZeroBasedAddress));
    xml.writeAttribute("GlobalHexView", boolToString(_globalHexView));
    xml << _dataViewDefinitions;
    xml << _trafficViewDefinitions;
    xml << _scriptViewDefinitions;
    xml.writeEndElement();
}

///
/// \brief AppPreferences::loadXml
/// \param xml
///
void AppPreferences::loadXml(QXmlStreamReader& xml)
{
    const QXmlStreamAttributes attributes = xml.attributes();

    if (attributes.hasAttribute("Font")) {
        _font.fromString(attributes.value("Font").toString());
    }

    if (attributes.hasAttribute("FontZoom")) {
        bool ok; const int zoom = attributes.value("FontZoom").toInt(&ok);
        if (ok) _fontZoom = zoom;
    }

    if (attributes.hasAttribute("BackgroundColor")) {
        _backgroundColor = QColor(attributes.value("BackgroundColor").toString());
    }

    if (attributes.hasAttribute("ForegroundColor")) {
        _foregroundColor = QColor(attributes.value("ForegroundColor").toString());
    }

    if (attributes.hasAttribute("StatusColor")) {
        _statusColor = QColor(attributes.value("StatusColor").toString());
    }

    if (attributes.hasAttribute("AddressColor")) {
        _addressColor = QColor(attributes.value("AddressColor").toString());
    }

    if (attributes.hasAttribute("CommentColor")) {
        _commentColor = QColor(attributes.value("CommentColor").toString());
    }

    if (attributes.hasAttribute("Language")) {
        _language = attributes.value("Language").toString();
    }

    if (attributes.hasAttribute("ScriptFont")) {
        _scriptFont.fromString(attributes.value("ScriptFont").toString());
    }

    if (attributes.hasAttribute("CodeAutoComplete")) {
        _codeAutoComplete = stringToBool(attributes.value("CodeAutoComplete").toString());
    }

    if (attributes.hasAttribute("AutoShowConsoleOutput")) {
        _autoShowConsoleOutput = stringToBool(attributes.value("AutoShowConsoleOutput").toString());
    }

    if (attributes.hasAttribute("ConsoleMaxLines")) {
        bool ok; const int n = attributes.value("ConsoleMaxLines").toInt(&ok);
        if (ok) _consoleMaxLines = n;
    }

    if (attributes.hasAttribute("GlobalZeroBasedAddress")) {
        _globalZeroBasedAddress = stringToBool(attributes.value("GlobalZeroBasedAddress").toString());
    }

    if (attributes.hasAttribute("GlobalHexView")) {
        _globalHexView = stringToBool(attributes.value("GlobalHexView").toString());
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("DataViewDefinitions")) {
            xml >> _dataViewDefinitions;
        } else if (xml.name() == QLatin1String("TrafficViewDefinitions")) {
            xml >> _trafficViewDefinitions;
        } else if (xml.name() == QLatin1String("ScriptViewDefinitions")) {
            xml >> _scriptViewDefinitions;
        } else {
            xml.skipCurrentElement();
        }
    }
}
