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
    void xmlReadsEveryBooleanAttribute();
    void xmlMissingAttributesKeepExistingValues();
    void xmlIgnoresWrongRootElement();
    void xmlIgnoresWrongReaderState();
    void xmlRejectsNonNumericDelays();
    void settingsDefaultsWhenGroupIsMissing();
    void settersClampNegativeDelayValues();
    void settingsReadStringBooleansAndClampNegativeDelays();
    void xmlWriterSerializesEveryAttribute();
    void settingsWriterSerializesFalseAndTrueValues();
    void xmlDelayBoundsAcceptZeroAndPositiveValues();
    void xmlBooleanAttributesAcceptEveryTrueToken();
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

void TestModbusErrorSimulations::xmlReadsEveryBooleanAttribute()
{
    const QByteArray xml = R"(<ModbusErrorSimulations
        NoResponse="yes"
        ResponseIncorrectId="1"
        ResponseIllegalFunction="on"
        ResponseDeviceBusy="true"
        ResponseIncorrectCrc="false"
        ResponseDelay="0"
        ResponseRandomDelay="anything"
        ResponseDelayTime="12"
        ResponseRandomDelayUpToTime="34"/>)";

    ModbusErrorSimulations restored;
    QXmlStreamReader reader(xml);
    reader.readNextStartElement();
    reader >> restored;

    QVERIFY(restored.noResponse());
    QVERIFY(restored.responseIncorrectId());
    QVERIFY(restored.responseIllegalFunction());
    QVERIFY(restored.responseDeviceBusy());
    QVERIFY(!restored.responseIncorrectCrc());
    QVERIFY(!restored.responseDelay());
    QVERIFY(!restored.responseRandomDelay());
    QCOMPARE(restored.responseDelayTime(), 12);
    QCOMPARE(restored.responseRandomDelayUpToTime(), 34);
}

void TestModbusErrorSimulations::xmlMissingAttributesKeepExistingValues()
{
    ModbusErrorSimulations restored;
    restored.setNoResponse(true);
    restored.setResponseIncorrectId(true);
    restored.setResponseIllegalFunction(true);
    restored.setResponseDeviceBusy(true);
    restored.setResponseIncorrectCrc(true);
    restored.setResponseDelay(true);
    restored.setResponseDelayTime(55);
    restored.setResponseRandomDelay(true);
    restored.setResponseRandomDelayUpToTime(66);

    QXmlStreamReader reader(QByteArrayLiteral("<ModbusErrorSimulations ResponseDelayTime=\"77\"/>"));
    reader.readNextStartElement();
    reader >> restored;

    QVERIFY(restored.noResponse());
    QVERIFY(restored.responseIncorrectId());
    QVERIFY(restored.responseIllegalFunction());
    QVERIFY(restored.responseDeviceBusy());
    QVERIFY(restored.responseIncorrectCrc());
    QVERIFY(restored.responseDelay());
    QVERIFY(restored.responseRandomDelay());
    QCOMPARE(restored.responseDelayTime(), 77);
    QCOMPARE(restored.responseRandomDelayUpToTime(), 66);
}

void TestModbusErrorSimulations::xmlIgnoresWrongRootElement()
{
    ModbusErrorSimulations restored;
    restored.setNoResponse(true);
    restored.setResponseDelayTime(55);

    QXmlStreamReader reader(QByteArrayLiteral("<NotModbusErrorSimulations NoResponse=\"false\" ResponseDelayTime=\"0\"/>"));
    reader.readNextStartElement();
    reader >> restored;

    QVERIFY(restored.noResponse());
    QCOMPARE(restored.responseDelayTime(), 55);
}

void TestModbusErrorSimulations::xmlIgnoresWrongReaderState()
{
    ModbusErrorSimulations restored;
    restored.setNoResponse(true);

    QXmlStreamReader reader(QByteArrayLiteral("<ModbusErrorSimulations NoResponse=\"false\"/>"));
    reader.readNext();
    reader >> restored;

    QVERIFY(restored.noResponse());
}

void TestModbusErrorSimulations::xmlRejectsNonNumericDelays()
{
    const QByteArray xml = R"(<ModbusErrorSimulations ResponseDelayTime="bad" ResponseRandomDelayUpToTime="also-bad"/>)";

    ModbusErrorSimulations restored;
    restored.setResponseDelayTime(10);
    restored.setResponseRandomDelayUpToTime(20);

    QXmlStreamReader reader(xml);
    reader.readNextStartElement();
    reader >> restored;

    QCOMPARE(restored.responseDelayTime(), 10);
    QCOMPARE(restored.responseRandomDelayUpToTime(), 20);
}

