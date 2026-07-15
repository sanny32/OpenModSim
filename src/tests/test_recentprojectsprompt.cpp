// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

#include <QtTest>

#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

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
    QTimer::singleShot(0, this, [&promptIsSafe]() {
        auto* prompt = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        promptIsSafe = prompt
            && prompt->standardButton(prompt->defaultButton()) == QMessageBox::No
            && prompt->standardButtons() == (QMessageBox::Yes | QMessageBox::No);
        if(prompt)
            prompt->button(QMessageBox::No)->click();
    });

    const bool confirmed = RecentProjectsPrompt::confirmClear(nullptr,
                                                               QStringLiteral("Clear Recent Projects"),
                                                               QStringLiteral("Clear the list of recent projects?"));

    QVERIFY(promptIsSafe);
    QVERIFY(!confirmed);
}

void TestRecentProjectsPrompt::acceptsConfirmedClear()
{
    QTimer::singleShot(0, this, []() {
        auto* prompt = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if(prompt)
            prompt->button(QMessageBox::Yes)->click();
    });

    QVERIFY(RecentProjectsPrompt::confirmClear(nullptr,
                                                QStringLiteral("Clear Recent Projects"),
                                                QStringLiteral("Clear the list of recent projects?")));
}

QTEST_MAIN(TestRecentProjectsPrompt)

#include "test_recentprojectsprompt.moc"
