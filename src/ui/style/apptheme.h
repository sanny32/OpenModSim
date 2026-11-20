// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file apptheme.h
/// \brief Declares the apptheme interfaces.
///

#ifndef APPTHEME_H
#define APPTHEME_H

#include <QObject>
#include "apppreferences.h"

class AppTheme final
    : public QObject
{
    Q_OBJECT

public:
    explicit AppTheme(QObject* parent = nullptr);
    bool isDark() const;
    bool supportsTheme() const;

    AppTheme(const AppTheme&) = delete;
    AppTheme& operator=(const AppTheme&) = delete;
    AppTheme(AppTheme&&) = delete;
    AppTheme& operator=(AppTheme&&) = delete;

signals:
    void colorSchemeChanged();

private:
    bool isSystemDark() const;
    void notifyColorSchemeChanged();
};

#endif // APPTHEME_H
