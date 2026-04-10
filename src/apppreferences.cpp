#include "fontutils.h"
#include "apppreferences.h"
#include "controls/applogoutput.h"

namespace {

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
    return zeroBased ? QStringLiteral("0-based") : QStringLiteral("1-based");
}

///
/// \brief enabledToText
/// \param enabled
/// \return
///
QString enabledToText(bool enabled)
{
    return enabled ? QStringLiteral("enabled") : QStringLiteral("disabled");
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

    qInfo(lcApp).noquote() << QStringLiteral("Preference changed: %1: %2 -> %3")
                              .arg(name, oldValue, newValue);
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

    qInfo(lcApp).noquote() << QStringLiteral("%1: %2 -> %3")
                              .arg(name, oldValue, newValue);
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
    if (_font == f)
        return;
    logPreferenceChange(QStringLiteral("Font"), fontToText(_font), fontToText(f));
    _font = f;
}

///
/// \brief AppPreferences::setFontZoom
/// \param zoom
///
void AppPreferences::setFontZoom(int zoom)
{
    if (_fontZoom == zoom)
        return;
    logPreferenceChange(QStringLiteral("FontZoom"), QString::number(_fontZoom), QString::number(zoom));
    _fontZoom = zoom;
}

///
/// \brief AppPreferences::setBackgroundColor
/// \param c
///
void AppPreferences::setBackgroundColor(const QColor& c)
{
    if (_backgroundColor == c)
        return;
    logPreferenceChange(QStringLiteral("BackgroundColor"), _backgroundColor.name(), c.name());
    _backgroundColor = c;
}

///
/// \brief AppPreferences::setForegroundColor
/// \param c
///
void AppPreferences::setForegroundColor(const QColor& c)
{
    if (_foregroundColor == c)
        return;
    logPreferenceChange(QStringLiteral("ForegroundColor"), _foregroundColor.name(), c.name());
    _foregroundColor = c;
}

///
/// \brief AppPreferences::setStatusColor
/// \param c
///
void AppPreferences::setStatusColor(const QColor& c)
{
    if (_statusColor == c)
        return;
    logPreferenceChange(QStringLiteral("StatusColor"), _statusColor.name(), c.name());
    _statusColor = c;
}

///
/// \brief AppPreferences::setAddressColor
/// \param c
///
void AppPreferences::setAddressColor(const QColor& c)
{
    if (_addressColor == c)
        return;
    logPreferenceChange(QStringLiteral("AddressColor"), _addressColor.name(), c.name());
    _addressColor = c;
}

///
/// \brief AppPreferences::setCommentColor
/// \param c
///
void AppPreferences::setCommentColor(const QColor& c)
{
    if (_commentColor == c)
        return;
    logPreferenceChange(QStringLiteral("CommentColor"), _commentColor.name(), c.name());
    _commentColor = c;
}

///
/// \brief AppPreferences::setCheckForUpdates
/// \param enable
///
void AppPreferences::setCheckForUpdates(bool enable)
{
    if (_checkForUpdates == enable)
        return;
    logPreferenceChange(QStringLiteral("CheckForUpdates"), boolToText(_checkForUpdates), boolToText(enable));
    _checkForUpdates = enable;
}

///
/// \brief AppPreferences::setLanguage
/// \param lang
///
void AppPreferences::setLanguage(const QString& lang)
{
    if (_language == lang)
        return;
    logPreferenceChange(QStringLiteral("Language"), _language, lang);
    _language = lang;
}

///
/// \brief AppPreferences::setDataViewDefinitions
/// \param dd
///
void AppPreferences::setDataViewDefinitions(const DataViewDefinitions& dd)
{
    if (_dataViewDefinitions == dd)
        return;

    logPreferenceChange(QStringLiteral("DataView.FormName"), _dataViewDefinitions.FormName, dd.FormName);
    logPreferenceChange(QStringLiteral("DataView.DeviceId"), QString::number(_dataViewDefinitions.DeviceId), QString::number(dd.DeviceId));
    logPreferenceChange(QStringLiteral("DataView.PointAddress"), QString::number(_dataViewDefinitions.PointAddress), QString::number(dd.PointAddress));
    logPreferenceChange(QStringLiteral("DataView.PointType"),
                        enumToString<QModbusDataUnit::RegisterType>(_dataViewDefinitions.PointType),
                        enumToString<QModbusDataUnit::RegisterType>(dd.PointType));
    logPreferenceChange(QStringLiteral("DataView.Length"), QString::number(_dataViewDefinitions.Length), QString::number(dd.Length));
    logPreferenceChange(QStringLiteral("DataView.ColumnsDistance"),
                        QString::number(_dataViewDefinitions.DataViewColumnsDistance),
                        QString::number(dd.DataViewColumnsDistance));
    logPreferenceChange(QStringLiteral("DataView.LeadingZeros"),
                        boolToText(_dataViewDefinitions.LeadingZeros),
                        boolToText(dd.LeadingZeros));

    _dataViewDefinitions = dd;
}

