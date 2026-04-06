#ifndef TRANSLATIONUTILS_H
#define TRANSLATIONUTILS_H

#include <QDir>
#include <QLocale>
#include <QRegularExpression>
#include <QStringList>

inline QStringList availableTranslations()
{
    QStringList locales;
    const QStringList files = QDir(":/translations").entryList(QStringList() << "*.qm", QDir::Files);

    for (auto file : files) {
        locales << file.remove("omodsim_").remove(".qm");
    }

    return locales;
}

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

#endif // TRANSLATIONUTILS_H

