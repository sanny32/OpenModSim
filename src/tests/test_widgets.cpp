// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_widgets.cpp
/// \brief Unit tests for reusable widgets and Modbus traffic rendering.
///

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QFile>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QMessageBox>
#include <QModbusRequest>
#include <QModbusResponse>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>

#include "application.h"
#include "datadelegate.h"
#include "outputdatawidget.h"
#include "scriptdocument.h"
#include "widgets/bytelisttextedit.h"
#include "widgets/coloredtoolbutton.h"
#include "widgets/findreplacebar.h"
#include "widgets/modbuslogwidget.h"
#include "widgets/modbusmessagewidget.h"
#include "widgets/numericlineedit.h"
#include "widgets/outputtrafficwidget.h"
#include "widgets/scripteditorwindow.h"
#include "widgets/statisticwidget.h"
#include "widgets/trafficlogwindow.h"

class TestWidgets : public QObject
{
    Q_OBJECT

private slots:
    void messageWidgetFormatsAllFunctions();
    void messageWidgetHandlesInvalidAndExceptionMessages();
    void logWidgetBuffersFiltersExportsAndPaints();
    void outputTrafficWidgetDisplaysAndPrintsMessages();
    void trafficLogWindowFiltersAndClearsMessages();
    void findReplaceBarEmitsActionsAndHandlesKeys();
    void numericLineEditSupportsEveryInputMode();
    void byteListEditorHandlesInputAndClipboard();
    void coloredToolButtonPaintsCustomColors();
    void statisticWidgetUpdatesCountersAndState();
    void scriptEditorWindowControlsDocumentLifecycle();
    void dataDelegatePaintsAddressesValuesAndComments();
};

