// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_server_api.cpp
/// \brief Guards the scripting API surface of the Server object against
/// breaking changes, and covers the 1.x compatibility overloads.
///

#include <QJSEngine>
#include <QMetaMethod>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "jsobjects/server.h"

class TestServerApi : public QObject
{
    Q_OBJECT

private slots:
    void onChangeKeepsLegacyThreeArgumentForm();
    void onErrorKeepsLegacyOneArgumentForm();
    void deviceIdPropertyIsExposed();
    void omittedDeviceIdFollowsServerDeviceId();
    void explicitDeviceIdWins();
    void invalidDeviceIdIsRejected();
    void batchWriteMethodsAreInvokable();
    void batchWriteAppliesWholeArray();
    void batchWriteRejectsNonArray();
    void unhandledErrorIsForwarded();
    void legacyOnChangeCallbackFiresFromScript();
    void invokableApiSurfaceIsStable();

private:
    static bool hasInvokable(const QByteArray& signature);
};

///
/// \brief Looks up a method by its normalised signature.
///
bool TestServerApi::hasInvokable(const QByteArray& signature)
{
    const auto& meta = Server::staticMetaObject;
    return meta.indexOfMethod(QMetaObject::normalizedSignature(signature.constData())) >= 0;
}

///
/// \brief Regression for issue #126: scripts written for 1.8 call
/// onChange(Register, address, callback) without a device id.
///
void TestServerApi::onChangeKeepsLegacyThreeArgumentForm()
{
    QVERIFY(hasInvokable("onChange(Register::Type,quint16,QJSValue)"));
    QVERIFY(hasInvokable("onChange(int,Register::Type,quint16,QJSValue)"));
}

void TestServerApi::onErrorKeepsLegacyOneArgumentForm()
{
    QVERIFY(hasInvokable("onError(QJSValue)"));
    QVERIFY(hasInvokable("onError(int,QJSValue)"));
}

void TestServerApi::deviceIdPropertyIsExposed()
{
    const auto& meta = Server::staticMetaObject;
    const int index = meta.indexOfProperty("deviceId");
    QVERIFY(index >= 0);
    QVERIFY(meta.property(index).isWritable());
}

///
/// \brief Calls that omit the device id must follow Server.deviceId rather than
/// silently targeting unit 1.
///
void TestServerApi::omittedDeviceIdFollowsServerDeviceId()
{
    ModbusMultiServer mbServer;
    mbServer.addDeviceId(7);
    mbServer.addUnitMap(QUuid::createUuid(), 7, QModbusDataUnit::HoldingRegisters, 0, 10);

    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server server(&mbServer, &order, AddressBase::Base0, &engine);

    server.setDeviceId(7);
    QCOMPARE(server.deviceId(), 7);

    server.writeHolding(1, 4242);

    const auto data = mbServer.data(7, QModbusDataUnit::HoldingRegisters, 1, 1);
    QCOMPARE(data.value(0), 4242);
    QCOMPARE(server.readHolding(1), 4242);
}

void TestServerApi::explicitDeviceIdWins()
{
    ModbusMultiServer mbServer;
    mbServer.addDeviceId(1);
    mbServer.addDeviceId(9);
    mbServer.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 10);
    mbServer.addUnitMap(QUuid::createUuid(), 9, QModbusDataUnit::HoldingRegisters, 0, 10);

    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server server(&mbServer, &order, AddressBase::Base0, &engine);

    server.setDeviceId(9);
    server.writeHolding(2, 111, 1);

    QCOMPARE(mbServer.data(1, QModbusDataUnit::HoldingRegisters, 2, 1).value(0), 111);
    QCOMPARE(mbServer.data(9, QModbusDataUnit::HoldingRegisters, 2, 1).value(0), 0);
}

void TestServerApi::invalidDeviceIdIsRejected()
{
    ModbusMultiServer mbServer;
    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server server(&mbServer, &order, AddressBase::Base0, &engine);

    QSignalSpy spy(&server, &Server::errorOccured);

    server.setDeviceId(300);
    QCOMPARE(server.deviceId(), 1);
    QCOMPARE(spy.count(), 1);
}

void TestServerApi::batchWriteMethodsAreInvokable()
{
    QVERIFY(hasInvokable("writeHoldings(quint16,QJSValue,int)"));
    QVERIFY(hasInvokable("writeInputs(quint16,QJSValue,int)"));
    QVERIFY(hasInvokable("writeCoils(quint16,QJSValue,int)"));
    QVERIFY(hasInvokable("writeDiscretes(quint16,QJSValue,int)"));
}

