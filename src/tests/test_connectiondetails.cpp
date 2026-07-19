// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondetails.cpp
/// \brief Unit tests for connection parameter normalization and serialization.
///

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "connectiondetails.h"

namespace {

///
/// \brief Creates representative serial connection parameters.
/// \return The initialized parameters.
///
SerialConnectionParams serialParams()
{
    SerialConnectionParams params;
    params.PortName = QStringLiteral("COM7");
    params.BaudRate = QSerialPort::Baud57600;
    params.WordLength = QSerialPort::Data7;
    params.Parity = QSerialPort::EvenParity;
    params.StopBits = QSerialPort::TwoStop;
    params.FlowControl = QSerialPort::HardwareControl;
    params.SetDTR = false;
    params.SetRTS = true;
    return params;
}

///
/// \brief Serializes a value into a byte array.
/// \param value Value to serialize.
/// \return Serialized bytes.
///
template<typename T>
QByteArray toDataStream(const T &value)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << value;
    return bytes;
}

///
/// \brief Deserializes a value from a byte array.
/// \param bytes Serialized bytes.
/// \return Deserialized value.
///
template<typename T>
T fromDataStream(const QByteArray &bytes)
{
    T value;
    QByteArray copy = bytes;
    QDataStream stream(&copy, QIODevice::ReadOnly);
    stream >> value;
    return value;
}

///
/// \brief Serializes a value into an XML document.
/// \param value Value to serialize.
/// \return Serialized XML.
///
template<typename T>
QString toXml(const T &value)
{
    QString xml;
    QXmlStreamWriter writer(&xml);
    writer << value;
    return xml;
}

///
/// \brief Deserializes a value from an XML document.
/// \param xml Serialized XML.
/// \return Deserialized value.
///
template<typename T>
T fromXml(const QString &xml)
{
    T value;
    QXmlStreamReader reader(xml);
    reader.readNextStartElement();
    reader >> value;
    return value;
}

}

class TestConnectionDetails : public QObject
{
    Q_OBJECT

private slots:
    void normalizesTcpParameters();
    void normalizesSerialParameters();
    void streamsTcpParameters();
    void streamsSerialParameters();
    void storesTcpParametersInSettings();
    void storesSerialParametersInSettings();
    void readsSettingsDefaults();
    void serializesTcpParametersToXml();
    void serializesSerialParametersToXml();
    void streamsConnectionDetails();
    void storesConnectionDetailsInSettings();
    void serializesConnectionDetailsToXml();
    void readsChildlessConnectionWithoutSwallowingSiblings();
    void comparesConnectionsByType();
    void validatesClientInfo();
};

void TestConnectionDetails::normalizesTcpParameters()
{
    TcpConnectionParams params;
    params.IPAddress = QStringLiteral("invalid address");
    params.ServicePort = 0;
    params.normalize();

    QCOMPARE(params.IPAddress, QStringLiteral("0.0.0.0"));
    QCOMPARE(params.ServicePort, quint16(1));

    params.IPAddress = QStringLiteral("127.0.0.1");
    params.ServicePort = 1502;
    params.normalize();
    QCOMPARE(params.IPAddress, QStringLiteral("127.0.0.1"));
    QCOMPARE(params.ServicePort, quint16(1502));
}

void TestConnectionDetails::normalizesSerialParameters()
{
    SerialConnectionParams params;
    params.BaudRate = static_cast<QSerialPort::BaudRate>(1);
    params.WordLength = static_cast<QSerialPort::DataBits>(1);
    params.Parity = static_cast<QSerialPort::Parity>(99);
    params.FlowControl = static_cast<QSerialPort::FlowControl>(99);
    params.normalize();

    QCOMPARE(params.BaudRate, QSerialPort::Baud1200);
    QCOMPARE(params.WordLength, QSerialPort::Data5);
    QCOMPARE(params.Parity, QSerialPort::MarkParity);
    QCOMPARE(params.FlowControl, QSerialPort::SoftwareControl);
}

void TestConnectionDetails::streamsTcpParameters()
{
    TcpConnectionParams expected;
    expected.IPAddress = QStringLiteral("192.0.2.10");
    expected.ServicePort = 1502;

    QCOMPARE(fromDataStream<TcpConnectionParams>(toDataStream(expected)), expected);
}

void TestConnectionDetails::streamsSerialParameters()
{
    const auto expected = serialParams();
    QCOMPARE(fromDataStream<SerialConnectionParams>(toDataStream(expected)), expected);
}

void TestConnectionDetails::storesTcpParametersInSettings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("tcp.ini")), QSettings::IniFormat);
    TcpConnectionParams expected;
    expected.IPAddress = QStringLiteral("198.51.100.4");
    expected.ServicePort = 2502;
    settings << expected;

    TcpConnectionParams actual;
    settings >> actual;
    QCOMPARE(actual, expected);
}

