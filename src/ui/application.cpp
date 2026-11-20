// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file application.cpp
/// \brief Implements the application functionality.
///

#include <QFileOpenEvent>
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

///
/// \brief Application::takePendingFile
/// \return File path received via QFileOpenEvent before any receiver connected,
///         or an empty string if none is pending.
///
QString Application::takePendingFile()
{
    return std::exchange(_pendingFile, {});
}

///
/// \brief Application::event
/// \param event
/// \return
///
bool Application::event(QEvent* event)
{
    if (event->type() == QEvent::FileOpen) {
        const QString filePath = static_cast<QFileOpenEvent*>(event)->file();
        _pendingFile = filePath;
        emit fileOpenRequested(filePath);
        return true;
    }
    return QApplication::event(event);
}
