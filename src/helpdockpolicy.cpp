// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file helpdockpolicy.cpp
/// \brief Implements the script help dock visibility rules.
///

#include "helpdockpolicy.h"

///
/// \brief HelpDockPolicy::nextAction
/// \param state
/// \return the action to apply to the help dock
///
HelpDockPolicy::Action HelpDockPolicy::nextAction(const State& state)
{
    if(!state.hasActiveForm)
        return Action::None;

    if(state.isScriptForm)
        return (!state.isVisible && state.wasShown) ? Action::Show : Action::None;

    if(state.isVisible && !state.isFloating)
        return Action::HideAndRemember;

    return Action::None;
}
