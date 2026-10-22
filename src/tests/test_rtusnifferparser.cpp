// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_rtusnifferparser.cpp
/// \brief Unit tests for RtuSnifferParser frame splitting and direction detection.
///

#include <QTest>

#include "qmodbusadurtu.h"
#include "rtusnifferparser.h"

namespace {

QByteArray withCrc(const QByteArray& hex)
{
    QByteArray frame = QByteArray::fromHex(hex);
    const quint16 crc = QModbusAduRtu::calculateCRC(frame.constData(), frame.size());
    frame.append(char(crc >> 8));
    frame.append(char(crc & 0xff));
    return frame;
}

QByteArray withBadCrc(const QByteArray& hex)
{
    QByteArray frame = withCrc(hex);
    frame[frame.size() - 1] = char(frame.at(frame.size() - 1) ^ 0xff);
    return frame;
}

QString request(const QByteArray& frame)
{
    return QStringLiteral("Q:") + QString::fromLatin1(frame.toHex());
}

QString answer(const QByteArray& frame)
{
    return QStringLiteral("A:") + QString::fromLatin1(frame.toHex());
}

QString chunk(const QByteArray& data, bool afterSilence = true)
{
    return (afterSilence ? QStringLiteral("+") : QString()) + QString::fromLatin1(data.toHex());
}

const QString Flush = QStringLiteral("!");

///
/// \brief Runs parser steps: "+hex" feeds after silence, "hex" feeds without a gap, "!" flushes.
///
QStringList run(RtuSnifferParser& parser, const QStringList& steps)
{
    QStringList result;
    for (const auto& step : steps) {
        QVector<RtuSniffedFrame> frames;
        if (step == Flush) {
            frames = parser.flush();
        } else {
            const bool afterSilence = step.startsWith(QLatin1Char('+'));
            const QByteArray hex = (afterSilence ? step.mid(1) : step).toLatin1();
            frames = parser.feed(QByteArray::fromHex(hex), afterSilence);
        }

        for (const auto& frame : frames)
            result << (frame.IsRequest ? request(frame.Data) : answer(frame.Data));
    }
    return result;
}

}

class TestRtuSnifferParser : public QObject
{
    Q_OBJECT

private slots:
    void splitsBusTraffic_data();
    void splitsBusTraffic();
    void keepsIncompleteFrameUntilFlush();
    void resetDropsBufferAndPairing();
};

