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
    QString resourcePath;
};

const QHash<QString, IconDescriptor>& iconRegistry()
{
    static const QHash<QString, IconDescriptor> registry {
        { QStringLiteral("omodsim/about"),                     { QStringLiteral("help-about"),                          QStringLiteral(":/res/icon-about.png") } },
        { QStringLiteral("omodsim/add"),                       { QStringLiteral("action/plus-circle"),                  QStringLiteral(":/res/icon-add.png") } },
        { QStringLiteral("omodsim/bell"),                      { QStringLiteral("misc/bell"),                           QStringLiteral(":/res/icon-bell.png") } },
        { QStringLiteral("omodsim/bell-dot"),                  { QStringLiteral("misc/bell"),                           QStringLiteral(":/res/icon-bell-dot.png") } },
        { QStringLiteral("omodsim/clear"),                     { QStringLiteral("action/trash"),                        QStringLiteral(":/res/edit-delete.png") } },
        { QStringLiteral("omodsim/connect"),                   { QStringLiteral("misc/link"),                           QStringLiteral(":/res/icon-connect.png") } },
        { QStringLiteral("omodsim/copy"),                      { QStringLiteral("edit-copy"),                           QStringLiteral(":/res/icon-copy.png") } },
        { QStringLiteral("omodsim/cut"),                       { QStringLiteral("edit-cut"),                            QStringLiteral(":/res/icon-cut.png") } },
        { QStringLiteral("omodsim/data-locked"),               { QStringLiteral("action/lock"),                         QStringLiteral(":/res/icon-data-locked.png") } },
        { QStringLiteral("omodsim/directory"),                 { QStringLiteral("file/folder-filled"),                  QStringLiteral(":/res/icon-directory.png") } },
        { QStringLiteral("omodsim/disconnect"),                { QStringLiteral("misc/link-break"),                     QStringLiteral(":/res/icon-disconnect.png") } },
        { QStringLiteral("omodsim/export"),                    { QStringLiteral("action/export"),                       QStringLiteral(":/res/icon-export.png") } },
        { QStringLiteral("omodsim/force-coils"),               { QStringLiteral(":/res/icon-force-coils.svg"),          QStringLiteral(":/res/icon-force-coils.png") } },
        { QStringLiteral("omodsim/force-discretes"),           { QStringLiteral(":/res/icon-force-discretes.svg"),      QStringLiteral(":/res/icon-force-discretes.png") } },
        { QStringLiteral("omodsim/github"),                    { QStringLiteral("brand/github-fill"),                   QStringLiteral(":/res/emblem-github.png") } },
        { QStringLiteral("omodsim/hex-view"),                  { QStringLiteral(":/res/icon-hex.svg"),                  QStringLiteral(":/res/icon-hex-view.png") } },
        { QStringLiteral("omodsim/information"),               { QStringLiteral("dialog-information"),                  QStringLiteral(":/res/icon-log-info.png") } },
        { QStringLiteral("omodsim/internet"),                  { QStringLiteral("applications-internet"),               QStringLiteral(":/res/applications-internet.png") } },
        { QStringLiteral("omodsim/import"),                    { QStringLiteral(":/res/action-import.svg"),             QStringLiteral(":/res/icon-import.png") } },
        { QStringLiteral("omodsim/mail"),                      { QStringLiteral("emblem-mail"),                         QStringLiteral(":/res/emblem-mail.png") } },
        { QStringLiteral("omodsim/message-parser"),            { QStringLiteral("action/preview"),                      QStringLiteral(":/res/icon-msg-parser.png") } },
        { QStringLiteral("omodsim/modbus-definitions"),        { QStringLiteral("navigation/settings"),                 QStringLiteral(":/res/icon-mb-definitions.png") } },
        { QStringLiteral("omodsim/new-data"),                  { QStringLiteral("misc/glasses"),                        QStringLiteral(":/res/icon-new-data.png") } },
        { QStringLiteral("omodsim/new-data-map"),              { QStringLiteral("navigation/map"),                      QStringLiteral(":/res/icon-new-data.png") } },
        { QStringLiteral("omodsim/new-script"),                { QStringLiteral("text-x-script"),                       QStringLiteral(":/res/icon-new-script.png") } },
        { QStringLiteral("omodsim/new-traffic"),               { QStringLiteral("misc/sms"),                            QStringLiteral(":/res/icon-new-traffic.png") } },
        { QStringLiteral("omodsim/open-project"),              { QStringLiteral("file/folder-open"),                    QStringLiteral(":/res/icon-open.png") } },
        { QStringLiteral("omodsim/paste"),                     { QStringLiteral("edit-paste"),                          QStringLiteral(":/res/icon-paste.png") } },
        { QStringLiteral("omodsim/preferences-defaults"),      { QStringLiteral("document/page-setup"),                 QStringLiteral(":/res/icon-define-data.png") } },
        { QStringLiteral("omodsim/preferences-interface"),     { QStringLiteral("document/template"),                   QStringLiteral(":/res/pref-interface.png") } },
        { QStringLiteral("omodsim/preferences-script"),        { QStringLiteral("file/markup"),                         QStringLiteral(":/res/icon-show-script.png") } },
        { QStringLiteral("omodsim/preset-holding-registers"),  { QStringLiteral(":/res/icon-preset-holding-regs.svg"),  QStringLiteral(":/res/icon-preset-holding-regs.png") } },
        { QStringLiteral("omodsim/preset-input-registers"),    { QStringLiteral(":/res/icon-preset-input-regs.svg"),    QStringLiteral(":/res/icon-preset-input-regs.png") } },
        { QStringLiteral("omodsim/print"),                     { QStringLiteral("action/print"),                        QStringLiteral(":/res/icon-print.png") } },
        { QStringLiteral("omodsim/redo"),                      { QStringLiteral("edit-redo"),                           QStringLiteral(":/res/icon-redo.png") } },
        { QStringLiteral("omodsim/remove"),                    { QStringLiteral("action/erase"),                        QStringLiteral(":/res/icon-remove.png") } },
        { QStringLiteral("omodsim/run-script"),                { QStringLiteral("media-playback-start"),                QStringLiteral(":/res/icon-run-script.png") } },
        { QStringLiteral("omodsim/save-project"),              { QStringLiteral("action/save-to-disk"),                 QStringLiteral(":/res/icon-save.png") } },
        { QStringLiteral("omodsim/show-data"),                 { QStringLiteral("misc/glasses"),                        QStringLiteral(":/res/icon-show-data.png") } },
        { QStringLiteral("omodsim/show-data-map"),             { QStringLiteral("navigation/map"),                      QStringLiteral(":/res/icon-show-data.png") } },
        { QStringLiteral("omodsim/show-script"),               { QStringLiteral("text-x-script"),                       QStringLiteral(":/res/icon-show-script.png") } },
        { QStringLiteral("omodsim/show-traffic"),              { QStringLiteral("misc/sms"),                            QStringLiteral(":/res/icon-show-traffic.png") } },
        { QStringLiteral("omodsim/split-view"),                { QStringLiteral("action/separate-horizontal"),          QStringLiteral(":/res/icon-split-view.png") } },
        { QStringLiteral("omodsim/stop-script"),               { QStringLiteral("media-playback-stop"),                 QStringLiteral(":/res/icon-stop-script.png") } },
        { QStringLiteral("omodsim/undo"),                      { QStringLiteral("edit-undo"),                           QStringLiteral(":/res/icon-undo.png") } },
        { QStringLiteral("omodsim/warning"),                   { QStringLiteral("dialog-warning"),                      QStringLiteral(":/res/icon-log-warning.png") } },
        { QStringLiteral("omodsim/error"),                     { QStringLiteral("dialog-error"),                        QStringLiteral(":/res/icon-log-critical.png") } }
    };

    return registry;
}

