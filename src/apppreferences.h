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
    void setFont(const QFont& f);

    int fontZoom() const { return _fontZoom; }
    void setFontZoom(int zoom);

    QColor backgroundColor() const { return _backgroundColor; }
    void setBackgroundColor(const QColor& c);

    QColor foregroundColor() const { return _foregroundColor; }
    void setForegroundColor(const QColor& c);

    QColor addressColor() const { return _addressColor; }
    void setAddressColor(const QColor& c);

    QColor commentColor() const { return _commentColor; }
    void setCommentColor(const QColor& c);

    // ----- Updates -----
    bool checkForUpdates() const { return _checkForUpdates; }
    void setCheckForUpdates(bool enable);

    // ----- Language -----
    QString language() const { return _language; }
    void setLanguage(const QString& lang);

    // ----- Default View Definitions -----
    DataViewDefinitions dataViewDefinitions() const { return _dataViewDefinitions; }
    void setDataViewDefinitions(const DataViewDefinitions& def);

    TrafficViewDefinitions trafficViewDefinitions() const { return _trafficViewDefinitions; }
    void setTrafficViewDefinitions(const TrafficViewDefinitions& def);

    ScriptViewDefinitions scriptViewDefinitions() const { return _scriptViewDefinitions; }
    void setScriptViewDefinitions(const ScriptViewDefinitions& def);

    bool globalZeroBasedAddress() const { return _globalZeroBasedAddress; }
    void setGlobalZeroBasedAddress(bool value);

    bool globalHexView() const { return _globalHexView; }
    void setGlobalHexView(bool value);

    // ----- Script Editor -----
    QFont scriptFont() const { return _scriptFont; }
    void setScriptFont(const QFont& f);

    bool codeAutoComplete() const { return _codeAutoComplete; }
    void setCodeAutoComplete(bool enable);

    bool autoShowConsoleOutput() const { return _autoShowConsoleOutput; }
    void setAutoShowConsoleOutput(bool enable);

    int consoleMaxLines() const { return _consoleMaxLines; }
    void setConsoleMaxLines(int n);

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
    QColor        _addressColor{ 128, 128, 128 };
    QColor        _commentColor{ 128, 128, 128 };
    QString       _language{ translationLang() };
    DataViewDefinitions _dataViewDefinitions;
    TrafficViewDefinitions _trafficViewDefinitions;
    ScriptViewDefinitions _scriptViewDefinitions;
    bool          _globalZeroBasedAddress{ false };
    bool          _globalHexView{ false };
    QFont         _scriptFont;
    bool          _codeAutoComplete{ true };
    bool          _autoShowConsoleOutput{ true };
    int           _consoleMaxLines{ 500 };
};

#endif // APPPREFERENCES_H

