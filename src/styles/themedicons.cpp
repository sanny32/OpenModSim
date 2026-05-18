// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file themedicons.cpp
/// \brief Implements the themedicons functionality.
///

#include "themedicons.h"

#include <QApplication>
#include <QFile>
#include <QHash>

#if defined(HAVE_QLEMENTINE_APP_STYLE)
#include <oclero/qlementine/icons/QlementineIcons.hpp>
#include "styles/qlementineappstyle.h"
#endif

namespace {
struct IconDescriptor {
    QString themeName;
    QString resourceName; // base name without extension, e.g. "save", "bell" — SVG preferred over PNG
};

///
/// \brief Returns the built-in mapping of icon aliases to theme and fallback resources.
/// \return Constant icon descriptor registry.
///
const QHash<QString, IconDescriptor>& iconRegistry()
{
    static const QHash<QString, IconDescriptor> registry {
        { QStringLiteral("omodsim/about"),                     { QStringLiteral("misc/question"),                       QStringLiteral("about") } },
        { QStringLiteral("omodsim/add"),                       { QStringLiteral("action/plus-circle"),                  QStringLiteral("add") } },
        { QStringLiteral("omodsim/bell"),                      { QStringLiteral("misc/bell"),                           QStringLiteral("bell") } },
        { QStringLiteral("omodsim/bell-dot"),                  { QStringLiteral("misc/bell"),                           QStringLiteral("bell-dot") } },
        { QStringLiteral("omodsim/clear"),                     { QStringLiteral("action/trash"),                        QStringLiteral("edit-delete.png") } },
        { QStringLiteral("omodsim/close"),                     { QStringLiteral("action/close"),                        QStringLiteral("close") } },
        { QStringLiteral("omodsim/connect"),                   { QStringLiteral("misc/link"),                           QStringLiteral("connect") } },
        { QStringLiteral("omodsim/copy"),                      { QStringLiteral("action/copy"),                         QStringLiteral("copy") } },
        { QStringLiteral("omodsim/cut"),                       { QStringLiteral("action/cut"),                          QStringLiteral("cut") } },
        { QStringLiteral("omodsim/data-locked"),               { QStringLiteral("action/lock"),                         QStringLiteral("data-locked") } },
        { QStringLiteral("omodsim/directory"),                 { QStringLiteral("file/folder-filled"),                  QStringLiteral("directory") } },
        { QStringLiteral("omodsim/disconnect"),                { QStringLiteral("misc/link-break"),                     QStringLiteral("disconnect") } },
        { QStringLiteral("omodsim/export"),                    { QStringLiteral("action/export"),                       QStringLiteral("export") } },
        { QStringLiteral("omodsim/force-coils"),               { QStringLiteral("omodsim/force-coils"),                 QStringLiteral("force-coils") } },
        { QStringLiteral("omodsim/force-discretes"),           { QStringLiteral("omodsim/force-discretes"),             QStringLiteral("force-discretes") } },
        { QStringLiteral("omodsim/github"),                    { QStringLiteral("brand/github-fill"),                   QStringLiteral("emblem-github") } },
        { QStringLiteral("omodsim/hex-view"),                  { QStringLiteral("omodsim/hex-view"),                    QStringLiteral("hex-view") } },
        { QStringLiteral("omodsim/information"),               { QStringLiteral("misc/info"),                           QStringLiteral("log-info") } },
        { QStringLiteral("omodsim/internet"),                  { QStringLiteral("misc/globe"),                          QStringLiteral("applications-internet") } },
        { QStringLiteral("omodsim/import"),                    { QStringLiteral("omodsim/import"),                      QStringLiteral("import") } },
        { QStringLiteral("omodsim/mail"),                      { QStringLiteral("misc/mail"),                           QStringLiteral("emblem-mail") } },
        { QStringLiteral("omodsim/message-parser"),            { QStringLiteral("action/preview"),                      QStringLiteral("msg-parser") } },
        { QStringLiteral("omodsim/modbus-definitions"),        { QStringLiteral("navigation/settings"),                 QStringLiteral("mb-definitions") } },
        { QStringLiteral("omodsim/new-data"),                  { QStringLiteral("misc/glasses"),                        QStringLiteral("new-data") } },
        { QStringLiteral("omodsim/new-data-map"),              { QStringLiteral("navigation/map"),                      QStringLiteral("new-data") } },
        { QStringLiteral("omodsim/new-script"),                { QStringLiteral("file/file-script"),                    QStringLiteral("new-script") } },
        { QStringLiteral("omodsim/new-traffic"),               { QStringLiteral("misc/sms"),                            QStringLiteral("new-traffic") } },
        { QStringLiteral("omodsim/open-project"),              { QStringLiteral("file/folder-open"),                    QStringLiteral("open") } },
        { QStringLiteral("omodsim/paste"),                     { QStringLiteral("action/paste"),                        QStringLiteral("paste") } },
        { QStringLiteral("omodsim/pause"),                     { QStringLiteral("media/pause"),                         QStringLiteral("pause") } },
        { QStringLiteral("omodsim/preferences-defaults"),      { QStringLiteral("document/page-setup"),                 QStringLiteral("define-data") } },
        { QStringLiteral("omodsim/preferences-interface"),     { QStringLiteral("document/template"),                   QStringLiteral("pref-interface") } },
        { QStringLiteral("omodsim/preferences-script"),        { QStringLiteral("file/markup"),                         QStringLiteral("show-script") } },
        { QStringLiteral("omodsim/preset-holding-registers"),  { QStringLiteral("omodsim/preset-holding-regs"),         QStringLiteral("preset-holding-regs") } },
        { QStringLiteral("omodsim/preset-input-registers"),    { QStringLiteral("omodsim/preset-input-regs"),           QStringLiteral("preset-input-regs") } },
        { QStringLiteral("omodsim/print"),                     { QStringLiteral("action/print"),                        QStringLiteral("print") } },
        { QStringLiteral("omodsim/redo"),                      { QStringLiteral("action/redo"),                         QStringLiteral("redo") } },
        { QStringLiteral("omodsim/remove"),                    { QStringLiteral("action/erase"),                        QStringLiteral("remove") } },
        { QStringLiteral("omodsim/run-script"),                { QStringLiteral("media/play-small"),                    QStringLiteral("run-script") } },
        { QStringLiteral("omodsim/save-project"),              { QStringLiteral("action/save-to-disk"),                 QStringLiteral("save") } },
        { QStringLiteral("omodsim/show-data"),                 { QStringLiteral("misc/glasses"),                        QStringLiteral("show-data") } },
        { QStringLiteral("omodsim/show-data-map"),             { QStringLiteral("navigation/map"),                      QStringLiteral("show-data") } },
        { QStringLiteral("omodsim/show-script"),               { QStringLiteral("file/file-script"),                    QStringLiteral("show-script") } },
        { QStringLiteral("omodsim/show-traffic"),              { QStringLiteral("misc/sms"),                            QStringLiteral("show-traffic") } },
        { QStringLiteral("omodsim/split-view"),                { QStringLiteral("action/separate-horizontal"),          QStringLiteral("split-view") } },
        { QStringLiteral("omodsim/stop-script"),               { QStringLiteral("media/stop-small"),                    QStringLiteral("stop-script") } },
        { QStringLiteral("omodsim/undo"),                      { QStringLiteral("action/undo"),                         QStringLiteral("undo") } },
        { QStringLiteral("omodsim/warning"),                   { QStringLiteral("misc/warning"),                        QStringLiteral("log-warning") } },
        { QStringLiteral("omodsim/find-next"),                 { QStringLiteral("navigation/arrow-right"),              QStringLiteral("arrow-right") } },
        { QStringLiteral("omodsim/find-previous"),             { QStringLiteral("navigation/arrow-left"),               QStringLiteral("arrow-left") } },
        { QStringLiteral("omodsim/match-case"),                { QStringLiteral("text/match-case"),                     QStringLiteral("match-case") } },
        { QStringLiteral("omodsim/match-whole-word"),          { QStringLiteral("text/match-whole-word"),               QStringLiteral("whole-word") } },
        { QStringLiteral("omodsim/replace"),                   { QStringLiteral("action/replace"),                      QStringLiteral("replace") } },
        { QStringLiteral("omodsim/replace-all"),               { QStringLiteral("omodsim/replace-all"),                 QStringLiteral("replace-all") } },
        { QStringLiteral("omodsim/error"),                     { QStringLiteral("action/clear"),                        QStringLiteral("log-critical") } },
        { QStringLiteral("omodsim/hex"),                       { QStringLiteral(""),                                    QStringLiteral("hex") } },
        { QStringLiteral("omodsim/js-prop"),                   { QStringLiteral(""),                                    QStringLiteral("prop") } },
        { QStringLiteral("omodsim/js-func"),                   { QStringLiteral(""),                                    QStringLiteral("func") } },
        { QStringLiteral("omodsim/js-enum"),                   { QStringLiteral(""),                                    QStringLiteral("enum") } },
        { QStringLiteral("omodsim/landscape"),                 { QStringLiteral(""),                                    QStringLiteral("landscape") } },
        { QStringLiteral("omodsim/portrait"),                  { QStringLiteral(""),                                    QStringLiteral("portrait") } },
        { QStringLiteral("omodsim/simulation-16bit"),          { QStringLiteral(""),                                    QStringLiteral("simulation-16bit") } },
        { QStringLiteral("omodsim/simulation-32bit"),          { QStringLiteral(""),                                    QStringLiteral("simulation-32bit") } },
        { QStringLiteral("omodsim/simulation-64bit"),          { QStringLiteral(""),                                    QStringLiteral("simulation-64bit") } }
    };

    return registry;
}

///
/// \brief Resolves an icon from the current desktop icon theme and optional Qlementine mapping.
/// \param themeName Theme icon identifier to resolve.
/// \return Resolved icon or a null icon when no themed match is found.
///
QIcon iconFromThemeName(const QString& themeName)
{
#if defined(HAVE_QLEMENTINE_APP_STYLE)
    // Prefer qlementine's freedesktop mapping first so macOS native icon libraries
    // do not override expected warning/info/error glyphs.
    if (!themeName.contains(QLatin1Char('/'))) {
        const QString mappedName = oclero::qlementine::icons::fromFreeDesktop(themeName);
        if (!mappedName.isEmpty() && QFile::exists(mappedName)) {
            const QIcon mappedIcon(mappedName);
            if (!mappedIcon.isNull())
                return mappedIcon;
        }
    }

    if (themeName.contains(QLatin1Char('/'))) {
        const QString qlementineIconPath = QStringLiteral(":/qlementine/icons/16/%1.svg").arg(themeName);
        if (QFile::exists(qlementineIconPath))
            return QIcon(qlementineIconPath);

        const QString customIconPath = QStringLiteral(":/res/icons/16/%1.svg").arg(themeName);
        if (QFile::exists(customIconPath))
            return QIcon(customIconPath);
    }
#endif

    QIcon icon = QIcon::fromTheme(themeName);
    return icon;
}
}

