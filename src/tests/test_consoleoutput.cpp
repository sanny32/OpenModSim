// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_consoleoutput.cpp
/// \brief Unit tests for the batched console output widget.
///

#include <QListWidget>
#include <QScrollBar>
#include <QTest>

#include "controls/consoleoutput.h"

class TestConsoleOutput : public QObject
{
    Q_OBJECT

private slots:
    void messagesAreNotLostWhenBatched();
    void flushAppliesEverythingSynchronously();
    void bulkInsertKeepsScrollAtBottom();
    void maxLinesEvictsOldestOnBulkInsert();
    void pendingMessagesBeyondMaxLinesAreDropped();
    void clearDropsPendingMessages();
    void isEmptyAccountsForPendingMessages();

private:
    static QListWidget* listOf(ConsoleOutput& console);
};

///
/// \brief The widget owns a single QListWidget holding the rendered messages.
///
QListWidget* TestConsoleOutput::listOf(ConsoleOutput& console)
{
    auto* list = console.findChild<QListWidget*>();
    Q_ASSERT(list != nullptr);
    return list;
}

///
/// \brief Regression for issue #126: a burst of log calls must all arrive,
/// without a layout pass per message.
///
void TestConsoleOutput::messagesAreNotLostWhenBatched()
{
    ConsoleOutput console;
    console.setMaxLines(2000);

    for (int i = 0; i < 1000; ++i)
        console.addMessage(QString("message %1").arg(i), ConsoleOutput::MessageType::Log);

    console.flush();

    auto* list = listOf(console);
    QCOMPARE(list->count(), 1000);
    QCOMPARE(list->item(0)->text(), QStringLiteral("message 0"));
    QCOMPARE(list->item(999)->text(), QStringLiteral("message 999"));
}

void TestConsoleOutput::flushAppliesEverythingSynchronously()
{
    ConsoleOutput console;

    console.addMessage(QStringLiteral("first"), ConsoleOutput::MessageType::Log);
    QCOMPARE(listOf(console)->count(), 0);

    console.flush();
    QCOMPARE(listOf(console)->count(), 1);
}

void TestConsoleOutput::bulkInsertKeepsScrollAtBottom()
{
    ConsoleOutput console;
    console.setMaxLines(2000);
    console.resize(400, 200);
    console.show();
    QVERIFY(QTest::qWaitForWindowExposed(&console));

    for (int i = 0; i < 500; ++i)
        console.addMessage(QString("message %1").arg(i), ConsoleOutput::MessageType::Log);

    console.flush();

    auto* bar = listOf(console)->verticalScrollBar();
    QCOMPARE(bar->value(), bar->maximum());
}

void TestConsoleOutput::maxLinesEvictsOldestOnBulkInsert()
{
    ConsoleOutput console;
    console.setMaxLines(100);

    for (int i = 0; i < 250; ++i)
        console.addMessage(QString("message %1").arg(i), ConsoleOutput::MessageType::Log);

    console.flush();

    auto* list = listOf(console);
    QCOMPARE(list->count(), 100);
    QCOMPARE(list->item(0)->text(), QStringLiteral("message 150"));
    QCOMPARE(list->item(99)->text(), QStringLiteral("message 249"));
}

///
/// \brief The pending queue must not grow past what the widget can display.
///
void TestConsoleOutput::pendingMessagesBeyondMaxLinesAreDropped()
{
    ConsoleOutput console;
    console.setMaxLines(10);

    for (int i = 0; i < 500; ++i)
        console.addMessage(QString("message %1").arg(i), ConsoleOutput::MessageType::Log);

    console.flush();

    auto* list = listOf(console);
    QCOMPARE(list->count(), 10);
    QCOMPARE(list->item(9)->text(), QStringLiteral("message 499"));
}

void TestConsoleOutput::clearDropsPendingMessages()
{
    ConsoleOutput console;

    console.addMessage(QStringLiteral("pending"), ConsoleOutput::MessageType::Log);
    console.clear();

    QVERIFY(console.isEmpty());

    QTest::qWait(60);
    QCOMPARE(listOf(console)->count(), 0);
}

void TestConsoleOutput::isEmptyAccountsForPendingMessages()
{
    ConsoleOutput console;
    QVERIFY(console.isEmpty());

    console.addMessage(QStringLiteral("queued"), ConsoleOutput::MessageType::Warning);
    QVERIFY(!console.isEmpty());

    console.flush();
    QVERIFY(!console.isEmpty());
}

QTEST_MAIN(TestConsoleOutput)
#include "test_consoleoutput.moc"
