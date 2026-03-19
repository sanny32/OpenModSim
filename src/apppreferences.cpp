#include "fontutils.h"
#include "apppreferences.h"

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
    _language        = settings.value("Language", _language).toString();
    _scriptFont.fromString(settings.value("ScriptFont", _scriptFont.toString()).toString());
    _codeAutoComplete = settings.value("CodeAutoComplete", _codeAutoComplete).toBool();
    _checkForUpdates  = settings.value("CheckForUpdates",  _checkForUpdates).toBool();

    settings >> _dataViewDefinitions;
    settings >> _trafficViewDefinitions;
    settings >> _scriptViewDefinitions;

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
    settings.setValue("Language",        _language);
    settings.setValue("ScriptFont",      _scriptFont.toString());
    settings.setValue("CodeAutoComplete",_codeAutoComplete);
    settings.setValue("CheckForUpdates", _checkForUpdates);

    settings << _dataViewDefinitions;
    settings << _trafficViewDefinitions;
    settings << _scriptViewDefinitions;

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
    xml.writeAttribute("Language",        _language);
    xml.writeAttribute("ScriptFont",      _scriptFont.toString());
    xml.writeAttribute("CodeAutoComplete", boolToString(_codeAutoComplete));
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

    if (attributes.hasAttribute("Language")) {
        _language = attributes.value("Language").toString();
    }

    if (attributes.hasAttribute("ScriptFont")) {
        _scriptFont.fromString(attributes.value("ScriptFont").toString());
    }

    if (attributes.hasAttribute("CodeAutoComplete")) {
        _codeAutoComplete = stringToBool(attributes.value("CodeAutoComplete").toString());
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
