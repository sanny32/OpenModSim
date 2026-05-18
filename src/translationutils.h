// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file translationutils.h
/// \brief Declares the translationutils interfaces.
///

#ifndef TRANSLATIONUTILS_H
#define TRANSLATIONUTILS_H

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QStringList>
#include <QTranslator>

///
/// \brief availableTranslations
/// \return
///
inline QStringList availableTranslations()
{
    QStringList locales;
    const QStringList files = QDir(":/res/translations").entryList(QStringList() << "*.qm", QDir::Files);

    for (auto file : files) {
        locales << file.remove("omodsim_").remove(".qm");
    }

    return locales;
}

///
/// \brief translationLang
/// \return
///
inline QString translationLang()
{
    const QStringList locales = availableTranslations();
    const QString sysLocale = QLocale::system().name();

    for (const QString &pattern : locales) {
        const QString regexPattern = QString("%1.*").arg(pattern);
        QRegularExpression re("^" + regexPattern + "$", QRegularExpression::CaseInsensitiveOption);

        if(re.match(sysLocale).hasMatch()) {
            return pattern;
        }
    }

    return "en";
}

///
/// \brief applyTranslation
/// \param lang
/// \param appTranslator
/// \param qtTranslator
///
inline void applyTranslation(const QString& lang, QTranslator& appTranslator, QTranslator& qtTranslator)
{
    qApp->removeTranslator(&appTranslator);
    qApp->removeTranslator(&qtTranslator);

    const QString resolved = (lang == "system") ? translationLang() : lang;
    if (resolved == "en")
        return;

    if (!appTranslator.load(QString(":/res/translations/omodsim_%1").arg(resolved)))
        return;

    qApp->installTranslator(&appTranslator);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
    const QString qtTranslationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif

    const QStringList qtTranslationPaths = {
        qApp->applicationDirPath() + "/translations",
        qApp->applicationDirPath() + "/../translations",
        qtTranslationsPath
    };
    for (const QString& path : qtTranslationPaths) {
        if (qtTranslator.load(QString("qt_%1").arg(resolved), path)) {
            qApp->installTranslator(&qtTranslator);
            break;
        }
    }
}

#endif // TRANSLATIONUTILS_H

