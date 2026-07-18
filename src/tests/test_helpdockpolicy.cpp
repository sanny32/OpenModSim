// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_helpdockpolicy.cpp
/// \brief Unit tests for the script help dock visibility rules.
///

#include <QTest>

#include "helpdockpolicy.h"

using HelpDockPolicy::Action;
using HelpDockPolicy::State;

class TestHelpDockPolicy : public QObject
{
    Q_OBJECT

private slots:
    void noActiveFormLeavesDockAlone();

    void scriptFormRestoresRememberedDock();
    void scriptFormKeepsVisibleDock();
    void scriptFormDoesNotShowDockTheUserClosed();

    void otherFormHidesAndRemembersDock();
    void otherFormLeavesFloatingDockAlone();
    void otherFormLeavesHiddenDockAlone();

    void firstRunRevealsHelpOnFirstScriptForm();
    void userClosedHelpStaysClosedAcrossForms();

private:
    static State scriptForm(bool visible, bool wasShown, bool floating = false);
    static State otherForm(bool visible, bool wasShown, bool floating = false);
};

State TestHelpDockPolicy::scriptForm(bool visible, bool wasShown, bool floating)
{
    State s;
    s.hasActiveForm = true;
    s.isScriptForm  = true;
    s.isVisible     = visible;
    s.isFloating    = floating;
    s.wasShown      = wasShown;
    return s;
}

State TestHelpDockPolicy::otherForm(bool visible, bool wasShown, bool floating)
{
    State s = scriptForm(visible, wasShown, floating);
    s.isScriptForm = false;
    return s;
}

void TestHelpDockPolicy::noActiveFormLeavesDockAlone()
{
    State s;
    s.hasActiveForm = false;
    s.isVisible = true;

    QCOMPARE(HelpDockPolicy::nextAction(s), Action::None);
}

void TestHelpDockPolicy::scriptFormRestoresRememberedDock()
{
    QCOMPARE(HelpDockPolicy::nextAction(scriptForm(false, true)), Action::Show);
}

void TestHelpDockPolicy::scriptFormKeepsVisibleDock()
{
    QCOMPARE(HelpDockPolicy::nextAction(scriptForm(true, true)), Action::None);
    QCOMPARE(HelpDockPolicy::nextAction(scriptForm(true, false)), Action::None);
}

///
/// \brief Without the remembered flag the dock stays closed: the user shut it.
///
void TestHelpDockPolicy::scriptFormDoesNotShowDockTheUserClosed()
{
    QCOMPARE(HelpDockPolicy::nextAction(scriptForm(false, false)), Action::None);
}

///
/// \brief The help belongs to script forms, so it steps aside for other forms.
///
void TestHelpDockPolicy::otherFormHidesAndRemembersDock()
{
    QCOMPARE(HelpDockPolicy::nextAction(otherForm(true, false)), Action::HideAndRemember);
}

///
/// \brief Undocking is an explicit request to keep the help around.
///
void TestHelpDockPolicy::otherFormLeavesFloatingDockAlone()
{
    QCOMPARE(HelpDockPolicy::nextAction(otherForm(true, false, /*floating*/ true)), Action::None);
    QCOMPARE(HelpDockPolicy::nextAction(otherForm(true, true, /*floating*/ true)), Action::None);
}

void TestHelpDockPolicy::otherFormLeavesHiddenDockAlone()
{
    QCOMPARE(HelpDockPolicy::nextAction(otherForm(false, true)), Action::None);
    QCOMPARE(HelpDockPolicy::nextAction(otherForm(false, false)), Action::None);
}

///
/// \brief Issue #126 follow-up: on a fresh profile the help is armed, so it
/// stays out of the way on the initial data form and appears as soon as the
/// user opens a script form.
///
void TestHelpDockPolicy::firstRunRevealsHelpOnFirstScriptForm()
{
    // Fresh profile: dock hidden, armed by loadAppSettings().
    bool visible = false;
    const bool wasShown = true;

    // The first run opens a data form - nothing should pop up there.
    QCOMPARE(HelpDockPolicy::nextAction(otherForm(visible, wasShown)), Action::None);

    // The user switches to a script form - the help reveals itself.
    const auto action = HelpDockPolicy::nextAction(scriptForm(visible, wasShown));
    QCOMPARE(action, Action::Show);

    // Once shown, staying on the script form must not fight the user.
    visible = true;
    QCOMPARE(HelpDockPolicy::nextAction(scriptForm(visible, /*wasShown*/ false)), Action::None);
}

///
/// \brief Closing the help must stick: no form switch may bring it back.
///
void TestHelpDockPolicy::userClosedHelpStaysClosedAcrossForms()
{
    // The user closed the dock; on_helpDockVisibilityChanged() leaves
    // WasShown false because it is only cleared when the dock becomes visible.
    const bool visible = false;
    const bool wasShown = false;

    QCOMPARE(HelpDockPolicy::nextAction(otherForm(visible, wasShown)), Action::None);
    QCOMPARE(HelpDockPolicy::nextAction(scriptForm(visible, wasShown)), Action::None);
    QCOMPARE(HelpDockPolicy::nextAction(otherForm(visible, wasShown)), Action::None);
}

QTEST_GUILESS_MAIN(TestHelpDockPolicy)
#include "test_helpdockpolicy.moc"
