#ifndef THEMEDICONS_H
#define THEMEDICONS_H

#include <QIcon>
#include <QString>

#if defined(Q_OS_MAC) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <oclero/qlementine/icons/QlementineIcons.hpp>
#endif

///
/// \brief Creates an icon from the current icon theme with an optional fallback.
/// \param name
/// \param fallbackPath
/// \return
///
inline QIcon themedIcon(const QString& name, const QString& fallbackPath = {})
{
    QIcon icon = QIcon::fromTheme(name);

#if defined(Q_OS_MAC) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (icon.isNull() && !name.contains(QLatin1Char('/'))) {
        const QString mappedName = oclero::qlementine::icons::fromFreeDesktop(name);
        if (!mappedName.isEmpty() && mappedName != name)
            icon = QIcon::fromTheme(mappedName);
    }
#endif

    if (icon.isNull() && !fallbackPath.isEmpty())
        icon = QIcon(fallbackPath);

    return icon;
}

#endif // THEMEDICONS_H
