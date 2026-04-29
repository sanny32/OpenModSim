#include "fontutils.h"
#include "enums.h"
#include "apppreferences.h"

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
/// \brief fontToText
/// \param font
/// \return
///
QString fontToText(const QFont& font)
{
    const int pointSize = font.pointSize() > 0 ? font.pointSize() : 10;
    return QStringLiteral("%1, %2").arg(font.family()).arg(pointSize);
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

    emit preferenceChanged("Font", fontToText(_font), fontToText(f));
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

    emit preferenceChanged("FontZoom", QString::number(_fontZoom), QString::number(zoom));
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

    emit preferenceChanged("BackgroundColor", _backgroundColor.name(), c.name());
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

    emit preferenceChanged("ForegroundColor", _foregroundColor.name(), c.name());
    _foregroundColor = c;
}

///
/// \brief AppPreferences::setAddressColor
/// \param c
///
void AppPreferences::setAddressColor(const QColor& c)
{
    if (_addressColor == c)
        return;

    emit preferenceChanged("AddressColor", _addressColor.name(), c.name());
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

    emit preferenceChanged("CommentColor", _commentColor.name(), c.name());
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

    emit preferenceChanged("CheckForUpdates", boolToText(_checkForUpdates), boolToText(enable));
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

    emit preferenceChanged("Language", _language, lang);
    _language = lang;
}

///
/// \brief AppPreferences::setDataViewDefinitions
/// \param def
///
void AppPreferences::setDataViewDefinitions(const DataViewDefinitions& def)
{
    emit preferenceChanged("DataView.FormName", _dataViewDefinitions.FormName, def.FormName);
    emit preferenceChanged("DataView.DeviceId", QString::number(_dataViewDefinitions.DeviceId), QString::number(def.DeviceId));
    emit preferenceChanged("DataView.PointAddress", QString::number(_dataViewDefinitions.PointAddress), QString::number(def.PointAddress));
    emit preferenceChanged("DataView.PointType", enumToString<QModbusDataUnit::RegisterType>(_dataViewDefinitions.PointType), enumToString<QModbusDataUnit::RegisterType>(def.PointType));
    emit preferenceChanged("DataView.Length", QString::number(_dataViewDefinitions.Length), QString::number(def.Length));
    emit preferenceChanged("DataView.ColumnsDistance", QString::number(_dataViewDefinitions.DataViewColumnsDistance), QString::number(def.DataViewColumnsDistance));
    emit preferenceChanged("DataView.LeadingZeros", boolToText(_dataViewDefinitions.LeadingZeros), boolToText(def.LeadingZeros));

    _dataViewDefinitions = def;
}

///
/// \brief AppPreferences::setTrafficViewDefinitions
/// \param def
///
void AppPreferences::setTrafficViewDefinitions(const TrafficViewDefinitions& def)
{
    emit preferenceChanged("TrafficView.FormName", _trafficViewDefinitions.FormName, def.FormName);
    emit preferenceChanged("TrafficView.UnitFilter", QString::number(_trafficViewDefinitions.UnitFilter), QString::number(def.UnitFilter));
    emit preferenceChanged("TrafficView.FunctionCodeFilter", QString::number(_trafficViewDefinitions.FunctionCodeFilter), QString::number(def.FunctionCodeFilter));
    emit preferenceChanged("TrafficView.LogLimit", QString::number(_trafficViewDefinitions.LogViewLimit), QString::number(def.LogViewLimit));
    emit preferenceChanged("TrafficView.ExceptionsOnly", boolToText(_trafficViewDefinitions.ExceptionsOnly), boolToText(def.ExceptionsOnly));
    emit preferenceChanged("TrafficView.AutoScroll", boolToText(_trafficViewDefinitions.Autoscroll), boolToText(def.Autoscroll));
    emit preferenceChanged("TrafficView.HexView", boolToText(_trafficViewDefinitions.HexView), boolToText(def.HexView));

    _trafficViewDefinitions = def;
}

///
/// \brief AppPreferences::setScriptViewDefinitions
/// \param def
///
void AppPreferences::setScriptViewDefinitions(const ScriptViewDefinitions& def)
{
    emit preferenceChanged("ScriptView.FormName", _scriptViewDefinitions.FormName, def.FormName);
    emit preferenceChanged("ScriptView.RunMode", enumToString<RunMode>(_scriptViewDefinitions.ScriptCfg.Mode), enumToString<RunMode>(def.ScriptCfg.Mode));
    emit preferenceChanged("ScriptView.Interval", QString::number(_scriptViewDefinitions.ScriptCfg.Interval), QString::number(def.ScriptCfg.Interval));
    emit preferenceChanged("ScriptView.RunOnStartup", boolToText(_scriptViewDefinitions.ScriptCfg.RunOnStartup), boolToText(def.ScriptCfg.RunOnStartup));

    _scriptViewDefinitions = def;
}

