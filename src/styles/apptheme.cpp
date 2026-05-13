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
#else
    if (qApp) {
        connect(qApp, &QGuiApplication::paletteChanged,
                this, [this]() { notifyColorSchemeChanged(); });
    }
#endif

    connect(&AppPreferences::instance(), &AppPreferences::settingChanged,
            this, [this](const QString& name, const QString&, const QString&) {
                if (name == QLatin1String("ThemeMode"))
                    notifyColorSchemeChanged();
            });
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
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
}

///
/// \brief AppTheme::notifyColorSchemeChanged
///
void AppTheme::notifyColorSchemeChanged()
{
    emit colorSchemeChanged();
}