void TestModbusErrorSimulations::settingsDefaultsWhenGroupIsMissing()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QSettings settings(dir.filePath(QStringLiteral("empty.ini")), QSettings::IniFormat);
    ModbusErrorSimulations restored;
    restored.setResponseRandomDelayUpToTime(42);

    settings >> restored;

    QVERIFY(!restored.noResponse());
    QVERIFY(!restored.responseIncorrectId());
    QVERIFY(!restored.responseIllegalFunction());
    QVERIFY(!restored.responseDeviceBusy());
    QVERIFY(!restored.responseIncorrectCrc());
    QVERIFY(!restored.responseDelay());
    QCOMPARE(restored.responseDelayTime(), 0);
    QVERIFY(!restored.responseRandomDelay());
    QCOMPARE(restored.responseRandomDelayUpToTime(), 1000);
}

void TestModbusErrorSimulations::settersClampNegativeDelayValues()
{
    ModbusErrorSimulations errsim;
    errsim.setResponseDelayTime(-1);
    errsim.setResponseRandomDelayUpToTime(-2);

    QCOMPARE(errsim.responseDelayTime(), 0);
    QCOMPARE(errsim.responseRandomDelayUpToTime(), 0);
}

void TestModbusErrorSimulations::settingsReadStringBooleansAndClampNegativeDelays()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QSettings settings(dir.filePath(QStringLiteral("string-values.ini")), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("ModbusErrorSimulations"));
    settings.setValue(QStringLiteral("NoResponse"), QStringLiteral("true"));
    settings.setValue(QStringLiteral("ResponseIncorrectId"), QStringLiteral("1"));
    settings.setValue(QStringLiteral("ResponseIllegalFunction"), QStringLiteral("false"));
    settings.setValue(QStringLiteral("ResponseDeviceBusy"), QStringLiteral("0"));
    settings.setValue(QStringLiteral("ResponseIncorrectCrc"), QStringLiteral("true"));
    settings.setValue(QStringLiteral("ResponseDelay"), QStringLiteral("1"));
    settings.setValue(QStringLiteral("ResponseDelayTime"), QStringLiteral("-5"));
    settings.setValue(QStringLiteral("ResponseRandomDelay"), QStringLiteral("false"));
    settings.setValue(QStringLiteral("ResponseRandomDelayUpToTime"), QStringLiteral("-10"));
    settings.endGroup();

    ModbusErrorSimulations restored;
    settings >> restored;

    QVERIFY(restored.noResponse());
    QVERIFY(restored.responseIncorrectId());
    QVERIFY(!restored.responseIllegalFunction());
    QVERIFY(!restored.responseDeviceBusy());
    QVERIFY(restored.responseIncorrectCrc());
    QVERIFY(restored.responseDelay());
    QCOMPARE(restored.responseDelayTime(), 0);
    QVERIFY(!restored.responseRandomDelay());
    QCOMPARE(restored.responseRandomDelayUpToTime(), 0);
}

void TestModbusErrorSimulations::xmlWriterSerializesEveryAttribute()
{
    ModbusErrorSimulations errsim;
    errsim.setNoResponse(true);
    errsim.setResponseIncorrectId(true);
    errsim.setResponseIllegalFunction(true);
    errsim.setResponseDeviceBusy(true);
    errsim.setResponseIncorrectCrc(true);
    errsim.setResponseDelay(true);
    errsim.setResponseDelayTime(15);
    errsim.setResponseRandomDelay(true);
    errsim.setResponseRandomDelayUpToTime(25);

    QByteArray buffer;
    QXmlStreamWriter writer(&buffer);
    writer << errsim;

    QVERIFY(buffer.contains("NoResponse=\"true\""));
    QVERIFY(buffer.contains("ResponseIncorrectId=\"true\""));
    QVERIFY(buffer.contains("ResponseIllegalFunction=\"true\""));
    QVERIFY(buffer.contains("ResponseDeviceBusy=\"true\""));
    QVERIFY(buffer.contains("ResponseIncorrectCrc=\"true\""));
    QVERIFY(buffer.contains("ResponseDelay=\"true\""));
    QVERIFY(buffer.contains("ResponseRandomDelay=\"true\""));
    QVERIFY(buffer.contains("ResponseDelayTime=\"15\""));
    QVERIFY(buffer.contains("ResponseRandomDelayUpToTime=\"25\""));
}