namespace {

template<typename T>
T* child(QObject* parent, const char* name)
{
    auto* result = parent->findChild<T*>(QLatin1String(name));
    Q_ASSERT(result != nullptr);
    return result;
}

QByteArray messageData(QModbusPdu::FunctionCode function, bool request)
{
    switch (function) {
    case QModbusPdu::ReadCoils:
    case QModbusPdu::ReadDiscreteInputs:
        return request ? QByteArray::fromHex("00000008") : QByteArray::fromHex("0155");
    case QModbusPdu::ReadHoldingRegisters:
    case QModbusPdu::ReadInputRegisters:
        return request ? QByteArray::fromHex("00000002") : QByteArray::fromHex("0400010002");
    case QModbusPdu::WriteSingleCoil:
        return QByteArray::fromHex("0000ff00");
    case QModbusPdu::WriteSingleRegister:
        return QByteArray::fromHex("00001234");
    case QModbusPdu::ReadExceptionStatus:
        return request ? QByteArray() : QByteArray::fromHex("55");
    case QModbusPdu::Diagnostics:
        return QByteArray::fromHex("00001234");
    case QModbusPdu::GetCommEventCounter:
        return request ? QByteArray() : QByteArray::fromHex("00000001");
    case QModbusPdu::GetCommEventLog:
        return request ? QByteArray() : QByteArray::fromHex("0600000001000200");
    case QModbusPdu::WriteMultipleCoils:
        return request ? QByteArray::fromHex("000000080155") : QByteArray::fromHex("00000008");
    case QModbusPdu::WriteMultipleRegisters:
        return request ? QByteArray::fromHex("000000020400010002") : QByteArray::fromHex("00000002");
    case QModbusPdu::ReportServerId:
        return request ? QByteArray() : QByteArray::fromHex("020102");
    case QModbusPdu::ReadFileRecord:
    case QModbusPdu::WriteFileRecord:
        return QByteArray::fromHex("06050001000200");
    case QModbusPdu::MaskWriteRegister:
        return QByteArray::fromHex("000000ffff00");
    case QModbusPdu::ReadWriteMultipleRegisters:
        return request ? QByteArray::fromHex("00000002000000020400010002")
                       : QByteArray::fromHex("0400010002");
    case QModbusPdu::ReadFifoQueue:
        return request ? QByteArray::fromHex("0000") : QByteArray::fromHex("06000200010002");
    default:
        return QByteArray::fromHex("0102");
    }
}

QSharedPointer<const ModbusMessage> message(QModbusPdu::FunctionCode function,
                                            bool request,
                                            ModbusMessage::ProtocolType protocol = ModbusMessage::Rtu)
{
    const auto timestamp = QDateTime::fromString(QStringLiteral("2026-01-02T03:04:05.006"), Qt::ISODateWithMs);
    if (request) {
        return ModbusMessage::create(QModbusRequest(function, messageData(function, true)),
                                     protocol, 1, 7, timestamp, true);
    }
    return ModbusMessage::create(QModbusResponse(function, messageData(function, false)),
                                 protocol, 1, 7, timestamp, false);
}

QVector<QSharedPointer<const ModbusMessage>> allMessages()
{
    const QList<QModbusPdu::FunctionCode> functions = {
        QModbusPdu::ReadCoils, QModbusPdu::ReadDiscreteInputs,
        QModbusPdu::ReadHoldingRegisters, QModbusPdu::ReadInputRegisters,
        QModbusPdu::WriteSingleCoil, QModbusPdu::WriteSingleRegister,
        QModbusPdu::ReadExceptionStatus, QModbusPdu::Diagnostics,
        QModbusPdu::GetCommEventCounter, QModbusPdu::GetCommEventLog,
        QModbusPdu::WriteMultipleCoils, QModbusPdu::WriteMultipleRegisters,
        QModbusPdu::ReportServerId, QModbusPdu::ReadFileRecord,
        QModbusPdu::WriteFileRecord, QModbusPdu::MaskWriteRegister,
        QModbusPdu::ReadWriteMultipleRegisters, QModbusPdu::ReadFifoQueue,
        static_cast<QModbusPdu::FunctionCode>(0x2b)
    };
    QVector<QSharedPointer<const ModbusMessage>> result;
    for (const auto function : functions) {
        result.append(message(function, true));
        result.append(message(function, false));
        result.append(message(function, true, ModbusMessage::Tcp));
        result.append(message(function, false, ModbusMessage::Tcp));
    }
    return result;
}

void sendEvent(QWidget& widget, QEvent::Type type)
{
    QEvent event(type);
    QApplication::sendEvent(&widget, &event);
}

void answerNextMessageBox(QMessageBox::StandardButton button)
{
    QTimer::singleShot(0, [button] {
        if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
            box->button(button)->click();
    });
}

}

void TestWidgets::messageWidgetFormatsAllFunctions()
{
    ModbusMessageWidget widget;
    for (const auto& msg : allMessages()) {
        for (const auto type : {DataType::Hex, DataType::UInt16, DataType::Binary}) {
            widget.setDataType(type);
            widget.setByteOrder(ByteOrder::Swapped);
            widget.setShowLeadingZeros(false);
            widget.setShowTimestamp(false);
            widget.setModbusMessage(msg);
            QVERIFY(widget.count() > 0);
            widget.actions().at(0)->trigger();
            if (!msg->isRequest() && !msg->isException())
                widget.actions().at(1)->trigger();
        }
    }
    widget.setBackGroundColor(QColor(Qt::white));
    QCOMPARE(widget.backgroundColor(), QColor(Qt::white));
    sendEvent(widget, QEvent::LanguageChange);
    widget.clear();
    QCOMPARE(widget.count(), 0);
    QVERIFY(!widget.modbusMessage());
}

