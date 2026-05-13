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
    static QIcon icon(const QString& name, const QString& fallbackPath = {});
};

inline QIcon themedIcon(const QString& name, const QString& fallbackPath = {})
{
    return ThemedIcons::icon(name, fallbackPath);
}

#endif // THEMEDICONS_H