///
/// \brief AppPreferences::setGlobalAddressBase
/// \param base
///
void AppPreferences::setGlobalAddressBase(AddressBase base)
{
    if (_globalAddressBase == base)
        return;

    emit settingChanged("GlobalAddressBase", boolToText(_globalAddressBase == AddressBase::Base0), boolToText(base == AddressBase::Base0));
    _globalAddressBase = base;
}

///
/// \brief AppPreferences::setGlobalHexView
/// \param value
///
void AppPreferences::setGlobalHexView(bool value)
{
    if (_globalHexView == value)
        return;

    const QString oldValue = boolToText(_globalHexView);
    const QString newValue = boolToText(value);
    _globalHexView = value;
    emit settingChanged("GlobalHexView", oldValue, newValue);
}

///
/// \brief AppPreferences::setScriptFont
/// \param f
///
void AppPreferences::setScriptFont(const QFont& f)
{
    if (_scriptFont == f)
        return;

    emit preferenceChanged("ScriptFont", fontToText(_scriptFont), fontToText(f));
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

    emit preferenceChanged("CodeAutoComplete", boolToText(_codeAutoComplete), boolToText(enable));
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

    emit preferenceChanged("AutoShowConsoleOutput", boolToText(_autoShowConsoleOutput), boolToText(enable));
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

    emit preferenceChanged("ConsoleMaxLines", QString::number(_consoleMaxLines), QString::number(n));
    _consoleMaxLines = n;
}

///
/// \brief AppPreferences::setShowWelcomeDialog
/// \param enable
///
void AppPreferences::setShowWelcomeDialog(bool enable)
{
    if (_showWelcomeDialog == enable)
        return;

    emit preferenceChanged("ShowWelcomeDialog", boolToText(_showWelcomeDialog), boolToText(enable));
    _showWelcomeDialog = enable;
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
    _addressColor    = QColor(settings.value("AddressColor",    _addressColor.name()).toString());
    _commentColor    = QColor(settings.value("CommentColor",    _commentColor.name()).toString());
    _language        = settings.value("Language", _language).toString();
    _scriptFont.fromString(settings.value("ScriptFont", _scriptFont.toString()).toString());
    _codeAutoComplete = settings.value("CodeAutoComplete", _codeAutoComplete).toBool();
    _autoShowConsoleOutput = settings.value("AutoShowConsoleOutput", _autoShowConsoleOutput).toBool();
    _consoleMaxLines = settings.value("ConsoleMaxLines", _consoleMaxLines).toInt();
    _checkForUpdates  = settings.value("CheckForUpdates",  _checkForUpdates).toBool();
    _showWelcomeDialog = settings.value("ShowWelcomeDialog", _showWelcomeDialog).toBool();

    settings >> _dataViewDefinitions;
    settings >> _trafficViewDefinitions;
    settings >> _scriptViewDefinitions;
    _globalAddressBase = settings.value("GlobalZeroBasedAddress", false).toBool() ? AddressBase::Base0 : AddressBase::Base1;
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
    settings.setValue("AddressColor",    _addressColor.name());
    settings.setValue("CommentColor",    _commentColor.name());
    settings.setValue("Language",        _language);
    settings.setValue("ScriptFont",      _scriptFont.toString());
    settings.setValue("CodeAutoComplete",_codeAutoComplete);
    settings.setValue("AutoShowConsoleOutput", _autoShowConsoleOutput);
    settings.setValue("ConsoleMaxLines", _consoleMaxLines);
    settings.setValue("CheckForUpdates", _checkForUpdates);
    settings.setValue("ShowWelcomeDialog", _showWelcomeDialog);

    settings << _dataViewDefinitions;
    settings << _trafficViewDefinitions;
    settings << _scriptViewDefinitions;
    settings.setValue("GlobalZeroBasedAddress", _globalAddressBase == AddressBase::Base0);
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
    xml.writeAttribute("AddressColor",    _addressColor.name());
    xml.writeAttribute("CommentColor",    _commentColor.name());
    xml.writeAttribute("Language",        _language);
    xml.writeAttribute("ScriptFont",      _scriptFont.toString());
    xml.writeAttribute("CodeAutoComplete", boolToString(_codeAutoComplete));
    xml.writeAttribute("AutoShowConsoleOutput", boolToString(_autoShowConsoleOutput));
    xml.writeAttribute("ConsoleMaxLines", QString::number(_consoleMaxLines));
    xml.writeAttribute("GlobalZeroBasedAddress", boolToString(_globalAddressBase == AddressBase::Base0));
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
        _globalAddressBase = stringToBool(attributes.value("GlobalZeroBasedAddress").toString()) ? AddressBase::Base0 : AddressBase::Base1;
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
