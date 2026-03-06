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

    DisplayDefinition dd;
    settings >> dd;
    _displayDefinition = dd;

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

    settings << _displayDefinition;

    settings.endGroup();
}
