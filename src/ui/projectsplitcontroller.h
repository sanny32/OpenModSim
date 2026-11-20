// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectsplitcontroller.h
/// \brief Declares project split-view coordination.
///

#ifndef PROJECTSPLITCONTROLLER_H
#define PROJECTSPLITCONTROLLER_H

#include <QObject>
#include <QPoint>
#include <QString>

class MdiArea;
class MdiAreaEx;
class ProjectFormManager;
class QWidget;

///
/// \brief Coordinates split panels and transient visual form clones.
///
class ProjectSplitController : public QObject
{
    Q_OBJECT

public:
    /// \brief Creates a controller for an MDI workspace and form manager.
    ProjectSplitController(MdiAreaEx* mdiArea, ProjectFormManager* forms, QObject* parent = nullptr);

    /// \brief Returns the panel on which a new form should be opened.
    MdiArea* activeCreateArea() const;
    /// \brief Returns the panel containing a form.
    MdiArea* areaOfForm(QWidget* form) const;
    /// \brief Returns the secondary panel when it exists.
    MdiArea* secondaryArea() const;
    /// \brief Returns whether tabbed split view is active.
    bool isSplitTabbedView() const;

    /// \brief Resolves a canonical form to the peer on the active panel.
    QWidget* resolveForActiveArea(QWidget* canonicalForm) const;
    /// \brief Creates a transient visual clone on an MDI area.
    QWidget* createClone(QWidget* source, MdiArea* area);
    /// \brief Opens or activates a form on the active panel.
    void openOnActivePanel(QWidget* form);
    /// \brief Copies serialized state between compatible forms.
    bool cloneState(QWidget* source, QWidget* target) const;

    /// \brief Returns whether a form can move to the opposite panel.
    bool canMoveToOtherPanel(QWidget* form) const;
    /// \brief Moves a canonical form to the opposite panel.
    void moveToOtherPanel(QWidget* form, QPoint globalDropPos = QPoint());
    /// \brief Disables split view when the secondary panel becomes empty.
    void resetIfEmpty();
    /// \brief Returns the shared running state of a script pair.
    bool isScriptPairRunning(QWidget* form) const;
    /// \brief Updates the running icon of a script peer.
    void updateScriptPairIcons(QWidget* form);
    /// \brief Places the active primary form on the secondary panel.
    int duplicatePrimaryTab();
    /// \brief Removes transient secondary peers before merging panels.
    void removeSecondaryClones();

    /// \brief Stores active-window state parsed from a project.
    void setPendingActivation(const QString& primaryTitle,
                              const QString& secondaryTitle,
                              const QString& panel);
    /// \brief Applies pending active-window state.
    void restoreActiveWindows();

private:
    MdiAreaEx* _mdiArea;
    ProjectFormManager* _forms;
    QString _pendingPrimaryTitle;
    QString _pendingSecondaryTitle;
    QString _pendingPanel;
};

#endif // PROJECTSPLITCONTROLLER_H
