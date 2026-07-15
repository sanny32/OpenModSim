// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

#include <QtTest>

#include "recentprojectsprompt.h"

class TestRecentProjectsPrompt : public QObject
{
    Q_OBJECT

private slots:
    void rejectsClearByDefault();
    void acceptsConfirmedClear();
};

void TestRecentProjectsPrompt::rejectsClearByDefault()
{
    bool promptIsSafe = false;
    const bool confirmed = RecentProjectsPrompt::confirmClear(nullptr,
                                                               QStringLiteral("Clear Recent Projects"),
                                                               QStringLiteral("Clear the list of recent projects?"),
                                                               [&promptIsSafe](QWidget* parent,
                                                                               const QString& title,
                                                                               const QString& text,
                                                                               QMessageBox::StandardButtons buttons,
                                                                               QMessageBox::StandardButton defaultButton) {
        promptIsSafe = !parent
            && title == QStringLiteral("Clear Recent Projects")
            && text == QStringLiteral("Clear the list of recent projects?")
            && buttons == (QMessageBox::Yes | QMessageBox::No)
            && defaultButton == QMessageBox::No;
        return QMessageBox::No;
    });

    QVERIFY(promptIsSafe);
    QVERIFY(!confirmed);
}

void TestRecentProjectsPrompt::acceptsConfirmedClear()
{
    QVERIFY(RecentProjectsPrompt::confirmClear(nullptr,
                                                QStringLiteral("Clear Recent Projects"),
                                                QStringLiteral("Clear the list of recent projects?"),
                                                [](QWidget*, const QString&, const QString&,
                                                   QMessageBox::StandardButtons,
                                                   QMessageBox::StandardButton) {
        return QMessageBox::Yes;
    }));
}

QTEST_APPLESS_MAIN(TestRecentProjectsPrompt)

#include "test_recentprojectsprompt.moc"