void TestConnectionDetails::storesSerialParametersInSettings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("serial.ini")), QSettings::IniFormat);
    const auto expected = serialParams();
    settings << expected;

    SerialConnectionParams actual;
    settings >> actual;
    QCOMPARE(actual, expected);
}

void TestConnectionDetails::readsSettingsDefaults()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("defaults.ini")), QSettings::IniFormat);
    TcpConnectionParams tcp;
    SerialConnectionParams serial;
    settings >> tcp;
    settings >> serial;

    QCOMPARE(tcp, TcpConnectionParams());
    QCOMPARE(serial.PortName, QString());
    QCOMPARE(serial.BaudRate, QSerialPort::Baud9600);
    QCOMPARE(serial.WordLength, QSerialPort::Data8);
    QVERIFY(!serial.SetDTR);
    QVERIFY(!serial.SetRTS);
}

void TestConnectionDetails::serializesTcpParametersToXml()
{
    TcpConnectionParams expected;
    expected.IPAddress = QStringLiteral("203.0.113.8");
    expected.ServicePort = 3502;

    QCOMPARE(fromXml<TcpConnectionParams>(toXml(expected)), expected);
}

void TestConnectionDetails::serializesSerialParametersToXml()
{
    const auto expected = serialParams();
    QCOMPARE(fromXml<SerialConnectionParams>(toXml(expected)), expected);
}

void TestConnectionDetails::streamsConnectionDetails()
{
    ConnectionDetails expected;
    expected.Type = ConnectionType::Serial;
    expected.SerialParams = serialParams();

    QCOMPARE(fromDataStream<ConnectionDetails>(toDataStream(expected)), expected);
}

void TestConnectionDetails::storesConnectionDetailsInSettings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("connection.ini")), QSettings::IniFormat);
    ConnectionDetails expected;
    expected.Type = ConnectionType::RtuTcp;
    expected.TcpParams.IPAddress = QStringLiteral("127.0.0.1");
    expected.TcpParams.ServicePort = 4502;
    settings << expected;

    ConnectionDetails actual;
    settings >> actual;
    QCOMPARE(actual, expected);
}

void TestConnectionDetails::serializesConnectionDetailsToXml()
{
    ConnectionDetails tcp;
    tcp.Type = ConnectionType::Tcp;
    tcp.TcpParams.IPAddress = QStringLiteral("127.0.0.1");
    tcp.TcpParams.ServicePort = 5502;
    QCOMPARE(fromXml<ConnectionDetails>(toXml(tcp)), tcp);

    ConnectionDetails serial;
    serial.Type = ConnectionType::Serial;
    serial.SerialParams = serialParams();
    QCOMPARE(fromXml<ConnectionDetails>(toXml(serial)), serial);
}

void TestConnectionDetails::comparesConnectionsByType()
{
    ConnectionDetails tcp;
    ConnectionDetails sameTcp;
    QCOMPARE(tcp, sameTcp);

    ConnectionDetails rtuTcp;
    rtuTcp.Type = ConnectionType::RtuTcp;
    QVERIFY(!(tcp == rtuTcp));
    QVERIFY(rtuTcp == rtuTcp);

    ConnectionDetails serial;
    serial.Type = ConnectionType::Serial;
    serial.SerialParams = serialParams();
    QVERIFY(serial == serial);
    QVERIFY(!(serial == tcp));
}

void TestConnectionDetails::validatesClientInfo()
{
    ModbusClientInfo info;
    QVERIFY(!info.isValid());
    info.Address = QStringLiteral("192.0.2.20");
    info.Port = 1234;
    QVERIFY(info.isValid());

    ModbusClientInfo copy = info;
    QCOMPARE(copy, info);
    copy.Port = 4321;
    QVERIFY(!(copy == info));
}

/// \brief Verifies a ConnectionDetails element without a parameters child leaves the
/// reader on its own closing tag, so the elements following it are still parsed.
void TestConnectionDetails::readsChildlessConnectionWithoutSwallowingSiblings()
{
    QXmlStreamReader xml(QStringLiteral(
        "<Connections>"
        "<ConnectionDetails ConnectionType=\"Tcp\"/>"
        "<ConnectionDetails ConnectionType=\"Serial\">"
        "<SerialConnectionParams PortName=\"COM7\"/>"
        "</ConnectionDetails>"
        "</Connections>"));

    QList<ConnectionDetails> connections;
    QVERIFY(xml.readNextStartElement());
    while (xml.readNextStartElement()) {
        ConnectionDetails cd;
        xml >> cd;
        connections.append(cd);
    }

    QVERIFY(!xml.hasError());
    QCOMPARE(connections.size(), 2);
    QCOMPARE(connections.at(0).Type, ConnectionType::Tcp);
    QCOMPARE(connections.at(1).Type, ConnectionType::Serial);
    QCOMPARE(connections.at(1).SerialParams.PortName, QStringLiteral("COM7"));
}

QTEST_GUILESS_MAIN(TestConnectionDetails)
#include "test_connectiondetails.moc"