QIcon iconFromThemeName(const QString& themeName)
{
    QIcon icon = QIcon::fromTheme(themeName);

#if defined(HAVE_QLEMENTINE_APP_STYLE)
    if (icon.isNull() && themeName.contains(QLatin1Char('/'))) {
        const QString qlementineIconPath = QStringLiteral(":/qlementine/icons/16/%1.svg").arg(themeName);
        if (QFile::exists(qlementineIconPath))
            icon = QIcon(qlementineIconPath);
    }

    if (icon.isNull() && !themeName.contains(QLatin1Char('/'))) {
        const QString mappedName = oclero::qlementine::icons::fromFreeDesktop(themeName);
        if (!mappedName.isEmpty() && mappedName != themeName)
            icon = QIcon(mappedName);
    }
#endif

    return icon;
}
}

QIcon ThemedIcons::icon(const QString& name, const QString& fallbackPath)
{
    const auto& registry = iconRegistry();
    const auto it = registry.constFind(name);
    const QString themeName = it == registry.cend() ? name : it->themeName;
    const QString resolvedFallbackPath = it == registry.cend() ? fallbackPath : it->resourcePath;

#if defined(HAVE_QLEMENTINE_APP_STYLE)
    if (dynamic_cast<QlementineAppStyle*>(qApp ? qApp->style() : nullptr)) {
        QIcon icon = iconFromThemeName(themeName);
        if (!icon.isNull())
            return icon;
    }
#endif

    return !resolvedFallbackPath.isEmpty() ? QIcon(resolvedFallbackPath) : QIcon();
}
