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
#include <QTcpServer>
#include <QTcpSocket>
#include <QDataStream>
#include <QUuid>

#include "scripting/server.h"

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
    void typedValuesRoundTrip_data();
    void typedValuesRoundTrip();
    void settingsReachBackingServer();
    void batchWritesAllRegisterKinds();
    void requestCallbacksDecodeAndRespond();
    void callbacksHandleAddressAndErrors();

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


/// \brief Exercises both address bases and byte and register orders through the script facade.
void TestServerApi::typedValuesRoundTrip_data()
{
    QTest::addColumn<bool>("baseOne");
    QTest::addColumn<bool>("byteSwap");
    QTest::addColumn<bool>("wordSwap");
    for (bool base : {false, true})
        for (bool bytes : {false, true})
            for (bool words : {false, true})
                QTest::newRow(qPrintable(QString("%1-%2-%3").arg(base).arg(bytes).arg(words)))
                    << base << bytes << words;
}

/// \brief Preserves signed, unsigned and floating values across the scripting boundary.
void TestServerApi::typedValuesRoundTrip()
{
    QFETCH(bool, baseOne);
    QFETCH(bool, byteSwap);
    QFETCH(bool, wordSwap);
    ModbusMultiServer backing;
    for (auto type : {QModbusDataUnit::HoldingRegisters, QModbusDataUnit::InputRegisters,
                      QModbusDataUnit::Coils, QModbusDataUnit::DiscreteInputs})
        backing.addUnitMap(QUuid::createUuid(), 1, type, 0, 32);
    ByteOrder order = byteSwap ? ByteOrder::Swapped : ByteOrder::Direct;
    QJSEngine engine;
    Server api(&backing, &order, AddressBase::Base0, &engine);
    api.setAddressBase(baseOne ? Address::Base::Base1 : Address::Base::Base0);
    QCOMPARE(api.addressBase(), baseOne ? Address::Base::Base1 : Address::Base::Base0);
    const quint16 address = baseOne ? 1 : 0;
    for (auto type : {Register::Type::Holding, Register::Type::Input}) {
        api.writeInt32(type, address, -1234567, wordSwap);
        QCOMPARE(api.readInt32(type, address, wordSwap), -1234567);
        api.writeUInt32(type, address, 4000000000u, wordSwap);
        QCOMPARE(api.readUInt32(type, address, wordSwap), 4000000000u);
        api.writeInt64(type, address, Q_INT64_C(-1234567890123), wordSwap);
        QCOMPARE(api.readInt64(type, address, wordSwap), Q_INT64_C(-1234567890123));
        api.writeUInt64(type, address, Q_UINT64_C(12345678901234567890), wordSwap);
        QCOMPARE(api.readUInt64(type, address, wordSwap), Q_UINT64_C(12345678901234567890));
        api.writeFloat(type, address, -12.5f, wordSwap);
        QCOMPARE(api.readFloat(type, address, wordSwap), -12.5f);
        api.writeDouble(type, address, 12345.625, wordSwap);
        QCOMPARE(api.readDouble(type, address, wordSwap), 12345.625);
    }
    api.writeInput(address, 0x1234);
    QCOMPARE(api.readInput(address), quint16(0x1234));
    api.writeCoil(address, true);
    QVERIFY(api.readCoil(address));
    api.writeDiscrete(address, true);
    QVERIFY(api.readDiscrete(address));
    api.writeAnsi(Register::Type::Holding, address, QStringLiteral("AB"), QStringLiteral("UTF-8"));
    QCOMPARE(api.readAnsi(Register::Type::Holding, address, QStringLiteral("UTF-8")), QStringLiteral("AB"));
}

/// \brief Applies script-visible error simulation settings to production definitions.
void TestServerApi::settingsReachBackingServer()
{
    ModbusMultiServer backing;
    backing.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 4);
    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server api(&backing, &order, AddressBase::Base0, &engine);
    api.setAddressSpace(Address::Space::Digits5);
    QCOMPARE(api.addressSpace(), Address::Space::Digits5);
    api.setAddressSpace(Address::Space::Digits6);
    QCOMPARE(api.addressSpace(), Address::Space::Digits6);
    for (bool enabled : {true, false}) {
        api.setUseGlobalUnitMap(enabled);
        QCOMPARE(api.useGlobalUnitMap(), enabled);
        api.setNoResponse(enabled);
        QCOMPARE(api.noResponse(), enabled);
        api.setResponseIncorrectId(enabled);
        QCOMPARE(api.responseIncorrectId(), enabled);
        api.setResponseIllegalFunction(enabled);
        QCOMPARE(api.responseIllegalFunction(), enabled);
        api.setResponseDeviceBusy(enabled);
        QCOMPARE(api.responseDeviceBusy(), enabled);
        api.setResponseIncorrectCrc(enabled);
        QCOMPARE(api.responseIncorrectCrc(), enabled);
        api.setResponseDelay(enabled);
        QCOMPARE(api.responseDelay(), enabled);
        api.setResponseRandomDelay(enabled);
        QCOMPARE(api.responseRandomDelay(), enabled);
    }
    api.setResponseDelayTime(123);
    QCOMPARE(api.responseDelayTime(), 123);
    api.setResponseRandomDelayUpToTime(456);
    QCOMPARE(api.responseRandomDelayUpToTime(), 456);
}