void TestWidgets::messageWidgetHandlesInvalidAndExceptionMessages()
{
    ModbusMessageWidget widget;
    const auto timestamp = QDateTime::currentDateTime();
    for (const bool request : {true, false}) {
        const auto invalid = ModbusMessage::create(QByteArray::fromHex("0103"), ModbusMessage::Rtu,
                                                   timestamp, request);
        widget.setModbusMessage(invalid);
        QVERIFY(widget.count() > 0);
    }
    const auto exception = ModbusMessage::create(
        QModbusExceptionResponse(QModbusPdu::ReadHoldingRegisters, QModbusExceptionResponse::IllegalDataAddress),
        ModbusMessage::Rtu, 1, 0, timestamp, false);
    widget.setModbusMessage(exception);
    QVERIFY(widget.count() > 0);
}

void TestWidgets::logWidgetBuffersFiltersExportsAndPaints()
{
    ModbusLogWidget widget;
    const auto messages = allMessages();
    widget.setRowLimit(20);
    widget.setAutoscroll(true);
    widget.setDataType(DataType::UInt16);
    widget.setShowLeadingZeros(false);
    widget.setBackGroundColor(QColor(Qt::white));
    widget.setState(LogViewState::Paused);
    widget.addItems(messages);
    QCOMPARE(widget.rowCount(), 0);
    widget.setState(LogViewState::Running);
    QVERIFY(widget.rowCount() > 0);
    QVERIFY(widget.rowCount() <= widget.rowLimit());
    widget.addItem(messages.first());
    QVERIFY(widget.rowCount() <= widget.rowLimit());
    QCOMPARE(widget.itemAt(widget.index(widget.rowCount() - 1)), messages.first());
    widget.addItems({});

    const auto index = widget.index(0);
    QVERIFY(index.isValid());
    QCOMPARE(index.data(Qt::UserRole).value<QSharedPointer<const ModbusMessage>>(), widget.itemAt(index));
    QVERIFY(!index.data(Qt::DisplayRole).toString().isEmpty());
    QVERIFY(!widget.itemAt(QModelIndex()));
    QVERIFY(!widget.model()->data(QModelIndex(), Qt::DisplayRole).isValid());
    widget.setCurrentIndex(index);
    for (auto* action : widget.actions())
        action->trigger();

    widget.resize(800, 400);
    widget.show();
    widget.grab();
    QPixmap image(900, 500);
    image.fill(Qt::white);
    QPainter painter(&image);
    widget.render(&painter);
    painter.end();

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("traffic.txt"));
    QVERIFY(widget.exportToTextFile(path));
    QFile output(path);
    QVERIFY(output.open(QIODevice::ReadOnly));
    QVERIFY(output.readAll().contains("[Rx]"));

    sendEvent(widget, QEvent::LanguageChange);
    widget.clear();
    QCOMPARE(widget.rowCount(), 0);
}

void TestWidgets::outputTrafficWidgetDisplaysAndPrintsMessages()
{
    OutputTrafficWidget widget;
    const auto messages = allMessages();
    widget.setDataType(DataType::Hex);
    widget.setLogViewLimit(100);
    widget.setAutosctollLogView(true);
    widget.setForegroundColor(QColor(Qt::black));
    widget.setBackgroundColor(QColor(Qt::white));
    widget.setFont(QFont(QStringLiteral("Courier"), 10));
    widget.updateTrafficBatch(messages);
    QCOMPARE(widget.rowCount(), messages.size());
    widget.updateTraffic(messages.first());
    QVERIFY(!widget.isLogEmpty());

    auto* log = child<ModbusLogWidget>(&widget, "logView");
    log->setCurrentIndex(log->index(0));
    log->selectionModel()->select(log->index(0), QItemSelectionModel::ClearAndSelect);

    QPixmap image(800, 100);
    image.fill(Qt::white);
    QPainter painter(&image);
    const int nextRow = widget.paint(QRect(0, 0, 800, 100), painter);
    QVERIFY(nextRow > 0);
    QVERIFY(nextRow < widget.rowCount());
    QCOMPARE(widget.paint(QRect(0, 0, 800, 10000), painter), widget.rowCount());
    painter.end();

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(widget.exportLogToTextFile(directory.filePath(QStringLiteral("traffic.txt"))));
    sendEvent(widget, QEvent::ApplicationPaletteChange);
    widget.clearLogView();
    QVERIFY(widget.isLogEmpty());
}

