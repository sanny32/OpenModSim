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
    static QIcon icon(const QString& name);
};

inline QIcon themedIcon(const QString& name)
{
    return ThemedIcons::icon(name);
}

#endif // THEMEDICONS_H