void TestModbusErrorSimulations::settingsWriterSerializesFalseAndTrueValues()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString path = dir.filePath(QStringLiteral("all-values.ini"));
    {
        ModbusErrorSimulations source;
        QSettings settings(path, QSettings::IniFormat);
        settings << source;
        settings.sync();
    }

    {
        QSettings settings(path, QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("ModbusErrorSimulations"));
        QCOMPARE(settings.value(QStringLiteral("NoResponse")).toBool(), false);
        QCOMPARE(settings.value(QStringLiteral("ResponseIncorrectId")).toBool(), false);
        QCOMPARE(settings.value(QStringLiteral("ResponseIllegalFunction")).toBool(), false);
        QCOMPARE(settings.value(QStringLiteral("ResponseDeviceBusy")).toBool(), false);
        QCOMPARE(settings.value(QStringLiteral("ResponseIncorrectCrc")).toBool(), false);
        QCOMPARE(settings.value(QStringLiteral("ResponseDelay")).toBool(), false);
        QCOMPARE(settings.value(QStringLiteral("ResponseDelayTime")).toInt(), 0);
        QCOMPARE(settings.value(QStringLiteral("ResponseRandomDelay")).toBool(), false);
        QCOMPARE(settings.value(QStringLiteral("ResponseRandomDelayUpToTime")).toInt(), 1000);
        settings.endGroup();
    }

    {
        ModbusErrorSimulations source;
        source.setNoResponse(true);
        source.setResponseIncorrectId(true);
        source.setResponseIllegalFunction(true);
        source.setResponseDeviceBusy(true);
        source.setResponseIncorrectCrc(true);
        source.setResponseDelay(true);
        source.setResponseDelayTime(35);
        source.setResponseRandomDelay(true);
        source.setResponseRandomDelayUpToTime(45);

        QSettings settings(path, QSettings::IniFormat);
        settings << source;
        settings.sync();
    }

    ModbusErrorSimulations restored;
    QSettings settings(path, QSettings::IniFormat);
    settings >> restored;

    QVERIFY(restored.noResponse());
    QVERIFY(restored.responseIncorrectId());
    QVERIFY(restored.responseIllegalFunction());
    QVERIFY(restored.responseDeviceBusy());
    QVERIFY(restored.responseIncorrectCrc());
    QVERIFY(restored.responseDelay());
    QCOMPARE(restored.responseDelayTime(), 35);
    QVERIFY(restored.responseRandomDelay());
    QCOMPARE(restored.responseRandomDelayUpToTime(), 45);
}

void TestModbusErrorSimulations::xmlDelayBoundsAcceptZeroAndPositiveValues()
{
    for (const QByteArray xml : {
             QByteArrayLiteral("<ModbusErrorSimulations ResponseDelayTime=\"0\" ResponseRandomDelayUpToTime=\"0\"/>"),
             QByteArrayLiteral("<ModbusErrorSimulations ResponseDelayTime=\"1\" ResponseRandomDelayUpToTime=\"2\"/>")
         }) {
        ModbusErrorSimulations restored;
        restored.setResponseDelayTime(10);
        restored.setResponseRandomDelayUpToTime(20);

        QXmlStreamReader reader(xml);
        reader.readNextStartElement();
        reader >> restored;

        QVERIFY(restored.responseDelayTime() >= 0);
        QVERIFY(restored.responseRandomDelayUpToTime() >= 0);
    }
}

void TestModbusErrorSimulations::xmlBooleanAttributesAcceptEveryTrueToken()
{
    for (const QByteArray token : {QByteArrayLiteral("true"), QByteArrayLiteral("1"),
                                   QByteArrayLiteral("yes"), QByteArrayLiteral("on"),
                                   QByteArrayLiteral("TRUE"), QByteArrayLiteral("On")}) {
        const QByteArray xml = QByteArrayLiteral("<ModbusErrorSimulations ")
            + QByteArrayLiteral("NoResponse=\"") + token
            + QByteArrayLiteral("\" ResponseIncorrectId=\"") + token
            + QByteArrayLiteral("\" ResponseIllegalFunction=\"") + token
            + QByteArrayLiteral("\" ResponseDeviceBusy=\"") + token
            + QByteArrayLiteral("\" ResponseIncorrectCrc=\"") + token
            + QByteArrayLiteral("\" ResponseDelay=\"") + token
            + QByteArrayLiteral("\" ResponseRandomDelay=\"") + token
            + QByteArrayLiteral("\"/>");

        ModbusErrorSimulations restored;
        QXmlStreamReader reader(xml);
        reader.readNextStartElement();
        reader >> restored;

        QVERIFY(restored.noResponse());
        QVERIFY(restored.responseIncorrectId());
        QVERIFY(restored.responseIllegalFunction());
        QVERIFY(restored.responseDeviceBusy());
        QVERIFY(restored.responseIncorrectCrc());
        QVERIFY(restored.responseDelay());
        QVERIFY(restored.responseRandomDelay());
    }
}

QTEST_GUILESS_MAIN(TestModbusErrorSimulations)
#include "test_modbuserrorsimulations.moc"