/// \brief Converts JavaScript arrays to register words and bit values and ignores empty arrays.
void TestServerApi::batchWritesAllRegisterKinds()
{
    ModbusMultiServer backing;
    for (auto type : {QModbusDataUnit::HoldingRegisters, QModbusDataUnit::InputRegisters,
                      QModbusDataUnit::Coils, QModbusDataUnit::DiscreteInputs})
        backing.addUnitMap(QUuid::createUuid(), 1, type, 0, 4);
    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server api(&backing, &order, AddressBase::Base1, &engine);
    const auto words = engine.evaluate("[42, 65535]");
    const auto bits = engine.evaluate("[true, false]");
    api.writeHoldings(1, words);
    api.writeInputs(1, words);
    api.writeCoils(1, bits);
    api.writeDiscretes(1, bits);
    QCOMPARE(api.readHolding(1), quint16(42));
    QCOMPARE(api.readInput(2), quint16(65535));
    QVERIFY(api.readCoil(1));
    QVERIFY(!api.readCoil(2));
    QVERIFY(api.readDiscrete(1));
    QVERIFY(!api.readDiscrete(2));
    QSignalSpy changes(&backing, &ModbusMultiServer::dataChanged);
    api.writeHoldings(1, engine.newArray());
    QCOMPARE(changes.count(), 0);
}


