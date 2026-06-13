// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_modbuserrorsimulations.cpp
/// \brief Unit tests for ModbusErrorSimulations get/set and serialization.
///

#include <QBuffer>
#include <QTemporaryDir>
#include <QTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "modbuserrorsimulations.h"

class TestModbusErrorSimulations : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreInactive();
    void settersRoundTripEachProperty();
    void settingsRoundTrip();
    void xmlRoundTrip();
    void xmlRejectsNegativeDelays();
};

void TestModbusErrorSimulations::defaultsAreInactive()
{
    ModbusErrorSimulations errsim;
    QVERIFY(!errsim.noResponse());
    QVERIFY(!errsim.responseIncorrectId());
    QVERIFY(!errsim.responseDelay());
    QCOMPARE(errsim.responseDelayTime(), 0);
    QCOMPARE(errsim.responseRandomDelayUpToTime(), 1000);
}

void TestModbusErrorSimulations::settersRoundTripEachProperty()
{
    ModbusErrorSimulations errsim;
    errsim.setNoResponse(true);
    errsim.setResponseIncorrectId(true);
    errsim.setResponseIllegalFunction(true);
    errsim.setResponseDeviceBusy(true);
    errsim.setResponseIncorrectCrc(true);
    errsim.setResponseDelay(true);
    errsim.setResponseDelayTime(250);
    errsim.setResponseRandomDelay(true);
    errsim.setResponseRandomDelayUpToTime(3000);

    QVERIFY(errsim.noResponse());
    QVERIFY(errsim.responseIncorrectId());
    QVERIFY(errsim.responseIllegalFunction());
    QVERIFY(errsim.responseDeviceBusy());
    QVERIFY(errsim.responseIncorrectCrc());
    QVERIFY(errsim.responseDelay());
    QCOMPARE(errsim.responseDelayTime(), 250);
    QVERIFY(errsim.responseRandomDelay());
    QCOMPARE(errsim.responseRandomDelayUpToTime(), 3000);
}

void TestModbusErrorSimulations::settingsRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("errsim.ini"));

    ModbusErrorSimulations source;
    source.setResponseDeviceBusy(true);
    source.setResponseDelay(true);
    source.setResponseDelayTime(123);
    source.setResponseRandomDelayUpToTime(456);

    {
        QSettings out(path, QSettings::IniFormat);
        out << source;
    }

    ModbusErrorSimulations restored;
    {
        QSettings in(path, QSettings::IniFormat);
        in >> restored;
    }

    QVERIFY(restored.responseDeviceBusy());
    QVERIFY(restored.responseDelay());
    QCOMPARE(restored.responseDelayTime(), 123);
    QCOMPARE(restored.responseRandomDelayUpToTime(), 456);
}

void TestModbusErrorSimulations::xmlRoundTrip()
{
    ModbusErrorSimulations source;
    source.setNoResponse(true);
    source.setResponseIncorrectCrc(true);
    source.setResponseDelayTime(77);
    source.setResponseRandomDelayUpToTime(888);

    QByteArray buffer;
    {
        QXmlStreamWriter writer(&buffer);
        writer << source;
    }

    ModbusErrorSimulations restored;
    QXmlStreamReader reader(buffer);
    reader.readNextStartElement();
    reader >> restored;

    QVERIFY(restored.noResponse());
    QVERIFY(restored.responseIncorrectCrc());
    QVERIFY(!restored.responseDeviceBusy());
    QCOMPARE(restored.responseDelayTime(), 77);
    QCOMPARE(restored.responseRandomDelayUpToTime(), 888);
}

void TestModbusErrorSimulations::xmlRejectsNegativeDelays()
{
    const QByteArray xml = R"(<ModbusErrorSimulations ResponseDelayTime="-5" ResponseRandomDelayUpToTime="-9"/>)";

    ModbusErrorSimulations restored;
    restored.setResponseDelayTime(10);
    restored.setResponseRandomDelayUpToTime(20);

    QXmlStreamReader reader(xml);
    reader.readNextStartElement();
    reader >> restored;

    QCOMPARE(restored.responseDelayTime(), 10);
    QCOMPARE(restored.responseRandomDelayUpToTime(), 20);
}

QTEST_GUILESS_MAIN(TestModbusErrorSimulations)
#include "test_modbuserrorsimulations.moc"
