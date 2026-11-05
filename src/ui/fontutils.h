// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file fontutils.h
/// \brief Declares the fontutils interfaces.
///

#ifndef FONTUTILS_H
#define FONTUTILS_H

#include <QFont>
#include <QList>
#include <QApplication>
#include <QFontDatabase>

inline int defaultMonospaceFontSize()
{
#ifdef Q_OS_MAC
    return 13;
#else
    return 10;
#endif
}

inline QFont defaultScriptFont(int pointSize = -1)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto families = QFontDatabase::families();
#else
    const auto families = QFontDatabase().families();
#endif
    const QString family = families.contains("Fira Code") ? "Fira Code"
                         : families.contains("Consolas")  ? "Consolas"
                         : "Courier New";
    QFont font(family, pointSize > 0 ? pointSize : defaultMonospaceFontSize());
    font.setStyleHint(QFont::Monospace);
    return font;
}

inline QFont defaultMonospaceFont(int pointSize = -1)
{
    QString family;

#if defined(Q_OS_WIN)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    auto families = QFontDatabase::families();
#else
    auto families = QFontDatabase().families();
#endif
    if (families.contains("Consolas"))
        family = "Consolas";
    else
        family = "Courier New";

#elif defined(Q_OS_MAC)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    auto families = QFontDatabase::families();
#else
    auto families = QFontDatabase().families();
#endif
    if (families.contains("Menlo"))
        family = "Menlo";
    else
        family = "Monaco";

#else
    family = "Monospace";
#endif

    QFont font(family, pointSize > 0 ? pointSize : defaultMonospaceFontSize());
    font.setStyleHint(QFont::Monospace);
    return font;
}

#endif // FONTUTILS_H

