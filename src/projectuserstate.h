// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectuserstate.h
/// \brief Declares the split of a project document into shared project data and
/// machine-local user state.
///

#ifndef PROJECTUSERSTATE_H
#define PROJECTUSERSTATE_H

#include <QByteArray>
#include <QString>

///
/// \brief The ProjectDocuments struct holds the two halves a saved project is written as.
///
struct ProjectDocuments
{
    QByteArray Project;  ///< Goes to the .omsim file, shared through version control.
    QByteArray User;     ///< Goes to the .omsim.user file, local to one machine.
};

///
/// \brief projectUserStatePath derives the user-state file path of a project: a hidden
/// file beside it, so the project directory stays readable. Note that a leading dot hides
/// the file on Unix only; on Windows it is an ordinary name.
/// \param projectPath Path of the .omsim file.
/// \return The path of the hidden .omsim.user file.
///
QString projectUserStatePath(const QString& projectPath);

///
/// \brief splitProjectUserState divides a generated project document into the shared
/// project data and the user state: window geometry, view mode, tab order, editor fonts,
/// colours and zoom, cursor and scroll positions, data map column widths and the leading
/// zeros of a data view. The column distance of a data view stays project data: it is part
/// of how a project presents its registers, not of how one machine happens to show them.
/// \param document The generated project XML.
/// \return Both halves; the user half is empty when there is no user state to store.
///
ProjectDocuments splitProjectUserState(const QByteArray& document);

///
/// \brief mergeProjectUserState folds a user-state document back into a project document,
/// producing the single document shape the project reader expects. Values already present
/// in the project document, as written by versions that stored everything in one file, are
/// overridden by the user state.
/// \param project The project document.
/// \param user The user-state document; may be empty.
/// \return The merged document, or the project document unchanged on error.
///
QByteArray mergeProjectUserState(const QByteArray& project, const QByteArray& user);

#endif // PROJECTUSERSTATE_H