void TestRtuSnifferParser::splitsBusTraffic_data()
{
    QTest::addColumn<QStringList>("steps");
    QTest::addColumn<QStringList>("expected");

    const QByteArray readReq = withCrc("010300000002");
    const QByteArray readResp = withCrc("010304000a000b");
    const QByteArray writeReq = withCrc("010600010003");
    const QByteArray broadcastWrite = withCrc("000600010003");
    const QByteArray writeMultiReq = withCrc("0110000000020400010002");
    const QByteArray writeMultiResp = withCrc("011000000002");
    const QByteArray exception = withCrc("018302");
    const QByteArray otherUnitReq = withCrc("070400100001");
    const QByteArray otherUnitResp = withCrc("0704020102");
    const QByteArray badReadReq = withBadCrc("010300000002");
    const QByteArray badReadResp = withBadCrc("010304000a000b");
    const QByteArray garbage = QByteArray::fromHex("ffff");
    const QByteArray truncatedWriteMulti = QByteArray::fromHex("01100000");
    const QByteArray largeResp = withCrc("0103c8" + QByteArray(400, 'a'));

    QTest::newRow("request")
        << QStringList{ chunk(readReq) }
        << QStringList{ request(readReq) };
    QTest::newRow("request and response")
        << QStringList{ chunk(readReq), chunk(readResp) }
        << QStringList{ request(readReq), answer(readResp) };
    QTest::newRow("request and response in one chunk")
        << QStringList{ chunk(readReq + readResp) }
        << QStringList{ request(readReq), answer(readResp) };
    QTest::newRow("response without gap")
        << QStringList{ chunk(readReq), chunk(readResp, false) }
        << QStringList{ request(readReq), answer(readResp) };
    QTest::newRow("frame split by driver delay")
        << QStringList{ chunk(readReq.left(3)), chunk(readReq.mid(3)) }
        << QStringList{ request(readReq) };
    QTest::newRow("large response in three delayed chunks")
        << QStringList{ chunk(readReq), chunk(largeResp.left(60)), chunk(largeResp.mid(60, 80)), chunk(largeResp.mid(140)) }
        << QStringList{ request(readReq), answer(largeResp) };
    QTest::newRow("echo response")
        << QStringList{ chunk(writeReq), chunk(writeReq) }
        << QStringList{ request(writeReq), answer(writeReq) };
    QTest::newRow("echo without preceding request")
        << QStringList{ chunk(writeReq) }
        << QStringList{ request(writeReq) };
    QTest::newRow("broadcast gets no response")
        << QStringList{ chunk(broadcastWrite), chunk(broadcastWrite) }
        << QStringList{ request(broadcastWrite), request(broadcastWrite) };
    QTest::newRow("write multiple")
        << QStringList{ chunk(writeMultiReq), chunk(writeMultiResp) }
        << QStringList{ request(writeMultiReq), answer(writeMultiResp) };
    QTest::newRow("exception response")
        << QStringList{ chunk(readReq), chunk(exception) }
        << QStringList{ request(readReq), answer(exception) };
    QTest::newRow("several units")
        << QStringList{ chunk(readReq), chunk(readResp), chunk(otherUnitReq), chunk(otherUnitResp) }
        << QStringList{ request(readReq), answer(readResp), request(otherUnitReq), answer(otherUnitResp) };
    QTest::newRow("garbage before frame")
        << QStringList{ chunk(garbage), chunk(readReq) }
        << QStringList{ request(garbage), request(readReq) };
    QTest::newRow("truncated frame before frame")
        << QStringList{ chunk(truncatedWriteMulti), chunk(readReq) }
        << QStringList{ request(truncatedWriteMulti), request(readReq) };
    QTest::newRow("bad crc request")
        << QStringList{ chunk(badReadReq), Flush }
        << QStringList{ request(badReadReq) };
    QTest::newRow("bad crc response")
        << QStringList{ chunk(readReq), chunk(badReadResp), Flush }
        << QStringList{ request(readReq), answer(badReadResp) };
    QTest::newRow("bad crc frame before frame")
        << QStringList{ chunk(badReadReq), chunk(readReq) }
        << QStringList{ request(badReadReq), request(readReq) };
    QTest::newRow("oversized garbage")
        << QStringList{ chunk(QByteArray(300, char(0xff))) }
        << QStringList{ request(QByteArray(300, char(0xff))) };
    QTest::newRow("flush of empty buffer")
        << QStringList{ Flush }
        << QStringList{};
}

void TestRtuSnifferParser::splitsBusTraffic()
{
    QFETCH(QStringList, steps);
    QFETCH(QStringList, expected);

    RtuSnifferParser parser;
    QCOMPARE(run(parser, steps), expected);
    QVERIFY(!parser.hasPendingData());
}

void TestRtuSnifferParser::keepsIncompleteFrameUntilFlush()
{
    const QByteArray readReq = withCrc("010300000002");

    RtuSnifferParser parser;
    QVERIFY(parser.feed(readReq.left(5), true).isEmpty());
    QVERIFY(parser.hasPendingData());

    const auto frames = parser.flush();
    QCOMPARE(frames.size(), 1);
    QCOMPARE(frames.first().Data, readReq.left(5));
    QVERIFY(!parser.hasPendingData());
}

void TestRtuSnifferParser::resetDropsBufferAndPairing()
{
    const QByteArray writeReq = withCrc("010600010003");

    RtuSnifferParser parser;
    QCOMPARE(run(parser, { chunk(writeReq), chunk(writeReq.left(3)) }), QStringList{ request(writeReq) });
    QVERIFY(parser.hasPendingData());

    parser.reset();
    QVERIFY(!parser.hasPendingData());
    QVERIFY(parser.flush().isEmpty());

    QCOMPARE(run(parser, { chunk(writeReq) }), QStringList{ request(writeReq) });
}

QTEST_GUILESS_MAIN(TestRtuSnifferParser)
#include "test_rtusnifferparser.moc"
