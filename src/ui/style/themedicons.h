// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file themedicons.h
/// \brief Declares the themedicons interfaces.
///

#ifndef THEMEDICONS_H
#define THEMEDICONS_H

#include <QIcon>
#include <QString>

class ThemedIcons
{
public:
    enum IconMode {
        Auto,     ///< theme-aware: theme first, fallback resource if not found
        Fallback  ///< fallback resource only, bypasses theme lookup
    };

    static QIcon icon(const QString& name, IconMode mode = Auto);
};

inline QIcon themedIcon(const QString& name, ThemedIcons::IconMode mode = ThemedIcons::Auto)
{
    return ThemedIcons::icon(name, mode);
}

#endif // THEMEDICONS_H