void TestWidgets::trafficLogWindowFiltersAndClearsMessages()
{
    ModbusMultiServer server;
    TrafficLogWindow window(server);
    const auto request = message(QModbusPdu::ReadHoldingRegisters, true);
    const auto response = message(QModbusPdu::ReadHoldingRegisters, false);
    emit server.request(request);
    emit server.response(request, response);

    auto* log = window.findChild<ModbusLogWidget*>();
    QVERIFY(log);
    QCOMPARE(log->rowCount(), 3);

    auto spinBoxes = window.findChildren<QSpinBox*>();
    QVERIFY(!spinBoxes.isEmpty());
    spinBoxes.first()->setValue(2);
    emit server.request(request);
    QCOMPARE(log->rowCount(), 3);
    spinBoxes.first()->setValue(1);

    const auto combos = window.findChildren<QComboBox*>();
    QCOMPARE(combos.size(), 2);
    combos[0]->setCurrentIndex(2);
    emit server.request(message(QModbusPdu::ReadCoils, true));
    QCOMPARE(log->rowCount(), 3);
    combos[0]->setCurrentIndex(3);
    emit server.request(request);
    QCOMPARE(log->rowCount(), 4);
    combos[1]->setCurrentIndex(0);

    const auto buttons = window.findChildren<QToolButton*>();
    QVERIFY(buttons.size() >= 2);
    buttons[0]->click();
    emit server.request(request);
    QCOMPARE(log->rowCount(), 4);
    emit server.response(request, response);
    QCOMPARE(log->rowCount(), 4);
    buttons[0]->click();

    answerNextMessageBox(QMessageBox::Yes);
    buttons[1]->click();
    QCoreApplication::processEvents();
    QCOMPARE(log->rowCount(), 0);
    buttons[1]->click();

    window.setDataType(DataType::UInt16);
    QCOMPARE(window.dataType(), DataType::UInt16);
    sendEvent(window, QEvent::LanguageChange);
}

void TestWidgets::findReplaceBarEmitsActionsAndHandlesKeys()
{
    QWidget parent;
    parent.resize(800, 600);
    FindReplaceBar bar(&parent);
    QSignalSpy nextSpy(&bar, &FindReplaceBar::findNext);
    QSignalSpy previousSpy(&bar, &FindReplaceBar::findPrevious);
    QSignalSpy replaceSpy(&bar, &FindReplaceBar::replaceRequested);
    QSignalSpy replaceAllSpy(&bar, &FindReplaceBar::replaceAllRequested);
    QSignalSpy closeSpy(&bar, &FindReplaceBar::closed);

    bar.showFind(QStringLiteral("value"));
    auto* search = child<QLineEdit>(&bar, "searchEdit");
    auto* replace = child<QLineEdit>(&bar, "replaceEdit");
    replace->setText(QStringLiteral("replacement"));
    emit search->textEdited(QStringLiteral("value"));
    child<QToolButton>(&bar, "matchCaseButton")->click();
    child<QToolButton>(&bar, "matchWordButton")->click();
    QVERIFY(bar.findFlags() & QTextDocument::FindCaseSensitively);
    QVERIFY(bar.findFlags() & QTextDocument::FindWholeWords);

    bar.showReplace();
    child<QToolButton>(&bar, "findButton")->click();
    child<QToolButton>(&bar, "replaceButton")->click();
    child<QToolButton>(&bar, "replaceAllButton")->click();
    QCOMPARE(nextSpy.count(), 1);
    QCOMPARE(replaceSpy.count(), 1);
    QCOMPARE(replaceAllSpy.count(), 1);

    QTest::keyClick(search, Qt::Key_Return, Qt::ShiftModifier);
    QCOMPARE(previousSpy.count(), 1);
    child<QToolButton>(&bar, "expandButton")->click();
    bar.setSearchOptionsVisible(false);
    QCOMPARE(bar.findFlags(), QTextDocument::FindFlags());
    bar.setSearchOptionsVisible(true);
    bar.setWindowedMode(true);
    bar.setUserMoved(true);
    bar.updatePosition();
    bar.setWindowedMode(false);
    bar.setReplaceEnabled(false);
    bar.showReplace(QStringLiteral("other"));
    QVERIFY(!bar.isReplaceEnabled());
    QTest::keyClick(replace, Qt::Key_Escape);
    QCOMPARE(closeSpy.count(), 1);
}