void TestServerApi::batchWriteAppliesWholeArray()
{
    ModbusMultiServer mbServer;
    mbServer.addDeviceId(1);
    mbServer.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::InputRegisters, 0, 100);

    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server server(&mbServer, &order, AddressBase::Base0, &engine);

    QJSValue values = engine.newArray(50);
    for (int i = 0; i < 50; ++i)
        values.setProperty(i, 500 + i);

    QSignalSpy spy(&mbServer, &ModbusMultiServer::dataChanged);
    server.writeInputs(0, values);

    QTRY_COMPARE(spy.count(), 1);

    const auto stored = mbServer.data(1, QModbusDataUnit::InputRegisters, 0, 50);
    QCOMPARE(static_cast<int>(stored.valueCount()), 50);
    QCOMPARE(stored.value(0), 500);
    QCOMPARE(stored.value(49), 549);
}

void TestServerApi::batchWriteRejectsNonArray()
{
    ModbusMultiServer mbServer;
    mbServer.addDeviceId(1);
    mbServer.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 10);

    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server server(&mbServer, &order, AddressBase::Base0, &engine);

    QSignalSpy spy(&server, &Server::errorOccured);
    server.writeHoldings(0, engine.toScriptValue(42));

    QCOMPARE(spy.count(), 1);
}

///
/// \brief Without a registered onError handler the message must still reach the
/// script console instead of being swallowed.
///
void TestServerApi::unhandledErrorIsForwarded()
{
    ModbusMultiServer mbServer;
    mbServer.addDeviceId(1);
    mbServer.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 10);

    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server server(&mbServer, &order, AddressBase::Base0, &engine);

    QSignalSpy spy(&server, &Server::errorOccured);
    server.writeHolding(0, 1, 200);

    QTRY_VERIFY(spy.count() >= 1);
}

///
/// \brief End-to-end regression for issue #126: the 1.x call shape must resolve
/// through QJSEngine and actually deliver value changes.
///
void TestServerApi::legacyOnChangeCallbackFiresFromScript()
{
    ModbusMultiServer mbServer;
    mbServer.addDeviceId(1);
    mbServer.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 10);

    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server server(&mbServer, &order, AddressBase::Base0, &engine);

    engine.globalObject().setProperty("Server", engine.newQObject(&server));
    engine.globalObject().setProperty("Register", engine.newQMetaObject(&Register::staticMetaObject));
    engine.globalObject().setProperty("observed", -1);

    const auto result = engine.evaluate(
        "Server.onChange(Register.Holding, 3, function(value) { observed = value; });");
    QVERIFY2(!result.isError(), qPrintable(result.toString()));

    server.writeHolding(3, 777);

    QTRY_COMPARE(engine.globalObject().property("observed").toInt(), 777);

    engine.globalObject().deleteProperty("Server");
    engine.globalObject().deleteProperty("Register");
}

///
/// \brief Golden list of the scripting entry points. Update it deliberately -
/// a change here breaks user scripts.
///
void TestServerApi::invokableApiSurfaceIsStable()
{
    const QList<QByteArray> expected = {
        "readHolding(quint16,int)",
        "writeHolding(quint16,quint16,int)",
        "readInput(quint16,int)",
        "writeInput(quint16,quint16,int)",
        "readDiscrete(quint16,int)",
        "writeDiscrete(quint16,bool,int)",
        "readCoil(quint16,int)",
        "writeCoil(quint16,bool,int)",
        "writeHoldings(quint16,QJSValue,int)",
        "writeInputs(quint16,QJSValue,int)",
        "writeCoils(quint16,QJSValue,int)",
        "writeDiscretes(quint16,QJSValue,int)",
        "readAnsi(Register::Type,quint16,QString,int)",
        "writeAnsi(Register::Type,quint16,QString,QString,int)",
        "readInt32(Register::Type,quint16,bool,int)",
        "writeInt32(Register::Type,quint16,qint32,bool,int)",
        "readUInt32(Register::Type,quint16,bool,int)",
        "writeUInt32(Register::Type,quint16,quint32,bool,int)",
        "readInt64(Register::Type,quint16,bool,int)",
        "writeInt64(Register::Type,quint16,qint64,bool,int)",
        "readUInt64(Register::Type,quint16,bool,int)",
        "writeUInt64(Register::Type,quint16,quint64,bool,int)",
        "readFloat(Register::Type,quint16,bool,int)",
        "writeFloat(Register::Type,quint16,float,bool,int)",
        "readDouble(Register::Type,quint16,bool,int)",
        "writeDouble(Register::Type,quint16,double,bool,int)",
        "onChange(int,Register::Type,quint16,QJSValue)",
        "onChange(Register::Type,quint16,QJSValue)",
        "onError(int,QJSValue)",
        "onError(QJSValue)",
        "onRequest(QJSValue)",
    };

    for (const auto& signature : expected)
        QVERIFY2(hasInvokable(signature), signature.constData());
}

QTEST_MAIN(TestServerApi)
#include "test_server_api.moc"