///
/// \brief AppPreferences::setTrafficViewDefinitions
/// \param dd
///
void AppPreferences::setTrafficViewDefinitions(const TrafficViewDefinitions& dd)
{
    if (_trafficViewDefinitions == dd)
        return;

    logPreferenceChange(QStringLiteral("TrafficView.FormName"), _trafficViewDefinitions.FormName, dd.FormName);
    logPreferenceChange(QStringLiteral("TrafficView.UnitFilter"), QString::number(_trafficViewDefinitions.UnitFilter), QString::number(dd.UnitFilter));
    logPreferenceChange(QStringLiteral("TrafficView.FunctionCodeFilter"),
                        QString::number(_trafficViewDefinitions.FunctionCodeFilter),
                        QString::number(dd.FunctionCodeFilter));
    logPreferenceChange(QStringLiteral("TrafficView.LogLimit"), QString::number(_trafficViewDefinitions.LogViewLimit), QString::number(dd.LogViewLimit));
    logPreferenceChange(QStringLiteral("TrafficView.ExceptionsOnly"),
                        boolToText(_trafficViewDefinitions.ExceptionsOnly),
                        boolToText(dd.ExceptionsOnly));
    logPreferenceChange(QStringLiteral("TrafficView.AutoScroll"),
                        boolToText(_trafficViewDefinitions.Autoscroll),
                        boolToText(dd.Autoscroll));
    logPreferenceChange(QStringLiteral("TrafficView.HexView"),
                        boolToText(_trafficViewDefinitions.HexView),
                        boolToText(dd.HexView));

    _trafficViewDefinitions = dd;
}

///
/// \brief AppPreferences::setScriptViewDefinitions
/// \param dd
///
void AppPreferences::setScriptViewDefinitions(const ScriptViewDefinitions& dd)
{
    if (_scriptViewDefinitions == dd)
        return;

    logPreferenceChange(QStringLiteral("ScriptView.FormName"), _scriptViewDefinitions.FormName, dd.FormName);
    logPreferenceChange(QStringLiteral("ScriptView.RunMode"),
                        enumToString<RunMode>(_scriptViewDefinitions.ScriptCfg.Mode),
                        enumToString<RunMode>(dd.ScriptCfg.Mode));
    logPreferenceChange(QStringLiteral("ScriptView.Interval"),
                        QString::number(_scriptViewDefinitions.ScriptCfg.Interval),
                        QString::number(dd.ScriptCfg.Interval));
    logPreferenceChange(QStringLiteral("ScriptView.RunOnStartup"),
                        boolToText(_scriptViewDefinitions.ScriptCfg.RunOnStartup),
                        boolToText(dd.ScriptCfg.RunOnStartup));

    _scriptViewDefinitions = dd;
}

///
/// \brief AppPreferences::setGlobalZeroBasedAddress
/// \param value
///
void AppPreferences::setGlobalZeroBasedAddress(bool value)
{
    if (_globalZeroBasedAddress == value)
        return;
    logSettingChange(QStringLiteral("Address Base"),
                     addressBaseToText(_globalZeroBasedAddress),
                     addressBaseToText(value));
    _globalZeroBasedAddress = value;
}

///
/// \brief AppPreferences::setGlobalHexView
/// \param value
///
void AppPreferences::setGlobalHexView(bool value)
{
    if (_globalHexView == value)
        return;
    logSettingChange(QStringLiteral("Hex View"),
                     enabledToText(_globalHexView),
                     enabledToText(value));
    _globalHexView = value;
}

///
/// \brief AppPreferences::setScriptFont
/// \param f
///
void AppPreferences::setScriptFont(const QFont& f)
{
    if (_scriptFont == f)
        return;
    logPreferenceChange(QStringLiteral("ScriptFont"), fontToText(_scriptFont), fontToText(f));
    _scriptFont = f;
}

///
/// \brief AppPreferences::setCodeAutoComplete
/// \param enable
///
void AppPreferences::setCodeAutoComplete(bool enable)
{
    if (_codeAutoComplete == enable)
        return;
    logPreferenceChange(QStringLiteral("CodeAutoComplete"), boolToText(_codeAutoComplete), boolToText(enable));
    _codeAutoComplete = enable;
}

///
/// \brief AppPreferences::setAutoShowConsoleOutput
/// \param enable
///
void AppPreferences::setAutoShowConsoleOutput(bool enable)
{
    if (_autoShowConsoleOutput == enable)
        return;
    logPreferenceChange(QStringLiteral("AutoShowConsoleOutput"), boolToText(_autoShowConsoleOutput), boolToText(enable));
    _autoShowConsoleOutput = enable;
}

///
/// \brief AppPreferences::setConsoleMaxLines
/// \param n
///
void AppPreferences::setConsoleMaxLines(int n)
{
    if (_consoleMaxLines == n)
        return;
    logPreferenceChange(QStringLiteral("ConsoleMaxLines"), QString::number(_consoleMaxLines), QString::number(n));
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
