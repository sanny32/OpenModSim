// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file helpbrowser.cpp
/// \brief Implements the HelpBrowser class.
///

#include <QEvent>
#include <QUrl>
#include "application.h"
#include "helpbrowser.h"

///
/// \brief HelpBrowser::HelpBrowser
/// \param parent
///
HelpBrowser::HelpBrowser(QWidget* parent)
    : QTextBrowser(parent)
{
    connect(&theApp()->theme(), &AppTheme::colorSchemeChanged, this, &HelpBrowser::reload);
}

///
/// \brief HelpBrowser::setHelp
/// \param helpFile
///
void HelpBrowser::setHelp(const QString& helpFile)
{
    _helpEngine = QSharedPointer<QHelpEngine>(new QHelpEngine(helpFile, this));
    _helpEngine->setupData();

    setSource(QUrl(tr("qthelp://omodsim/doc/index.html")));
    reload();
}

///
/// \brief HelpBrowser::loadResource
/// \param type
/// \param name
/// \return
///
QVariant HelpBrowser::loadResource(int type, const QUrl& name)
{
    if (name.scheme() == QLatin1String("qthelp") && _helpEngine) {
        if (name.path().endsWith(QLatin1String(".css")))
            return QVariant(QByteArray()); // CSS applied via setDefaultStyleSheet

        return QVariant(_helpEngine->fileData(name));
    }
    return QTextBrowser::loadResource(type, name);
}

///
/// \brief HelpBrowser::showHelp
/// \param helpKey
///
void HelpBrowser::showHelp(const QString& helpKey)
{
    const auto url = QString(tr("qthelp://omodsim/doc/index.html#%1")).arg(helpKey.toLower());
    setSource(QUrl(url));
}

///
/// \brief HelpBrowser::reload
///
void HelpBrowser::reload()
{
    applyStylesheet();
    QTextBrowser::reload();
}

///
/// \brief HelpBrowser::changeEvent
/// \param event
///
void HelpBrowser::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        setSource(QUrl(tr("qthelp://omodsim/doc/index.html")));

    QTextBrowser::changeEvent(event);
}

///
/// \brief HelpBrowser::applyStylesheet
///
void HelpBrowser::applyStylesheet()
{
    if (!_helpEngine) return;

    const auto fname = [](const QUrl& u) { return u.path().section(QLatin1Char('/'), -1); };
    auto urls = _helpEngine->files(QStringLiteral("omodsim"), QStringLiteral(""), QStringLiteral("css"));

    QStringList css;
    for (const auto& url : std::as_const(urls)) {
        if (fname(url) == QLatin1String("2.dark.css") && !theApp()->theme().isDark())
            continue;

        css << QString::fromUtf8(_helpEngine->fileData(url));
    }

    document()->setDefaultStyleSheet(css.join(QStringLiteral("\n")));
}
