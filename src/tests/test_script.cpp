// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_script.cpp
/// \brief Unit tests for the script timer lifecycle.
///

#include <QJSEngine>
#include <QSignalSpy>
#include <QTest>

#include "jsobjects/script.h"

class TestScript : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void setTimeoutFiresCallback();
    void setTimeoutKeepsScriptBusyUntilItFires();
    void chainedTimeoutsKeepScriptBusy();
    void clearTimeoutCancelsCallback();
    void setIntervalRepeats();
    void setIntervalKeepsScriptBusy();
    void clearIntervalStopsRepeating();
    void onInitRunsOnFirstRunOnly();
    void stopAllTimersCancelsPendingCallbacks();
    void nonCallableArgumentIsIgnored();

private:
    QJSValue counterCallback(const QString& name);

    QJSEngine _engine;
};

void TestScript::init()
{
    _engine.globalObject().setProperty("calls", 0);
}

///
/// \brief Builds a JS callback that increments a global counter.
///
QJSValue TestScript::counterCallback(const QString& name)
{
    return _engine.evaluate(QString("(function() { %1 = %1 + 1; })").arg(name));
}

void TestScript::setTimeoutFiresCallback()
{
    Script script(0);

    const int id = script.setTimeout(counterCallback("calls"), 1);
    QVERIFY(id > 0);

    QTRY_COMPARE(_engine.globalObject().property("calls").toInt(), 1);
}

///
/// \brief Regression for issue #126: a pending timer must keep the script alive
/// so that Once mode does not tear the engine down right after evaluate().
///
void TestScript::setTimeoutKeepsScriptBusyUntilItFires()
{
    Script script(0);
    QSignalSpy idleSpy(&script, &Script::idle);

    script.setTimeout(counterCallback("calls"), 20);
    QVERIFY(script.hasPendingTimers());
    QCOMPARE(idleSpy.count(), 0);

    QTRY_COMPARE(idleSpy.count(), 1);
    QVERIFY(!script.hasPendingTimers());
    QCOMPARE(_engine.globalObject().property("calls").toInt(), 1);
}

///
/// \brief A callback that schedules another timeout must not report idle in between.
///
void TestScript::chainedTimeoutsKeepScriptBusy()
{
    Script script(0);
    QSignalSpy idleSpy(&script, &Script::idle);

    _engine.globalObject().setProperty("script", _engine.newQObject(&script));
    QJSValue chained = _engine.evaluate(
        "(function step() {"
        "    calls = calls + 1;"
        "    if(calls < 3) script.setTimeout(step, 1);"
        "})");
    QVERIFY(!chained.isError());

    script.setTimeout(chained, 1);

    QTRY_COMPARE(_engine.globalObject().property("calls").toInt(), 3);
    QTRY_COMPARE(idleSpy.count(), 1);
    QVERIFY(!script.hasPendingTimers());

    _engine.globalObject().deleteProperty("script");
}

void TestScript::clearTimeoutCancelsCallback()
{
    Script script(0);

    const int id = script.setTimeout(counterCallback("calls"), 10);
    script.clearTimeout(id);

    QVERIFY(!script.hasPendingTimers());
    QTest::qWait(40);
    QCOMPARE(_engine.globalObject().property("calls").toInt(), 0);
}

void TestScript::setIntervalRepeats()
{
    Script script(0);

    script.setInterval(counterCallback("calls"), 1);
    QTRY_VERIFY(_engine.globalObject().property("calls").toInt() >= 3);
}

///
/// \brief An active interval never reports idle - the script runs until stopped.
///
void TestScript::setIntervalKeepsScriptBusy()
{
    Script script(0);
    QSignalSpy idleSpy(&script, &Script::idle);

    script.setInterval(counterCallback("calls"), 1);
    QTRY_VERIFY(_engine.globalObject().property("calls").toInt() >= 2);

    QVERIFY(script.hasPendingTimers());
    QCOMPARE(idleSpy.count(), 0);
}

void TestScript::clearIntervalStopsRepeating()
{
    Script script(0);

    const int id = script.setInterval(counterCallback("calls"), 1);
    QTRY_VERIFY(_engine.globalObject().property("calls").toInt() >= 2);

    script.clearInterval(id);
    const int afterClear = _engine.globalObject().property("calls").toInt();

    QVERIFY(!script.hasPendingTimers());
    QTest::qWait(40);
    QCOMPARE(_engine.globalObject().property("calls").toInt(), afterClear);
}

void TestScript::onInitRunsOnFirstRunOnly()
{
    Script script(0);
    const auto callback = counterCallback("calls");

    script.run(_engine, QStringLiteral(""));
    script.onInit(callback);
    QCOMPARE(_engine.globalObject().property("calls").toInt(), 1);

    script.run(_engine, QStringLiteral(""));
    script.onInit(callback);
    QCOMPARE(_engine.globalObject().property("calls").toInt(), 1);
}

void TestScript::stopAllTimersCancelsPendingCallbacks()
{
    Script script(0);

    script.setTimeout(counterCallback("calls"), 5);
    script.setInterval(counterCallback("calls"), 5);
    QVERIFY(script.hasPendingTimers());

    script.stopAllTimers();
    QVERIFY(!script.hasPendingTimers());

    QTest::qWait(40);
    QCOMPARE(_engine.globalObject().property("calls").toInt(), 0);
}

void TestScript::nonCallableArgumentIsIgnored()
{
    Script script(0);

    QCOMPARE(script.setTimeout(_engine.toScriptValue(42), 1), 0);
    QCOMPARE(script.setInterval(QJSValue(), 1), 0);
    QVERIFY(!script.hasPendingTimers());
}

QTEST_GUILESS_MAIN(TestScript)
#include "test_script.moc"