void TestWidgets::numericLineEditSupportsEveryInputMode()
{
    const QList<QPair<NumericLineEdit::InputMode, QString>> inputs = {
        {NumericLineEdit::Int32Mode, QStringLiteral("-123")},
        {NumericLineEdit::UInt32Mode, QStringLiteral("123")},
        {NumericLineEdit::HexMode, QStringLiteral("00ff")},
        {NumericLineEdit::BinaryMode, QStringLiteral("1010")},
        {NumericLineEdit::FloatMode, QStringLiteral("1.25")},
        {NumericLineEdit::DoubleMode, QStringLiteral("2.5")},
        {NumericLineEdit::Int64Mode, QStringLiteral("-123456789")},
        {NumericLineEdit::UInt64Mode, QStringLiteral("123456789")},
        {NumericLineEdit::AnsiMode, QStringLiteral("A")}
    };
    for (const auto& input : inputs) {
        NumericLineEdit editor(input.first);
        editor.setLeadingZeroes(true);
        editor.setHexButtonVisible(true);
        editor.setCodepage(QStringLiteral("UTF-8"));
        editor.setText(input.second);
        QTest::keyClick(&editor, Qt::Key_Return);
        editor.setHexView(true);
        editor.setHexView(false);
        editor.resize(200, 30);
        editor.show();
        QTest::keyClick(&editor, Qt::Key_Up);
        QTest::keyClick(&editor, Qt::Key_Down);
        editor.setFocus();
        QTest::keyClick(&editor, Qt::Key_Tab);
        editor.clear();
        editor.setAllowEmptyValue(true);
        editor.clear();
        QVERIFY(editor.isEmpty());
    }

    NumericLineEdit ranged;
    ranged.setInputRange(-10, 10);
    ranged.setValue(5);
    QCOMPARE(ranged.value<int>(), 5);
    QCOMPARE(ranged.range<int>().from(), -10);
    QCOMPARE(ranged.range<int>().to(), 10);
    ranged.setValue(100);
    QCOMPARE(ranged.value<int>(), 10);
}

void TestWidgets::byteListEditorHandlesInputAndClipboard()
{
    ByteListTextEdit editor(ByteListTextEdit::HexMode);
    editor.setValue(QByteArray::fromHex("0001ff"));
    QCOMPARE(editor.value(), QByteArray::fromHex("0001ff"));
    QVERIFY(!editor.text().isEmpty());
    editor.setInputMode(ByteListTextEdit::DecMode);
    editor.setValue(QByteArray::fromHex("0001ff"));
    editor.setText(QStringLiteral("0 1 255"));
    QCOMPARE(editor.value(), QByteArray::fromHex("0001ff"));
    QTest::keyClicks(&editor, " 2");
    QTest::keyClick(&editor, Qt::Key_Backspace);
    QApplication::clipboard()->setText(QStringLiteral("0 1 2"));
    QTest::keyClick(&editor, Qt::Key_V, Qt::ControlModifier);
    editor.show();
    editor.setFocus();
    QTest::keyClick(&editor, Qt::Key_Tab);
    editor.clear();
    QVERIFY(editor.isEmpty());
}

