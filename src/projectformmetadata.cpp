// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectformmetadata.cpp
/// \brief Implements accessors for project form runtime metadata.
///

#include "projectformmetadata.h"

#include <QWidget>

///
/// \brief Returns the persistent identity assigned to a project form.
/// \param form Project form widget.
/// \return Form UUID, or a null UUID when it has not been assigned.
///
QUuid projectFormId(const QWidget* form)
{
    return form ? form->property(ProjectFormMetadata::FormId).toUuid() : QUuid();
}

///
/// \brief Assigns the persistent identity of a project form.
/// \param form Project form widget.
/// \param id Form UUID.
///
void setProjectFormId(QWidget* form, const QUuid& id)
{
    if (form)
        form->setProperty(ProjectFormMetadata::FormId, id);
}

///
/// \brief Returns the canonical form UUID shared by a split pair.
/// \param form Project form or split clone.
/// \return Origin UUID, falling back to the form UUID.
///
QUuid splitOriginId(const QWidget* form)
{
    if (!form)
        return {};

    const auto originId = form->property(ProjectFormMetadata::SplitOriginId).toUuid();
    return originId.isNull() ? projectFormId(form) : originId;
}

///
/// \brief Assigns the canonical form UUID to a split peer.
/// \param form Project form or split clone.
/// \param id Canonical form UUID.
///
void setSplitOriginId(QWidget* form, const QUuid& id)
{
    if (form)
        form->setProperty(ProjectFormMetadata::SplitOriginId, id);
}

///
/// \brief Returns whether the widget is a transient split clone.
/// \param form Project form widget.
/// \return True for a transient clone.
///
bool isSplitClone(const QWidget* form)
{
    return form && form->property(ProjectFormMetadata::SplitAutoClone).toBool();
}

///
/// \brief Marks a widget as a transient split clone.
/// \param form Project form widget.
/// \param clone Clone state.
///
void setSplitClone(QWidget* form, bool clone)
{
    if (form)
        form->setProperty(ProjectFormMetadata::SplitAutoClone, clone);
}

///
/// \brief Returns whether permanent deletion of the form is disabled.
/// \param form Project form widget.
/// \return True when deletion is locked.
///
bool isFormDeletionLocked(const QWidget* form)
{
    return form && form->property(ProjectFormMetadata::DeleteLocked).toBool();
}

///
/// \brief Returns the shared running state of a split script pair.
/// \param form Script form widget.
/// \return True when the pair is running.
///
bool isSplitScriptRunning(const QWidget* form)
{
    return form && form->property(ProjectFormMetadata::SplitScriptRunning).toBool();
}
