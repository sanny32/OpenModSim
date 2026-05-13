// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file application.h
/// \brief Declares the application wrapper that owns app-level services.
///

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include "styles/apptheme.h"

class Application final
    : public QApplication
{
public:
    explicit Application(int& argc, char** argv);

    AppTheme& theme();
    const AppTheme& theme() const;

    static Application* instance();

private:
    AppTheme _theme;
};

inline Application* theApp()
{
    return static_cast<Application*>(qApp);
}

#endif // APPLICATION_H
