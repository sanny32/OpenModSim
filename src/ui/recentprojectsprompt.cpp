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
    return confirmClear(parent, title, text,
                        [](QWidget* promptParent, const QString& promptTitle,
                           const QString& promptText, QMessageBox::StandardButtons buttons,
                           QMessageBox::StandardButton defaultButton) {
        return QMessageBox::question(promptParent, promptTitle, promptText, buttons, defaultButton);
    });
}

///
/// \brief Asks whether the recent-projects list should be cleared using the supplied handler.
/// \param parent Parent widget for the modal prompt.
/// \param title Prompt window title.
/// \param text Prompt message.
/// \param question Handler used to ask the confirmation question.
/// \return True only when the handler returns Yes.
///
bool confirmClear(QWidget* parent, const QString& title, const QString& text,
                  const QuestionHandler& question)
{
    const auto answer = question(parent,
                                 title,
                                 text,
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No);
    return answer == QMessageBox::Yes;
}

}
