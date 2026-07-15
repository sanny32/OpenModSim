// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file recentprojectsprompt.cpp
/// \brief Implements confirmation prompts for recent projects.
///

#include "recentprojectsprompt.h"

#include <QMessageBox>

namespace RecentProjectsPrompt
{

///
/// \brief Asks whether the recent-projects list should be cleared.
/// \param parent Parent widget for the modal prompt.
/// \param title Prompt window title.
/// \param text Prompt message.
/// \return True only when the user confirms the operation.
///
bool confirmClear(QWidget* parent, const QString& title, const QString& text)
{
    const auto answer = QMessageBox::question(parent,
                                              title,
                                              text,
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::No);
    return answer == QMessageBox::Yes;
}

}
