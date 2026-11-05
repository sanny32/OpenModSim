// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file helpdockpolicy.h
/// \brief Declares the script help dock visibility rules.
///

#ifndef HELPDOCKPOLICY_H
#define HELPDOCKPOLICY_H

///
/// \brief Visibility rules for the script help dock.
///
/// The help applies to script forms only, so it is hidden while another form
/// kind is active and restored when a script form comes back. A floating dock
/// is left alone: undocking it is an explicit request to keep it around.
///
namespace HelpDockPolicy
{
    ///
    /// \brief What should happen to the dock.
    ///
    enum class Action
    {
        None,
        Show,
        HideAndRemember
    };

    ///
    /// \brief Everything the rules depend on.
    ///
    struct State
    {
        bool hasActiveForm = false;
        bool isScriptForm  = false;
        bool isVisible     = false;
        bool isFloating    = false;
        bool wasShown      = false;
    };

    Action nextAction(const State& state);
}

#endif // HELPDOCKPOLICY_H
