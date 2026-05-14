// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file apptheme.cpp
/// \brief Implements the apptheme functionality.
///

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#endif
#include "apptheme.h"

///
/// \brief AppTheme::AppTheme
/// \param parent Owning QObject.
///
AppTheme::AppTheme(QObject* parent)
    : QObject(parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (auto* hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged,
                this, [this]() { notifyColorSchemeChanged(); });
    }

    connect(&AppPreferences::instance(), &AppPreferences::globalSettingChanged,
            this, [this](const QString& name, const QString&, const QString&) {
                if (name == QLatin1String("ThemeMode"))
                    notifyColorSchemeChanged();
            });
#endif
}

///
/// \brief AppTheme::supportsTheme
/// \return True when the active application style supports theme switching.
///
bool AppTheme::supportsTheme() const
{
#if defined(HAVE_QLEMENTINE_APP_STYLE)
    return true;
#else
    return false;
#endif
}

///
/// \brief AppTheme::isDark
/// \return True when active theme should be treated as dark.
///
bool AppTheme::isDark() const
{
    switch (AppPreferences::instance().themeMode()) {
        case AppThemeMode::Light:
            return false;
        case AppThemeMode::Dark:
            return true;
        case AppThemeMode::System:
        default:
            return isSystemDark();
    }
}

///
/// \brief AppTheme::isSystemDark
/// \return True when platform/system color scheme is dark.
///
bool AppTheme::isSystemDark() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (const auto* hints = QGuiApplication::styleHints())
        return hints->colorScheme() == Qt::ColorScheme::Dark;
#endif
    return false;
}

///
/// \brief AppTheme::notifyColorSchemeChanged
///
void AppTheme::notifyColorSchemeChanged()
{
    emit colorSchemeChanged();
}
