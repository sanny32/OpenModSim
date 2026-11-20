// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_cacheprune.cpp
/// \brief Tests cleanup of superseded CI cache entries.
///

#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTest>

class TestCachePrune : public QObject
{
    Q_OBJECT
private slots:
    void deletesMatchingEntriesExceptKeptKey();
    void rejectsEmptyPrefix();
    void reportsListFailures();
    void reportsDeleteFailures();
};

namespace {
struct CacheFixture
{
    QTemporaryDir directory;
    QString logPath = directory.filePath(QStringLiteral("calls.txt"));

    /// \brief Installs a deterministic substitute for the GitHub CLI.
    bool prepare()
    {
        QFile cli(directory.filePath(QStringLiteral("gh")));
        if (!cli.open(QIODevice::WriteOnly))
            return false;
        cli.write("#!/bin/sh\n"
                  "printf '%s\\n' \"$*\" >> \"$MOCK_LOG\"\n"
                  "if [ \"$2\" = list ]; then\n"
                  "  [ \"${MOCK_FAIL:-0}\" = 0 ] || exit 3\n"
                  "  printf '%s\\n' \"$MOCK_ENTRIES\"\n"
                  "fi\n"
                  "if [ \"$2\" = delete ] && [ \"${MOCK_DELETE_FAIL:-0}\" = 1 ]; then exit 4; fi\n");
        cli.close();
        return cli.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    }

    /// \brief Runs cleanup with an isolated CLI and cache listing.
    int run(const QString& entries, const QString& prefix, const QString& keep = {},
            bool fail = false, bool deleteFail = false)
    {
        auto env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("PATH"), directory.path() + QLatin1Char(':') + env.value(QStringLiteral("PATH")));
        env.insert(QStringLiteral("GITHUB_REF"), QStringLiteral("refs/heads/dev"));
        env.insert(QStringLiteral("PREFIX"), prefix);
        env.insert(QStringLiteral("KEEP"), keep);
        env.insert(QStringLiteral("MOCK_LOG"), logPath);
        env.insert(QStringLiteral("MOCK_ENTRIES"), entries);
        env.insert(QStringLiteral("MOCK_FAIL"), fail ? QStringLiteral("1") : QStringLiteral("0"));
        env.insert(QStringLiteral("MOCK_DELETE_FAIL"), deleteFail ? QStringLiteral("1") : QStringLiteral("0"));
        QProcess process;
        process.setProcessEnvironment(env);
        process.start(QStringLiteral("/bin/bash"), {QStringLiteral(OMODSIM_CACHE_PRUNE_SCRIPT)});
        if (!process.waitForFinished(5000) || process.exitStatus() != QProcess::NormalExit)
            return -1;
        return process.exitCode();
    }

    /// \brief Returns the commands recorded by the substitute CLI.
    QByteArray calls() const
    {
        QFile file(logPath);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
};
}

/// \brief Deletes only entries for the requested prefix and current ref.
void TestCachePrune::deletesMatchingEntriesExceptKeptKey()
{
#ifdef Q_OS_WIN
    QSKIP("The cleanup action runs only on Linux and macOS.");
#else
    CacheFixture fixture;
    QVERIFY(fixture.prepare());
    const QString entries = QStringLiteral("1 ccache-tests-linux-old\n2 ccache-tests-linux-keep\n3 unrelated");
    QCOMPARE(fixture.run(entries, QStringLiteral("ccache-tests-linux-"),
                         QStringLiteral("ccache-tests-linux-keep")), 0);
    const auto calls = fixture.calls();
    QVERIFY(calls.contains("list --ref refs/heads/dev --key ccache-tests-linux-"));
    QVERIFY(calls.contains("cache delete 1"));
    QVERIFY(!calls.contains("cache delete 2"));
    QVERIFY(!calls.contains("cache delete 3"));
#endif
}

/// \brief Prevents a missing prefix from selecting unrelated caches.
void TestCachePrune::rejectsEmptyPrefix()
{
#ifdef Q_OS_WIN
    QSKIP("The cleanup action runs only on Linux and macOS.");
#else
    CacheFixture fixture;
    QVERIFY(fixture.prepare());
    QVERIFY(fixture.run(QStringLiteral("1 unrelated"), QString()) != 0);
    QVERIFY(fixture.calls().isEmpty());
#endif
}

/// \brief Propagates cache-listing failures instead of reporting a successful cleanup.
void TestCachePrune::reportsListFailures()
{
#ifdef Q_OS_WIN
    QSKIP("The cleanup action runs only on Linux and macOS.");
#else
    CacheFixture fixture;
    QVERIFY(fixture.prepare());
    QVERIFY(fixture.run(QString(), QStringLiteral("ccache-tests-"), {}, true) != 0);
    QVERIFY(!fixture.calls().contains("cache delete"));
#endif
}

/// \brief Propagates failures while deleting a matching cache.
void TestCachePrune::reportsDeleteFailures()
{
#ifdef Q_OS_WIN
    QSKIP("The cleanup action runs only on Linux and macOS.");
#else
    CacheFixture fixture;
    QVERIFY(fixture.prepare());
    QVERIFY(fixture.run(QStringLiteral("1 ccache-tests-old"), QStringLiteral("ccache-tests-"), {}, false, true) != 0);
    QVERIFY(fixture.calls().contains("cache delete 1"));
#endif
}

QTEST_MAIN(TestCachePrune)
#include "test_cacheprune.moc"
