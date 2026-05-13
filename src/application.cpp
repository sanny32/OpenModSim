// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file application.cpp
/// \brief Implements the application wrapper and theme accessors.
///

#include "application.h"

///
/// \brief Application::Application
/// \param argc Application argument count.
/// \param argv Application argument values.
///
Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
    , _theme(this)
{
}

///
/// \brief Application::theme
/// \return Mutable reference to the shared app theme manager.
///
AppTheme& Application::theme()
{
    return _theme;
}

///
/// \brief Application::theme
/// \return Const reference to the shared app theme manager.
///
const AppTheme& Application::theme() const
{
    return _theme;
}

///
/// \brief Application::instance
/// \return Pointer to the process-wide Application instance.
///
Application* Application::instance()
{
    return static_cast<Application*>(QApplication::instance());
}
