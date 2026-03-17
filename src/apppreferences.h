#ifndef APPPREFERENCES_H
#define APPPREFERENCES_H

#include <QFont>
#include <QColor>
#include <QSettings>
#include <QXmlStreamWriter>
#include "displaydefinition.h"
#include "translationutils.h"

///
/// \brief The AppPreferences class
///
/// Singleton storing application-wide preferences (defaults for new windows,
/// language, Modbus settings and script-editor options). Preferences are persisted
/// via load()/save() which operate on the currently active QSettings group.
///
class AppPreferences
{
public:
    static AppPreferences& instance();

    // ----- Font & Colors -----
    QFont font() const { return _font; }
    void setFont(const QFont& f) { _font = f; }

    int fontZoom() const { return _fontZoom; }
    void setFontZoom(int zoom) { _fontZoom = zoom; }

    QColor backgroundColor() const { return _backgroundColor; }
    void setBackgroundColor(const QColor& c) { _backgroundColor = c; }

    QColor foregroundColor() const { return _foregroundColor; }
    void setForegroundColor(const QColor& c) { _foregroundColor = c; }

    QColor statusColor() const { return _statusColor; }
    void setStatusColor(const QColor& c) { _statusColor = c; }

    // ----- Updates -----
    bool checkForUpdates() const { return _checkForUpdates; }
    void setCheckForUpdates(bool enable) { _checkForUpdates = enable; }

    // ----- Language -----
    QString language() const { return _language; }
    void setLanguage(const QString& lang) { _language = lang; }

    // ----- Default View Definitions -----
    DataViewDefinitions dataViewDefinitions() const { return _dataViewDefinitions; }
    void setDataViewDefinitions(const DataViewDefinitions& dd) { _dataViewDefinitions = dd; }

    TrafficViewDefinitions trafficViewDefinitions() const { return _trafficViewDefinitions; }
    void setTrafficViewDefinitions(const TrafficViewDefinitions& dd) { _trafficViewDefinitions = dd; }

    ScriptViewDefinitions scriptViewDefinitions() const { return _scriptViewDefinitions; }
    void setScriptViewDefinitions(const ScriptViewDefinitions& dd) { _scriptViewDefinitions = dd; }

    // ----- Script Editor -----
    QFont scriptFont() const { return _scriptFont; }
    void setScriptFont(const QFont& f) { _scriptFont = f; }

    bool codeAutoComplete() const { return _codeAutoComplete; }
    void setCodeAutoComplete(bool enable) { _codeAutoComplete = enable; }

    // ----- Persistence -----
    void load(QSettings& settings);
    void save(QSettings& settings) const;

    void loadXml(QXmlStreamReader& xml);
    void saveXml(QXmlStreamWriter& xml) const;

private:
    AppPreferences();
    AppPreferences(const AppPreferences&) = delete;
    AppPreferences& operator=(const AppPreferences&) = delete;

    QFont         _font;
    int           _fontZoom{ 100 };
    bool          _checkForUpdates{ true };
    QColor        _backgroundColor{ Qt::white };
    QColor        _foregroundColor{ Qt::black };
    QColor        _statusColor{ Qt::red };
    QString       _language{ translationLang() };
    DataViewDefinitions _dataViewDefinitions;
    TrafficViewDefinitions _trafficViewDefinitions;
    ScriptViewDefinitions _scriptViewDefinitions;
    QFont         _scriptFont;
    bool          _codeAutoComplete{ true };
};

#endif // APPPREFERENCES_H
