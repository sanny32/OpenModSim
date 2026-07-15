// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file recentprojectsprompt.h
/// \brief Declares confirmation prompts for recent projects.
///

#ifndef RECENTPROJECTSPROMPT_H
#define RECENTPROJECTSPROMPT_H

#include <functional>
#include <QMessageBox>
#include <QString>

class QWidget;

namespace RecentProjectsPrompt
{

using QuestionHandler = std::function<QMessageBox::StandardButton(
    QWidget*, const QString&, const QString&, QMessageBox::StandardButtons,
    QMessageBox::StandardButton)>;

///
/// \brief Asks whether the recent-projects list should be cleared.
/// \param parent Parent widget for the modal prompt.
/// \param title Prompt window title.
/// \param text Prompt message.
/// \return True only when the user confirms the operation.
///
bool confirmClear(QWidget* parent, const QString& title, const QString& text);

///
/// \brief Asks whether the recent-projects list should be cleared using the supplied handler.
/// \param parent Parent widget for the modal prompt.
/// \param title Prompt window title.
/// \param text Prompt message.
/// \param question Handler used to ask the confirmation question.
/// \return True only when the handler returns Yes.
///
bool confirmClear(QWidget* parent, const QString& title, const QString& text,
                  const QuestionHandler& question);

}

#endif // RECENTPROJECTSPROMPT_H
