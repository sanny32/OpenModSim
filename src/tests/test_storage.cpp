// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_storage.cpp
/// \brief Unit tests for the script Storage key/value store.
///

#include <QJSEngine>
#include <QTest>

#include "jsobjects/storage.h"

class TestStorage : public QObject
{
    Q_OBJECT

private slots:
    void setGetAndLength();
    void missingKeyReturnsNull();
    void removeItem();
    void clear();
    void keyReturnsStoredValue();
    void keyAtLengthReturnsNull();
    void keyOutOfRangeReturnsNull();

private:
    QJSEngine _engine;
};

void TestStorage::setGetAndLength()
{
    Storage storage;
    QCOMPARE(storage.length(), 0);

    storage.setItem(QStringLiteral("a"), _engine.toScriptValue(42));
    storage.setItem(QStringLiteral("b"), _engine.toScriptValue(QStringLiteral("hi")));

    QCOMPARE(storage.length(), 2);
    QCOMPARE(storage.getItem(QStringLiteral("a")).toInt(), 42);
    QCOMPARE(storage.getItem(QStringLiteral("b")).toString(), QStringLiteral("hi"));
}

void TestStorage::missingKeyReturnsNull()
{
    Storage storage;
    QVERIFY(storage.getItem(QStringLiteral("missing")).isNull());
}

void TestStorage::removeItem()
{
    Storage storage;
    storage.setItem(QStringLiteral("a"), _engine.toScriptValue(1));
    storage.setItem(QStringLiteral("b"), _engine.toScriptValue(2));

    storage.removeItem(QStringLiteral("a"));
    QCOMPARE(storage.length(), 1);
    QVERIFY(storage.getItem(QStringLiteral("a")).isNull());
    QCOMPARE(storage.getItem(QStringLiteral("b")).toInt(), 2);
}

void TestStorage::clear()
{
    Storage storage;
    storage.setItem(QStringLiteral("a"), _engine.toScriptValue(1));
    storage.clear();
    QCOMPARE(storage.length(), 0);
}

void TestStorage::keyReturnsStoredValue()
{
    Storage storage;
    storage.setItem(QStringLiteral("only"), _engine.toScriptValue(7));
    QCOMPARE(storage.key(0).toInt(), 7);
}

void TestStorage::keyAtLengthReturnsNull()
{
    Storage storage;
    QVERIFY(storage.key(0).isNull());

    storage.setItem(QStringLiteral("only"), _engine.toScriptValue(1));
    QVERIFY(storage.key(storage.length()).isNull());
}

void TestStorage::keyOutOfRangeReturnsNull()
{
    Storage storage;
    storage.setItem(QStringLiteral("a"), _engine.toScriptValue(1));

    QVERIFY(storage.key(5).isNull());
    QVERIFY(storage.key(-1).isNull());
}

QTEST_GUILESS_MAIN(TestStorage)
#include "test_storage.moc"