/// \brief Dispatches wire requests to JavaScript with decoded fields and custom responses.
void TestServerApi::requestCallbacksDecodeAndRespond()
{
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, 0))
        QSKIP("A local TCP listener is required.");
    const quint16 port = probe.serverPort();
    probe.close();
    ModbusMultiServer backing;
    backing.addDeviceId(1);
    backing.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 32);
    ConnectionDetails connection;
    connection.Type = ConnectionType::Tcp;
    connection.TcpParams.IPAddress = QStringLiteral("127.0.0.1");
    connection.TcpParams.ServicePort = port;
    backing.connectDevice(connection);
    QTRY_VERIFY(backing.isConnected());
    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server api(&backing, &order, AddressBase::Base0, &engine);
    engine.globalObject().setProperty("captured", engine.newObject());
    api.onRequest(engine.evaluate("(function(req) { captured = req; return {data:[42]}; })"));
    QVERIFY(QMetaObject::invokeMethod(&backing, [] {}, Qt::BlockingQueuedConnection));
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(socket.waitForConnected(1000));
    struct Case { QModbusPdu::FunctionCode code; const char* data; const char* property; int expected; };
    const QList<Case> cases = {
        {QModbusPdu::ReadCoils, "00030002", "count", 2},
        {QModbusPdu::ReadDiscreteInputs, "00030002", "address", 3},
        {QModbusPdu::ReadHoldingRegisters, "00030002", "count", 2},
        {QModbusPdu::ReadInputRegisters, "00030002", "address", 3},
        {QModbusPdu::WriteSingleCoil, "0003ff00", "address", 3},
        {QModbusPdu::WriteSingleRegister, "0003002a", "value", 42},
        {QModbusPdu::WriteMultipleCoils, "000300020102", "count", 2},
        {QModbusPdu::WriteMultipleRegisters, "0003000204002a0007", "count", 2},
        {QModbusPdu::MaskWriteRegister, "000300ff000f", "orMask", 15},
        {QModbusPdu::ReadWriteMultipleRegisters, "000300020005000204002a0007", "writeCount", 2},
        {QModbusPdu::Diagnostics, "0000002a", "subfunc", 0},
        {QModbusPdu::ReadFifoQueue, "0003", "address", 3}
    };
    quint16 transaction = 1;
    for (const auto& item : cases) {
        QByteArray frame;
        QDataStream stream(&frame, QIODevice::WriteOnly);
        QModbusRequest request(item.code, QByteArray::fromHex(item.data));
        stream << transaction++ << quint16(0) << quint16(request.size() + 1) << quint8(1) << request;
        QCOMPARE(socket.write(frame), qint64(frame.size()));
        QVERIFY(socket.flush());
        QTRY_VERIFY_WITH_TIMEOUT(socket.bytesAvailable() >= 9, 3000);
        const QByteArray response = socket.readAll();
        QCOMPARE(quint8(response.at(7)), quint8(item.code));
        QCOMPARE(quint8(response.at(8)), quint8(42));
        const auto captured = engine.globalObject().property("captured");
        QCOMPARE(captured.property("deviceId").toInt(), 1);
        QCOMPARE(captured.property("functionCode").toInt(), int(item.code));
        QCOMPARE(captured.property(item.property).toInt(), item.expected);
    }
    struct ReplyCase { QModbusPdu::FunctionCode code; const char* result; const char* expected; };
    const QList<ReplyCase> replies = {
        {QModbusPdu::ReadHoldingRegisters, "[4660, 22136]", "030412345678"},
        {QModbusPdu::ReadInputRegisters, "[1]", "04020001"},
        {QModbusPdu::ReadCoils, "[true, false, true]", "010105"},
        {QModbusPdu::ReadDiscreteInputs, "[false, true]", "020102"},
        {QModbusPdu::ReadHoldingRegisters, "({exception:2})", "8302"}
    };
    for (const auto& item : replies) {
        api.onRequest(engine.evaluate(QStringLiteral("(function(req) { return %1; })").arg(QLatin1String(item.result))));
        QVERIFY(QMetaObject::invokeMethod(&backing, [] {}, Qt::BlockingQueuedConnection));
        QByteArray frame;
        QDataStream stream(&frame, QIODevice::WriteOnly);
        QModbusRequest request(item.code, QByteArray::fromHex("00000002"));
        stream << transaction++ << quint16(0) << quint16(request.size() + 1) << quint8(1) << request;
        QCOMPARE(socket.write(frame), qint64(frame.size()));
        QVERIFY(socket.flush());
        const auto expected = QByteArray::fromHex(item.expected);
        QTRY_VERIFY_WITH_TIMEOUT(socket.bytesAvailable() >= 7 + expected.size(), 3000);
        QCOMPARE(socket.readAll().mid(7), expected);
    }
    for (const auto& result : {QStringLiteral("null"), QStringLiteral("undefined"),
                               QStringLiteral("(function(){ throw new Error('bad'); })()")}) {
        api.onRequest(engine.evaluate(QStringLiteral("(function(req) { return %1; })").arg(result)));
        QVERIFY(QMetaObject::invokeMethod(&backing, [] {}, Qt::BlockingQueuedConnection));
        QByteArray frame;
        QDataStream stream(&frame, QIODevice::WriteOnly);
        QModbusRequest request(QModbusPdu::ReadHoldingRegisters, QByteArray::fromHex("00000001"));
        stream << transaction++ << quint16(0) << quint16(request.size() + 1) << quint8(1) << request;
        QCOMPARE(socket.write(frame), qint64(frame.size()));
        QVERIFY(socket.flush());
        QTRY_VERIFY_WITH_TIMEOUT(socket.bytesAvailable() >= 11, 3000);
        QCOMPARE(socket.readAll().mid(7), QByteArray::fromHex("03020000"));
    }
    api.onRequest(QJSValue());
    backing.closeConnections();
}


/// \brief Routes change and error callbacks for the selected unit and address base.
void TestServerApi::callbacksHandleAddressAndErrors()
{
    ModbusMultiServer backing;
    backing.addDeviceId(1);
    backing.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 0, 4);
    ByteOrder order = ByteOrder::Direct;
    QJSEngine engine;
    Server api(&backing, &order, AddressBase::Base1, &engine);
    engine.globalObject().setProperty("changed", -1);
    engine.globalObject().setProperty("failure", QString());
    api.onChange(1, Register::Type::Holding, 1,
                 engine.evaluate("(function(value) { changed = value; })"));
    api.writeHolding(1, 23);
    QTRY_COMPARE(engine.globalObject().property("changed").toInt(), 23);
    QSignalSpy errors(&api, &Server::errorOccured);
    api.onChange(1, Register::Type::Holding, 1, QJSValue(42));
    QCOMPARE(errors.count(), 1);
    api.onError(1, engine.evaluate("(function(message) { failure = message; })"));
    emit backing.errorOccured(1, QStringLiteral("failure"));
    QTRY_COMPARE(engine.globalObject().property("failure").toString(), QStringLiteral("failure"));
    QCOMPARE(errors.count(), 1);
    api.onError(2, QJSValue(42));
    emit backing.errorOccured(2, QStringLiteral("unhandled"));
    QTRY_COMPARE(errors.count(), 2);
}

QTEST_MAIN(TestServerApi)
#include "test_server_api.moc"