///
/// \brief Returns an icon by OpenModSim name with theme-aware and resource fallback resolution.
/// \param name Icon name or alias key.
/// \return Resolved icon, or a null icon if no source can be resolved.
///
QIcon ThemedIcons::icon(const QString& name, IconMode mode)
{
    static const QString kFallbackDir = QStringLiteral(":/res/icons/fallback/");

    const auto& registry = iconRegistry();
    const auto it = registry.constFind(name);
    const QString themeName = it == registry.cend() ? name : it->themeName;
    const QString resourceName = it == registry.cend() ? QString{} : it->resourceName;

#if defined(HAVE_QLEMENTINE_APP_STYLE)
    if (mode == Auto && dynamic_cast<QlementineAppStyle*>(qApp ? qApp->style() : nullptr)) {
        QIcon icon = iconFromThemeName(themeName);
        if (!icon.isNull())
            return icon;
    }
#endif

    if (resourceName.isEmpty())
        return {};

    const auto iconPath = kFallbackDir + resourceName;
    if (QFile::exists(iconPath))
        return QIcon(iconPath);

    const QString svgPath = iconPath + QStringLiteral(".svg");
    if (QFile::exists(svgPath))
        return QIcon(svgPath);

    return QIcon(iconPath + QStringLiteral(".png"));
}
