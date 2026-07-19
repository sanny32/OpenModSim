// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectformmetadata.h
/// \brief Declares accessors for project form runtime metadata.
///

#ifndef PROJECTFORMMETADATA_H
#define PROJECTFORMMETADATA_H

#include <QUuid>

class QWidget;

namespace ProjectFormMetadata {

inline constexpr const char* FormId = "FormId";
inline constexpr const char* SplitOriginId = "SplitOriginId";
inline constexpr const char* SplitAutoClone = "SplitAutoClone";
inline constexpr const char* SplitScriptRunning = "SplitScriptRunning";
inline constexpr const char* DeleteLocked = "DeleteLocked";
inline constexpr const char* Closed = "Closed";
inline constexpr const char* SplitPanel = "SplitPanel";

}

///
/// \brief Returns the persistent identity assigned to a project form.
/// \param form Project form widget.
/// \return Form UUID, or a null UUID when it has not been assigned.
///
QUuid projectFormId(const QWidget* form);

///
/// \brief Assigns the persistent identity of a project form.
/// \param form Project form widget.
/// \param id Form UUID.
///
void setProjectFormId(QWidget* form, const QUuid& id);

///
/// \brief Returns the canonical form UUID shared by a split pair.
/// \param form Project form or split clone.
/// \return Origin UUID, falling back to the form UUID.
///
QUuid splitOriginId(const QWidget* form);

///
/// \brief Assigns the canonical form UUID to a split peer.
/// \param form Project form or split clone.
/// \param id Canonical form UUID.
///
void setSplitOriginId(QWidget* form, const QUuid& id);

///
/// \brief Returns whether the widget is a transient split clone.
/// \param form Project form widget.
/// \return True for a transient clone.
///
bool isSplitClone(const QWidget* form);

///
/// \brief Marks a widget as a transient split clone.
/// \param form Project form widget.
/// \param clone Clone state.
///
void setSplitClone(QWidget* form, bool clone);

///
/// \brief Returns whether permanent deletion of the form is disabled.
/// \param form Project form widget.
/// \return True when deletion is locked.
///
bool isFormDeletionLocked(const QWidget* form);

///
/// \brief Returns the shared running state of a split script pair.
/// \param form Script form widget.
/// \return True when the pair is running.
///
bool isSplitScriptRunning(const QWidget* form);

#endif // PROJECTFORMMETADATA_H
