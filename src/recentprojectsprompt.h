// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file recentprojectsprompt.h
/// \brief Declares confirmation prompts for recent projects.
///

#ifndef RECENTPROJECTSPROMPT_H
#define RECENTPROJECTSPROMPT_H

#include <QString>

class QWidget;

namespace RecentProjectsPrompt
{

///
/// \brief Asks whether the recent-projects list should be cleared.
/// \param parent Parent widget for the modal prompt.
/// \param title Prompt window title.
/// \param text Prompt message.
/// \return True only when the user confirms the operation.
///
bool confirmClear(QWidget* parent, const QString& title, const QString& text);

}

#endif // RECENTPROJECTSPROMPT_H