void TestWidgets::coloredToolButtonPaintsCustomColors()
{
    ColoredToolButton button;
    button.setText(QStringLiteral("Run"));
    button.setColors({"#112233", "#223344", "#334455", "#445566"});
    button.resize(100, 40);
    button.show();
    QVERIFY(!button.grab().isNull());
    button.setDown(true);
    QVERIFY(!button.grab().isNull());
    button.clearColors();
    QVERIFY(!button.grab().isNull());
}

void TestWidgets::statisticWidgetUpdatesCountersAndState()
{
    StatisticWidget widget;
    QSignalSpy resetSpy(&widget, &StatisticWidget::ctrsReseted);
    QSignalSpy stateSpy(&widget, &StatisticWidget::logStateChanged);
    widget.increaseRequests();
    widget.increaseResponses();
    widget.setCounters(10, 20);
    child<QPushButton>(&widget, "pushButtonResetCtrs")->click();
    QCOMPARE(resetSpy.count(), 1);
    for (const auto state : {LogViewState::Paused, LogViewState::Running, LogViewState::Unknown}) {
        widget.setLogState(state);
        QCOMPARE(widget.logState(), state);
    }
    widget.setLogState(LogViewState::Running);
    child<QPushButton>(&widget, "pushButtonPause")->click();
    QVERIFY(stateSpy.count() > 0);
    sendEvent(widget, QEvent::LanguageChange);
}

void TestWidgets::scriptEditorWindowControlsDocumentLifecycle()
{
    ScriptDocument document(QStringLiteral("Initial"));
    QSignalSpy nameSpy(&document, &ScriptDocument::nameChanged);
    document.setName(QStringLiteral("Script"));
    document.setScript(QStringLiteral("var value = 1;"));
    document.setByteOrder(ByteOrder::Swapped);
    document.setAddressBase(AddressBase::Base0);
    document.setCursorPosition(3);
    document.setScrollPosition(2);
    QCOMPARE(document.name(), QStringLiteral("Script"));
    QCOMPARE(document.script(), QStringLiteral("var value = 1;"));
    QCOMPARE(nameSpy.count(), 1);

    ModbusMultiServer server;
    ScriptEditorWindow window(&document, &server);
    QCOMPARE(window.document(), &document);
    QVERIFY(window.scriptControl());
    window.runScript(RunMode::Once, 0);
    QTest::qWait(20);
    window.stopScript();
    window.close();
}

void TestWidgets::dataDelegatePaintsAddressesValuesAndComments()
{
    QStandardItemModel model;
    const QList<QPair<QString, QColor>> rows = {
        {QStringLiteral("00001: 0x0001; comment"), QColor(Qt::yellow)},
        {QStringLiteral("plain value"), QColor()},
        {QStringLiteral("00002: 42"), QColor(Qt::green)}
    };
    for (int row = 0; row < rows.size(); ++row) {
        auto* item = new QStandardItem(rows[row].first);
        item->setData(rows[row].second, ColorRole);
        item->setData(row == 0, FindMatchRole);
        item->setData(row == 2, FindCurrentRole);
        model.appendRow(item);
    }

    QTableView view;
    auto* delegate = new DataDelegate(&view);
    delegate->setAddressColor(QColor(Qt::darkGray));
    delegate->setCommentColor(QColor(Qt::darkGreen));
    view.setItemDelegate(delegate);
    view.setModel(&model);
    view.resize(500, 200);
    view.show();
    view.setCurrentIndex(model.index(0, 0));
    QVERIFY(!view.grab().isNull());
    QCOMPARE(delegate->addressColor(), QColor(Qt::darkGray));
    QCOMPARE(delegate->commentColor(), QColor(Qt::darkGreen));
}

int main(int argc, char** argv)
{
    Application app(argc, argv);
    TestWidgets test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_widgets.moc"
