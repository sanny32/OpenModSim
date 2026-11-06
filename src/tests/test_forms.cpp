// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_forms.cpp
/// \brief Integration tests for form persistence, traffic filtering and editing.
///

#include <memory>
#include <QAction>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QMenu>
#include <QListWidget>
#include <QHelpEvent>
#include <QStyledItemDelegate>
#include <QToolTip>
#include <QPlainTextEdit>
#include <QPrinter>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTableView>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include "style/qlementineappstyle.h"
#include "style/macappstyle.h"
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include "application.h"
#include "apppreferences.h"
#include "applogger.h"
#include "widgets/applogoutput.h"
#include "appproject.h"
#include "datasimulator.h"
#include "formdataview.h"
#include "formdatamapview.h"
#include "formscriptview.h"
#include "formtrafficview.h"
#include "mainwindow.h"
#include "htmldelegate.h"
#include "scriptdocument.h"
#include "menuconnect.h"
#include "widgets/mainstatusbar.h"
#include "widgets/helpwidget.h"
#include "legacyprojectloader.h"
#include "widgets/jscodeeditor.h"
#include "widgets/mdiareaex.h"
#include "widgets/projecttreewidget.h"
#include "widgets/outputtrafficwidget.h"
#include "widgets/outputdatawidget.h"
#include "widgets/numericlineedit.h"
#include "dialogs/dialogwriteregister.h"
#include "dialogs/dialogwritestatusregister.h"
#include "widgets/findreplacebar.h"

Q_DECLARE_METATYPE(ProjectFormType)

namespace {
void selectNextFile(const QString& path);

struct Fixture
{
    QTemporaryDir Profile;
    MainWindow Host{Profile.path(), false};
    MdiAreaEx Mdi;
    ModbusMultiServer Server;
    DataSimulator Simulator;
    ProjectTreeWidget Tree;
    AppProject Project{&Mdi, Server, &Simulator, &Tree, &Host};

    template<class T> T* create(ProjectFormKind kind)
    {
        return qobject_cast<T*>(Project.createMdiChild(kind));
    }
};

/// \brief Retrieves a named control, failing immediately if the UI contract changes.
template<class T> T* control(QObject* parent, const char* name)
{
    auto* result = parent->findChild<T*>(QLatin1String(name));
    Q_ASSERT(result);
    return result;
}

/// \brief Answers modal confirmation through its standard button semantics.
void confirm(QMessageBox::StandardButton answer)
{
    QTimer::singleShot(0, [answer] {
        if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
            box->button(answer)->click();
    });
}

/// \brief Delivers language changes without relying on installed translation files.
void translate(QWidget* widget)
{
    QEvent event(QEvent::LanguageChange);
    QApplication::sendEvent(widget, &event);
}

/// \brief Serializes form settings through the public XML facade.
template<class T> QByteArray xmlFor(T* form)
{
    QByteArray data;
    QXmlStreamWriter xml(&data);
    form->saveXml(xml);
    return data;
}

/// \brief Restores a form from a complete XML element.
template<class T> bool restoreXml(T* form, const QByteArray& data)
{
    QXmlStreamReader xml(data);
    if (!xml.readNextStartElement())
        return false;
    form->loadXml(xml);
    return !xml.hasError();
}
}

class TestForms : public QObject
{
    Q_OBJECT
private slots:
    void trafficFiltersBufferedHistory();
    void trafficSettingsRoundTrip();
    void dataSettingsRoundTrip();
    void scriptSettingsAndEditing();
    void mapEditingAndPersistence();
    void printsAllFormKinds();
    void mainWindowPropagatesPreferences();
    void restoresLegacyDataView();
    void editsMapCellsWithDelegates();
    void searchesAndReplacesScriptText();
    void preferencesRoundTrip();
    void applicationLogFiltersAndClears();
    void dataOutputFindsAndFormatsValues();
    void mdiPanelsPreserveWindowsAndOptions();
    void treeTracksFormLifecycle();
    void themedControlsRenderInBothThemes();
    void contextMenusApplyActions();
    void mainWindowActionsAndScriptCommands();
    void mapAutomaticallyAddsRequestedRegisters();
    void styleColorsCoverInteractionStates();
    void logsServerAndPreferenceEvents();
    void mapDragAndBackendUpdates();
    void consoleFiltersAndCopiesMessages();
    void restoresLegacyDataEmbeddedInXml();
    void splitViewsKeepScriptAndTrafficState();
    void tabBarTracksAndDragsWindows();
    void mainWindowSavesImportsAndConfirms();
    void connectionMenusAndStatusFollowServer();
    void scriptTreeContextCommands();
    void scriptDocumentXmlRoundTrip();
    void htmlDelegateRendersSelections();
    void helpSearchNavigatesAndCloses();
    void dataViewWritesAndSimulatesValues();
};

/// \brief Rebuilds retained traffic when filters change and preserves history while paused.
void TestForms::trafficFiltersBufferedHistory()
{
    Fixture f;
    auto* form = f.create<FormTrafficView>(ProjectFormKind::Traffic);
    QVERIFY(form);
    f.Mdi.show();
    form->show();
    auto dd = form->displayDefinition();
    dd.LogViewLimit = 100;
    form->setDisplayDefinition(dd);
    form->setLogViewState(LogViewState::Running);
    const ConnectionDetails connection;
    const auto now = QDateTime::currentDateTime();
    const auto request = ModbusMessage::create(QModbusRequest(QModbusPdu::ReadHoldingRegisters,
        QByteArray::fromHex("00000001")), ModbusMessage::Tcp, 1, 1, now, true);
    const auto response = ModbusMessage::create(QModbusResponse(QModbusPdu::ReadHoldingRegisters,
        QByteArray::fromHex("02002a")), ModbusMessage::Tcp, 1, 1, now, false);
    const auto error = ModbusMessage::create(QModbusExceptionResponse(QModbusPdu::ReadHoldingRegisters,
        QModbusExceptionResponse::IllegalDataAddress), ModbusMessage::Tcp, 1, 1, now, false);
    auto* output = form->findChild<OutputTrafficWidget*>();
    QVERIFY(output);
    emit f.Server.requestOnConnection(connection, request);
    emit f.Server.responseOnConnection(connection, request, response);
    emit f.Server.responseOnConnection(connection, request, error);
    QTRY_COMPARE(output->rowCount(), 3);
    dd.ExceptionsOnly = true;
    form->setDisplayDefinition(dd);
    QCOMPARE(output->rowCount(), 1);
    dd.ExceptionsOnly = false;
    dd.UnitFilter = 2;
    form->setDisplayDefinition(dd);
    QVERIFY(form->isLogEmpty());
    dd.UnitFilter = -1;
    dd.FunctionCodeFilter = 1;
    form->setDisplayDefinition(dd);
    QVERIFY(form->isLogEmpty());
    dd.FunctionCodeFilter = -1;
    form->setDisplayDefinition(dd);
    QCOMPARE(output->rowCount(), 3);
    form->setLogViewState(LogViewState::Paused);
    emit f.Server.requestOnConnection(connection, request);
    QCOMPARE(output->rowCount(), 3);
    form->setLogViewState(LogViewState::Running);
    QTRY_COMPARE(output->rowCount(), 4);
    dd.LogViewLimit = 4;
    form->setDisplayDefinition(dd);
    for (int i = 0; i < 20; ++i)
        emit f.Server.requestOnConnection(connection, request);
    QTRY_VERIFY(output->rowCount() <= 4);
    form->setHexView(false);
    QVERIFY(!form->hexView());
    translate(form);
    confirm(QMessageBox::No);
    control<QAction>(form, "actionClearTraffic")->trigger();
    QVERIFY(!form->isLogEmpty());
    confirm(QMessageBox::Yes);
    control<QAction>(form, "actionClearTraffic")->trigger();
    QVERIFY(form->isLogEmpty());
}

/// \brief Restores traffic filters, colors and fonts through both persistence formats.
void TestForms::trafficSettingsRoundTrip()
{
    Fixture f;
    auto* source = f.create<FormTrafficView>(ProjectFormKind::Traffic);
    auto* restored = f.create<FormTrafficView>(ProjectFormKind::Traffic);
    auto dd = source->displayDefinition();
    dd.FormName = QStringLiteral("Filtered traffic");
    dd.UnitFilter = 7;
    dd.FunctionCodeFilter = 3;
    dd.LogViewLimit = 500;
    dd.Autoscroll = true;
    dd.ExceptionsOnly = true;
    source->setDisplayDefinition(dd);
    source->setBackgroundColor(QColor("#234567"));
    source->setForegroundColor(QColor("#abcdef"));
    source->setFont(QFont(QStringLiteral("Monaco"), 13));
    QVERIFY(restoreXml(restored, xmlFor(source)));
    QCOMPARE(restored->displayDefinition(), source->displayDefinition());
    QCOMPARE(restored->backgroundColor(), source->backgroundColor());
    QCOMPARE(restored->foregroundColor(), source->foregroundColor());
    QCOMPARE(restored->font().family(), source->font().family());
    QCOMPARE(restored->font().pointSize(), source->font().pointSize());
    QSettings settings(f.Profile.filePath("traffic.ini"), QSettings::IniFormat);
    source->saveSettings(settings);
    restored->setDisplayDefinition(TrafficViewDefinitions());
    restored->loadSettings(settings);
    QCOMPARE(restored->displayDefinition(), source->displayDefinition());
    restored->linkTo(source);
    restored->connectEditSlots();
    restored->disconnectEditSlots();
    QSignalSpy closing(restored, &FormTrafficView::closing);
    restored->close();
    QCOMPARE(closing.count(), 1);
}

/// \brief Restores data view formatting, addresses and custom palette through both formats.
void TestForms::dataSettingsRoundTrip()
{
    Fixture f;
    auto* source = f.create<FormDataView>(ProjectFormKind::Data);
    auto* restored = f.create<FormDataView>(ProjectFormKind::Data);
    auto dd = source->displayDefinition();
    dd.FormName = QStringLiteral("Input data");
    dd.DeviceId = 7;
    dd.PointType = QModbusDataUnit::InputRegisters;
    dd.PointAddress = 12;
    dd.Length = 16;
    dd.DataViewColumnsDistance = 4;
    dd.LeadingZeros = false;
    source->setDisplayDefinition(dd);
    source->setDataType(DataType::Float32);
    source->setRegisterOrder(RegisterOrder::LSRF);
    source->setByteOrder(ByteOrder::Swapped);
    source->setCodepage(QStringLiteral("UTF-8"));
    source->setFont(QFont(QStringLiteral("Monaco"), 13));
    source->setBackgroundColor(QColor("#234567"));
    source->setForegroundColor(QColor("#abcdef"));
    source->setAddressColor(QColor("#123456"));
    source->setCommentColor(QColor("#654321"));
    source->setZoomPercent(125);
    source->setDescription(7, dd.PointType, 12, QStringLiteral("Pressure"));
    source->setColor(7, dd.PointType, 12, QColor(Qt::yellow));
    QVERIFY(restoreXml(restored, xmlFor(source)));
    QCOMPARE(restored->displayDefinition(), source->displayDefinition());
    QCOMPARE(restored->dataType(), source->dataType());
    QCOMPARE(restored->registerOrder(), source->registerOrder());
    QCOMPARE(restored->byteOrder(), source->byteOrder());
    QCOMPARE(restored->codepage(), source->codepage());
    QSettings settings(f.Profile.filePath("data.ini"), QSettings::IniFormat);
    source->saveSettings(settings);
    restored->loadSettings(settings);
    QCOMPARE(restored->displayDefinition(), source->displayDefinition());
    source->setAddressBase(AddressBase::Base0);
    QVERIFY(source->zeroBasedAddress());
    source->setAddressBase(AddressBase::Base1);
    QVERIFY(!source->zeroBasedAddress());
    source->linkTo(restored);
    auto linkedDefinition = source->displayDefinition();
    linkedDefinition.DeviceId = 5;
    linkedDefinition.PointAddress = 20;
    linkedDefinition.Length = 12;
    source->setDisplayDefinition(linkedDefinition);
    QCOMPARE(restored->displayDefinition().DeviceId, quint8(5));
    QCOMPARE(restored->displayDefinition().PointAddress, quint16(20));
    QCOMPARE(restored->displayDefinition().Length, quint16(12));
    source->setDescription(5, linkedDefinition.PointType, 20, QStringLiteral("Linked"));
    source->setColor(5, linkedDefinition.PointType, 20, QColor(Qt::yellow));
    source->setForegroundColor(QColor(Qt::red));
    QCOMPARE(restored->foregroundColor(), source->foregroundColor());
    source->setAddressColor(QColor(Qt::blue));
    QCOMPARE(restored->addressColor(), source->addressColor());
    source->setCommentColor(QColor(Qt::green));
    QCOMPARE(restored->commentColor(), source->commentColor());
    source->setBackgroundColor(QColor(Qt::cyan));
    QCOMPARE(restored->backgroundColor(), source->backgroundColor());
    source->showFind();
    translate(source);
}

/// \brief Persists scripts and exercises document edits and linked run state.
void TestForms::scriptSettingsAndEditing()
{
    Fixture f;
    auto* source = f.create<FormScriptView>(ProjectFormKind::Script);
    auto* restored = f.create<FormScriptView>(ProjectFormKind::Script);
    source->setFormName(QStringLiteral("Calculation"));
    source->setScript(QStringLiteral("var pressure = 42;\nconsole.log(pressure);"));
    source->setScriptCursorPosition(4);
    source->setScriptScrollPosition(0);
    source->setBackgroundColor(QColor("#234567"));
    source->setForegroundColor(QColor("#abcdef"));
    source->setFont(QFont(QStringLiteral("Monaco"), 13));
    source->setZoomPercent(125);
    QCOMPARE(source->zoomPercent(), 100);
    QVERIFY(restoreXml(restored, xmlFor(source)));
    QCOMPARE(restored->script(), source->script());
    QCOMPARE(restored->definitions(), source->definitions());
    QCOMPARE(restored->font().family(), source->font().family());
    QCOMPARE(restored->font().pointSize(), source->font().pointSize());
    QSettings settings(f.Profile.filePath("script.ini"), QSettings::IniFormat);
    source->saveSettings(settings);
    restored->loadSettings(settings);
    QCOMPARE(restored->script(), source->script());
    source->enableAutoComplete(false);
    QVERIFY(!source->isAutoCompleteEnabled());
    source->enableAutoComplete(true);
    QVERIFY(source->isAutoCompleteEnabled());
    QVERIFY(source->canRunScript());
    source->setPeerRunning(true);
    QVERIFY(!source->canRunScript());
    QVERIFY(source->canStopScript());
    source->setPeerRunning(false);
    source->connectEditSlots();
    source->disconnectEditSlots();
    source->showFind();
    source->showReplace();
    auto* editor = source->findChild<QPlainTextEdit*>();
    QVERIFY(editor);
    editor->moveCursor(QTextCursor::End);
    editor->insertPlainText(QStringLiteral("\nvar extra = 1;"));
    QVERIFY(source->canUndo());
    source->scriptControl()->undo();
    QVERIFY(source->canRedo());
    source->scriptControl()->redo();
    QApplication::clipboard()->setText(QStringLiteral("copy"));
    QVERIFY(source->canPaste());
    source->scriptControl()->search(QStringLiteral("pressure"));
    QCOMPARE(source->searchText(), QStringLiteral("pressure"));
    restored->linkRunStopTo(source);
    QSignalSpy stopped(source, &FormScriptView::scriptStopped);
    source->runScript();
    source->stopScript();
    QTRY_VERIFY(stopped.count() > 0);
    translate(source);
}

/// \brief Adds, inserts and deletes rows and keeps map metadata across XML restoration.
void TestForms::mapEditingAndPersistence()
{
    Fixture f;
    auto* source = f.create<FormDataMapView>(ProjectFormKind::DataMap);
    auto* restored = f.create<FormDataMapView>(ProjectFormKind::DataMap);
    auto* model = source->findChild<DataMapDataModel*>();
    auto* table = control<QTableView>(source, "tableView");
    QVERIFY(model);
    QVERIFY(source->isEmpty());
    control<QAction>(source, "actionAdd")->trigger();
    QCOMPARE(model->rowCount(), 1);
    table->selectRow(0);
    control<QAction>(source, "actionInsert")->trigger();
    QCOMPARE(model->rowCount(), 2);
    table->selectRow(0);
    control<QAction>(source, "actionDelete")->trigger();
    QCOMPARE(model->rowCount(), 1);
    source->setAddressBase(AddressBase::Base0);
    source->setHexView(true);
    source->setAutoAddOnRequest(true);
    source->setAutoRequestMap(true);
    QVERIFY(restoreXml(restored, xmlFor(source)));
    QCOMPARE(restored->findChild<DataMapDataModel*>()->rowCount(), 1);
    QVERIFY(restored->zeroBasedAddress());
    QVERIFY(restored->hexView());
    QVERIFY(restored->autoAddOnRequest());
    QVERIFY(restored->isAutoRequestMap());
    source->resize(900, 500);
    source->show();
    QVERIFY(!source->grab().isNull());
    translate(source);
    confirm(QMessageBox::No);
    control<QAction>(source, "actionClear")->trigger();
    QCOMPARE(model->rowCount(), 1);
    confirm(QMessageBox::Yes);
    control<QAction>(source, "actionClear")->trigger();
    QVERIFY(source->isEmpty());
}

/// \brief Produces printable output for each form without requiring a physical printer.
void TestForms::printsAllFormKinds()
{
    Fixture f;
    auto* data = f.create<FormDataView>(ProjectFormKind::Data);
    auto* map = f.create<FormDataMapView>(ProjectFormKind::DataMap);
    auto* script = f.create<FormScriptView>(ProjectFormKind::Script);
    auto* traffic = f.create<FormTrafficView>(ProjectFormKind::Traffic);
    script->setScript(QStringLiteral("var value = 42;\nconsole.log(value);"));
    control<QAction>(map, "actionAdd")->trigger();
    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    const QString path = f.Profile.filePath("print.pdf");
    printer.setOutputFileName(path);
    data->print(&printer);
    QVERIFY(QFile(path).size() > 0);
    map->print(&printer);
    QVERIFY(QFile(path).size() > 0);
    script->print(&printer);
    QVERIFY(QFile(path).size() > 0);
    traffic->print(&printer);
    QVERIFY(QFile(path).size() > 0);
    map->print(nullptr);
    traffic->print(nullptr);
}

/// \brief Applies global preferences through the same actions used by the main toolbar.
void TestForms::mainWindowPropagatesPreferences()
{
    QTemporaryDir profile;
    MainWindow host(profile.path(), false);
    for (const auto* name : {"actionNewDataView", "actionNewScript", "actionNewDataMapView", "actionNewTrafficView"})
        control<QAction>(&host, name)->trigger();
    auto* data = host.findChild<FormDataView*>();
    auto* script = host.findChild<FormScriptView*>();
    auto* map = host.findChild<FormDataMapView*>();
    auto* traffic = host.findChild<FormTrafficView*>();
    QVERIFY(data && script && map && traffic);
    host.applyGlobalAddressBase(AddressBase::Base0, false);
    QVERIFY(data->zeroBasedAddress());
    QVERIFY(map->zeroBasedAddress());
    host.applyGlobalHexView(true, false);
    QVERIFY(map->hexView());
    QVERIFY(traffic->hexView());
    host.applyGlobalHexView(false, false);
    QVERIFY(!map->hexView());
    host.applyAutoComplete(false);
    QVERIFY(!script->isAutoCompleteEnabled());
    host.applyConsoleMaxLines(100);
    host.applyFont(QFont(QStringLiteral("Monaco"), 12));
    host.applyScriptFont(QFont(QStringLiteral("Monaco"), 14));
    host.applyZoom(125);
    host.applyColors(QColor("#234567"), QColor("#abcdef"), QColor(Qt::red), QColor(Qt::green));
    QCOMPARE(data->backgroundColor(), QColor("#234567"));
    host.applyThemeColors();
    host.applyCheckForUpdates(false);
    host.showOutputConsole();
    host.setViewMode(QMdiArea::SubWindowView);
    host.setViewMode(QMdiArea::TabbedView);
    translate(&host);
    const QString path = profile.filePath("project.omsim");
    QVERIFY(host.saveProject(path));
    QVERIFY(host.closeProject());
    QVERIFY(host.loadProject(path));
    QVERIFY(host.closeProject());
}


/// \brief Loads values, appearance and embedded script from a legacy project form.
void TestForms::restoresLegacyDataView()
{
    Fixture f;
    auto* form = f.create<FormDataView>(ProjectFormKind::Data);
    QVERIFY(form);
    const QByteArray text = R"xml(
<FormModSim Title="Legacy Registers" DataDisplayMode="UInt32" RegisterOrder="LSRF" Codepage="UTF-8" ByteOrder="Swapped">
  <Window Left="20" Top="30" Width="450" Height="350" Maximized="false" Minimized="false"/>
  <DisplayDefinition DeviceId="7" PointType="4" PointAddress="10" Length="3" DataViewColumnsDistance="4" LeadingZeros="false" ZeroBasedAddress="true"/>
  <AddressDescriptionMap><Description Address="11">pressure</Description></AddressDescriptionMap>
  <AddressColorMap><Color Address="12" Value="#112233"/></AddressColorMap>
  <ModbusDataUnit><Value Address="10">100</Value><Value Address="12">300</Value></ModbusDataUnit>
  <JScriptControl><Script>var x = 1;</Script></JScriptControl>
  <ScriptSettings Mode="Once" Interval="1000" RunOnStartup="false"/>
  <Unknown/>
</FormModSim>)xml";
    QXmlStreamReader xml(text);
    QVERIFY(xml.readNextStartElement());
    LegacyProjectLoader::loadDataView(xml, *form, f.Project);
    QVERIFY(!xml.hasError());
    QCOMPARE(form->displayDefinition().DeviceId, quint8(7));
    QCOMPARE(form->displayDefinition().PointAddress, quint16(10));
    QCOMPARE(form->displayDefinition().Length, quint16(3));
    QCOMPARE(form->dataType(), DataType::UInt32);
    QCOMPARE(form->registerOrder(), RegisterOrder::LSRF);
    QCOMPARE(form->byteOrder(), ByteOrder::Swapped);
    QVERIFY(form->zeroBasedAddress());
    QCOMPARE(f.Server.data(7, QModbusDataUnit::HoldingRegisters, 10, 3).value(0), quint16(100));
    QCOMPARE(f.Project.forms(ProjectFormKind::Script).size(), 1);
}

/// \brief Exercises the map's editors across register formats and checks model commits.
void TestForms::editsMapCellsWithDelegates()
{
    Fixture f;
    auto* form = f.create<FormDataMapView>(ProjectFormKind::DataMap);
    auto* source = form->findChild<DataMapDataModel*>();
    auto* table = control<QTableView>(form, "tableView");
    QVERIFY(source);
    DataMapEntry entry;
    source->addEntry({1, QModbusDataUnit::HoldingRegisters, 0}, entry);
    auto* model = table->model();
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 120, 30);
    for (int column : {ColUnit, ColType, ColAddress, ColDataType, ColRegistryOrder, ColByteOrder, ColValue}) {
        const QModelIndex index = model->index(0, column);
        auto* delegate = qobject_cast<QStyledItemDelegate*>(table->itemDelegateForColumn(column));
        QVERIFY(delegate);
        QWidget* editor = delegate->createEditor(table, option, index);
        QVERIFY(editor);
        delegate->setEditorData(editor, index);
        delegate->updateEditorGeometry(editor, option, index);
        delegate->setModelData(editor, model, index);
        delete editor;
    }
    auto* delegate = qobject_cast<QStyledItemDelegate*>(table->itemDelegateForColumn(ColValue));
    QVERIFY(delegate);
    for (auto type : {DataType::Binary, DataType::Hex, DataType::Ansi, DataType::Int16, DataType::UInt16,
                      DataType::Int32, DataType::UInt32, DataType::Int64, DataType::UInt64,
                      DataType::Float32, DataType::Float64}) {
        QVERIFY(source->setData(source->index(0, ColDataType), enumToString(type), Qt::DisplayRole));
        const auto index = model->index(0, ColValue);
        QWidget* editor = delegate->createEditor(table, option, index);
        QVERIFY(editor);
        delegate->setEditorData(editor, index);
        delegate->setModelData(editor, model, index);
        delete editor;
    }
    for (int column : {ColDataType, ColRegistryOrder, ColByteOrder}) {
        auto* cellDelegate = qobject_cast<QStyledItemDelegate*>(table->itemDelegateForColumn(column));
        QHelpEvent event(QEvent::ToolTip, QPoint(2, 2), QPoint(2, 2));
        QVERIFY(cellDelegate->helpEvent(&event, table, option, model->index(0, column)));
        QToolTip::hideText();
    }
}

/// \brief Finds, replaces and navigates matches without losing editor state.
void TestForms::searchesAndReplacesScriptText()
{
    JSCodeEditor editor;
    editor.resize(600, 300);
    editor.show();
    editor.setPlainText(QStringLiteral("alpha beta alpha\nAlpha beta alphabet"));
    QCOMPARE(editor.highlightAllMatches(QStringLiteral("alpha")), 4);
    QCOMPARE(editor.totalMatchCount(), 4);
    QVERIFY(editor.findNext(QStringLiteral("alpha")));
    QVERIFY(editor.findPrevious(QStringLiteral("alpha")));
    QCOMPARE(editor.highlightAllMatches(QStringLiteral("alpha"), QTextDocument::FindCaseSensitively), 3);
    QCOMPARE(editor.highlightAllMatches(QStringLiteral("alpha"), QTextDocument::FindWholeWords), 3);
    QCOMPARE(editor.replaceAll(QStringLiteral("beta"), QStringLiteral("gamma")), 2);
    QVERIFY(!editor.toPlainText().contains(QStringLiteral("beta")));
    editor.moveCursor(QTextCursor::Start);
    QCOMPARE(editor.highlightAllMatches(QStringLiteral("alpha")), 4);
    QVERIFY(editor.findNext(QStringLiteral("alpha")));
    editor.replaceCurrent(QStringLiteral("alpha"), QStringLiteral("delta"));
    QVERIFY(editor.toPlainText().contains(QStringLiteral("delta")));
    editor.clearSearchHighlights();
    QCOMPARE(editor.totalMatchCount(), 0);
    QVERIFY(!editor.findNext(QStringLiteral("missing")));
    editor.enableAutoComplete(false);
    QVERIFY(!editor.isAutoCompleteEnabled());
    editor.enableAutoComplete(true);
    QVERIFY(editor.isAutoCompleteEnabled());
    editor.setForegroundColor(QColor("#123456"));
    editor.setBackgroundColor(QColor("#abcdef"));
    QCOMPARE(editor.foregroundColor(), QColor("#123456"));
    QCOMPARE(editor.backgroundColor(), QColor("#abcdef"));
    QTest::keyClick(&editor, Qt::Key_Home);
    QTest::keyClicks(&editor, "test");
    QTest::keyClick(&editor, Qt::Key_Return);
    QVERIFY(editor.lineNumberAreaWidth() > 0);
    QVERIFY(!editor.grab().isNull());
    translate(&editor);
}


/// \brief Round-trips preferences, including project defaults, through XML and settings.
void TestForms::preferencesRoundTrip()
{
    QTemporaryDir dir;
    auto& prefs = AppPreferences::instance();
    QSettings original(dir.filePath("original.ini"), QSettings::IniFormat);
    prefs.save(original);
    QSignalSpy changes(&prefs, &AppPreferences::settingChanged);
    QSignalSpy global(&prefs, &AppPreferences::globalSettingChanged);
    prefs.setFont(QFont(QStringLiteral("Monaco"), 12));
    prefs.setScriptFont(QFont(QStringLiteral("Monaco"), 14));
    prefs.setFontZoom(125);
    prefs.setBackgroundColor(QColor("#123456"));
    prefs.setForegroundColor(QColor("#abcdef"));
    prefs.setAddressColor(QColor("#123abc"));
    prefs.setCommentColor(QColor("#abc123"));
    prefs.setLanguage(QStringLiteral("en"));
    prefs.setCodeAutoComplete(false);
    prefs.setAutoShowConsoleOutput(false);
    prefs.setShowWelcomeDialog(false);
    prefs.setConsoleMaxLines(200);
    prefs.setCheckForUpdates(false);
    prefs.setGlobalAddressBase(AddressBase::Base0);
    prefs.setGlobalHexView(true);
    prefs.setSaveAllModifiedRegisters(true);
    prefs.setSaveRegisterTimestamps(false);
    prefs.setSaveRuntimeRegisterValues(false);
    DataViewDefinitions data;
    data.DeviceId = 8;
    data.PointAddress = 23;
    data.Length = 32;
    prefs.setDataViewDefinitions(data);
    TrafficViewDefinitions traffic;
    traffic.UnitFilter = 8;
    traffic.LogViewLimit = 500;
    prefs.setTrafficViewDefinitions(traffic);
    ScriptViewDefinitions script;
    script.ScriptCfg.Mode = RunMode::Once;
    prefs.setScriptViewDefinitions(script);
    QVERIFY(!changes.isEmpty());
    QVERIFY(!global.isEmpty());
    QByteArray saved;
    QXmlStreamWriter writer(&saved);
    prefs.saveXml(writer);
    prefs.setFontZoom(80);
    prefs.setGlobalHexView(false);
    QXmlStreamReader reader(saved);
    QVERIFY(reader.readNextStartElement());
    prefs.loadXml(reader);
    QVERIFY(!reader.hasError());
    QCOMPARE(prefs.fontZoom(), 125);
    QCOMPARE(prefs.backgroundColor(), QColor("#123456"));
    QCOMPARE(prefs.globalAddressBase(), AddressBase::Base0);
    QVERIFY(prefs.globalHexView());
    QCOMPARE(prefs.dataViewDefinitions(), data);
    QCOMPARE(prefs.trafficViewDefinitions(), traffic);
    QCOMPARE(prefs.scriptViewDefinitions(), script);
    QSettings savedSettings(dir.filePath("saved.ini"), QSettings::IniFormat);
    prefs.save(savedSettings);
    prefs.setFontZoom(80);
    prefs.setGlobalHexView(false);
    prefs.load(savedSettings);
    QCOMPARE(prefs.fontZoom(), 125);
    QVERIFY(prefs.globalHexView());
    prefs.load(original);
}

/// \brief Filters event severity, evicts old entries and confirms destructive clearing.
void TestForms::applicationLogFiltersAndClears()
{
    AppLogOutput log;
    QSignalSpy opened(&log, &AppLogOutput::openRequested);
    log.setMaxLines(3);
    log.addEvent(QStringLiteral("first"), AppLogOutput::EventType::Info);
    log.addEvent(QStringLiteral("warning"), AppLogOutput::EventType::Warning);
    log.addEvent(QStringLiteral("error"), AppLogOutput::EventType::Error);
    QCOMPARE(opened.count(), 2);
    auto* list = control<QListWidget>(&log, "listWidget");
    QCOMPARE(list->count(), 3);
    control<QAction>(&log, "actionFilterWarn")->setChecked(false);
    QVERIFY(list->item(1)->isHidden());
    control<QAction>(&log, "actionFilterWarn")->setChecked(true);
    QVERIFY(!list->item(1)->isHidden());
    control<QAction>(&log, "actionFilterError")->setChecked(false);
    QVERIFY(list->item(2)->isHidden());
    control<QAction>(&log, "actionFilterError")->setChecked(true);
    log.addEvent(QStringLiteral("fourth"), AppLogOutput::EventType::Info);
    QCOMPARE(list->count(), 3);
    QCOMPARE(list->item(0)->text(), QStringLiteral("warning"));
    QVERIFY(QMetaObject::invokeMethod(&log, "copyAllToClipboard"));
    QVERIFY(QApplication::clipboard()->text().contains(QStringLiteral("error")));
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("events.txt"));
    selectNextFile(path);
    control<QAction>(&log, "actionExport")->trigger();
    QFile exported(path);
    QVERIFY(exported.open(QIODevice::ReadOnly));
    QVERIFY(exported.readAll().contains("error"));
    log.show();
    QVERIFY(!log.grab().isNull());
    translate(&log);
    confirm(QMessageBox::No);
    control<QAction>(&log, "actionClear")->trigger();
    QVERIFY(!log.isEmpty());
    confirm(QMessageBox::Yes);
    control<QAction>(&log, "actionClear")->trigger();
    QVERIFY(log.isEmpty());
}


/// \brief Finds matching register values and maintains annotations across display changes.
void TestForms::dataOutputFindsAndFormatsValues()
{
    OutputDataWidget output;
    DataViewDefinitions dd;
    dd.DeviceId = 1;
    dd.PointAddress = 0;
    dd.Length = 8;
    dd.PointType = QModbusDataUnit::HoldingRegisters;
    QModbusDataUnit values(dd.PointType, 0, QVector<quint16>{1, 42, 7, 42, 255, 0, 65535, 1});
    output.setDataType(DataType::UInt16);
    output.setup(dd, true, AddressSpace::Addr6Digits, {}, values);
    QCOMPARE(output.data(), values.values());
    QCOMPARE(output.rowCount(), 8);
    QVERIFY(output.findNextByValue(QStringLiteral("42")));
    QVERIFY(output.findNextByValue(QStringLiteral("42")));
    QVERIFY(output.findPreviousByValue(QStringLiteral("42")));
    QVERIFY(!output.findNextByValue(QStringLiteral("not a number")));
    output.setDataType(DataType::Hex);
    QVERIFY(output.findNextByValue(QStringLiteral("0x2a")));
    QVERIFY(!output.findPreviousByValue(QStringLiteral("10000")));
    output.setDataType(DataType::Int16);
    QVERIFY(output.findNextByValue(QStringLiteral("42")));
    QVERIFY(!output.findNextByValue(QStringLiteral("32768")));
    QVERIFY(!output.findPreviousByValue(QStringLiteral("-32769")));
    QVERIFY(!output.findPreviousByValue(QString()));
    output.highlightByValue(42, Qt::yellow);
    output.clearHighlights();
    output.setDescription(1, dd.PointType, 1, QStringLiteral("Temperature"));
    output.setColor(1, dd.PointType, 1, QColor(Qt::yellow));
    QVERIFY(!output.descriptionMap().isEmpty());
    QVERIFY(!output.colorMap().isEmpty());
    for (auto type : {DataType::Int16, DataType::UInt16, DataType::Int32, DataType::UInt32,
                      DataType::Int64, DataType::UInt64, DataType::Float32, DataType::Float64,
                      DataType::Ansi, DataType::Binary, DataType::Hex}) {
        output.setDataType(type);
        output.setRegisterOrder(RegisterOrder::LSRF);
        output.setByteOrder(ByteOrder::Swapped);
        output.setCodepage(QStringLiteral("UTF-8"));
        output.refreshDisplay();
        output.setSimulated(type, RegisterOrder::LSRF, 1, dd.PointType, 0, true);
        output.setSimulated(type, RegisterOrder::LSRF, 1, dd.PointType, 0, false);
    }
    output.setZoomPercent(150);
    QCOMPARE(output.zoomPercent(), 150);
    output.resize(600, 300);
    output.show();
    QVERIFY(!output.grab().isNull());
    output.setDataType(DataType::UInt16);
    auto* list = control<QListView>(&output, "listView");
    const auto item = list->model()->index(0, 0);
    const QPoint point = list->visualRect(item).center();
    QSignalSpy edited(&output, &OutputDataWidget::itemDoubleClicked);
    QTimer::singleShot(0, [] {
        if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget()))
            QTest::mouseClick(menu, Qt::LeftButton, {}, menu->actionGeometry(menu->actions().at(0)).center());
    });
    QVERIFY(QMetaObject::invokeMethod(&output, "on_listView_customContextMenuRequested", Q_ARG(QPoint, point)));
    QCOMPARE(edited.count(), 1);
    QTimer::singleShot(0, [] {
        if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget()))
            QTest::mouseClick(menu, Qt::LeftButton, {}, menu->actionGeometry(menu->actions().at(5)).center());
    });
    QVERIFY(QMetaObject::invokeMethod(&output, "on_listView_customContextMenuRequested", Q_ARG(QPoint, point)));
    QVERIFY(!output.colorMap().isEmpty());
    output.setDataType(DataType::Binary);
    dd.PointType = QModbusDataUnit::Coils;
    QModbusDataUnit bits(dd.PointType, 0, QVector<quint16>{1, 0, 1, 0});
    output.setup(dd, true, AddressSpace::Addr5Digits, {}, bits);
    output.highlightBits(1, Qt::yellow);
    QVERIFY(output.findNextByValue(QStringLiteral("1")));
    output.clearFindByValue();
    translate(&output);
}


/// \brief Keeps window identity and panel options across tabbed and split modes.
void TestForms::mdiPanelsPreserveWindowsAndOptions()
{
    MdiAreaEx area;
    area.resize(800, 500);
    area.show();
    area.setDocumentMode(true);
    QVERIFY(area.documentMode());
    area.setTabsClosable(true);
    QVERIFY(area.tabsClosable());
    area.setTabsMovable(true);
    QVERIFY(area.tabsMovable());
    area.setTabsExpanding(true);
    QVERIFY(area.tabsExpanding());
    area.setActivationOrder(QMdiArea::ActivationHistoryOrder);
    QCOMPARE(area.activationOrder(), QMdiArea::ActivationHistoryOrder);
    area.setBackground(QBrush(QColor(Qt::blue)));
    QCOMPARE(area.background().color(), QColor(Qt::blue));
    area.setOption(QMdiArea::DontMaximizeSubWindowOnActivation, true);
    QVERIFY(area.testOption(QMdiArea::DontMaximizeSubWindowOnActivation));
    area.setTabPosition(QTabWidget::South);
    QCOMPARE(area.tabPosition(), QTabWidget::South);
    area.setTabShape(QTabWidget::Triangular);
    QCOMPARE(area.tabShape(), QTabWidget::Triangular);
    auto* first = new QWidget;
    auto* second = new QWidget;
    first->setWindowTitle(QStringLiteral("First"));
    second->setWindowTitle(QStringLiteral("Second"));
    auto* firstWindow = area.addSubWindow(first);
    auto* secondWindow = area.addSubWindow(second);
    firstWindow->show();
    secondWindow->show();
    QCOMPARE(area.subWindowList().size(), 2);
    area.setViewMode(QMdiArea::SubWindowView);
    area.cascadeSubWindows();
    area.tileSubWindows();
    area.setViewMode(QMdiArea::TabbedView);
    area.setActiveSubWindow(firstWindow);
    QCOMPARE(area.activeSubWindow(), firstWindow);
    area.setSplitViewEnabled(true);
    QVERIFY(area.isSplitView());
    QVERIFY(area.secondaryArea());
    area.moveSubWindowToOtherPanel(firstWindow);
    QCOMPARE(area.subWindowList().size(), 2);
    QCOMPARE(area.secondaryArea()->localSubWindowList().size(), 1);
    area.moveSubWindowToOtherPanel(firstWindow);
    QCOMPARE(area.primaryArea()->localSubWindowList().size(), 2);
    area.setSplitViewEnabled(false);
    QVERIFY(!area.isSplitView());
    QCOMPARE(area.subWindowList().size(), 2);
    translate(&area);
    area.closeAllSubWindows();
    QCoreApplication::processEvents();
}

/// \brief Reflects form titles, open state and script execution in the project tree.
void TestForms::treeTracksFormLifecycle()
{
    ProjectTreeWidget tree;
    QWidget data;
    QWidget script;
    QWidget traffic;
    QWidget map;
    data.setWindowTitle(QStringLiteral("Data"));
    script.setWindowTitle(QStringLiteral("Script"));
    traffic.setWindowTitle(QStringLiteral("Traffic"));
    map.setWindowTitle(QStringLiteral("Map"));
    tree.addForm(ProjectFormType::Data, &data);
    tree.addForm(ProjectFormType::Script, &script);
    tree.addForm(ProjectFormType::Traffic, &traffic);
    tree.addForm(ProjectFormType::DataMap, &map);
    QCOMPARE(tree.topLevelItemCount(), 4);
    QSignalSpy activated(&tree, &ProjectTreeWidget::formActivated);
    tree.activateForm(&script);
    tree.setFormScriptRunning(&script, true);
    tree.setFormScriptRunning(&script, false);
    tree.setFormOpen(&data, false);
    tree.setFormOpen(&data, true);
    data.setWindowTitle(QStringLiteral("Renamed"));
    tree.updateFormTitle(&data);
    const auto found = tree.findItems(QStringLiteral("Renamed"), Qt::MatchRecursive);
    QCOMPARE(found.size(), 1);
    QVERIFY(QMetaObject::invokeMethod(&tree, "on_itemActivated", Q_ARG(QTreeWidgetItem*, found.first()), Q_ARG(int, 0)));
    QCOMPARE(activated.count(), 1);
    tree.show();
    QVERIFY(!tree.grab().isNull());
    translate(&tree);
    tree.removeForm(&data);
    QVERIFY(tree.findItems(QStringLiteral("Renamed"), Qt::MatchRecursive).isEmpty());
    tree.removeForm(&script);
    tree.removeForm(&traffic);
    tree.removeForm(&map);
}


/// \brief Renders common controls under each supported application theme and style.
void TestForms::themedControlsRenderInBothThemes()
{
#if defined(HAVE_QLEMENTINE_APP_STYLE)
    auto& prefs = AppPreferences::instance();
    const auto original = prefs.themeMode();
    for (int variant = 0; variant < 2; ++variant) {
        std::unique_ptr<QStyle> style(variant == 0 ? static_cast<QStyle*>(new QlementineAppStyle)
                                                   : static_cast<QStyle*>(new MacAppStyle));
        QWidget panel;
        panel.setStyle(style.get());
        auto* layout = new QVBoxLayout(&panel);
        auto* toolbar = new QToolBar(&panel);
        toolbar->addAction(QStringLiteral("Open"));
        layout->addWidget(toolbar);
        auto* button = new QPushButton(QStringLiteral("Apply"), &panel);
        layout->addWidget(button);
        auto* check = new QCheckBox(QStringLiteral("Enabled"), &panel);
        layout->addWidget(check);
        auto* combo = new QComboBox(&panel);
        combo->addItems({QStringLiteral("First"), QStringLiteral("Second")});
        layout->addWidget(combo);
        auto* edit = new QLineEdit(QStringLiteral("Text"), &panel);
        layout->addWidget(edit);
        auto* tabs = new QTabWidget(&panel);
        auto* list = new QListWidget(tabs);
        list->addItems({QStringLiteral("First"), QStringLiteral("Second")});
        auto* table = new QTableView(tabs);
        auto* model = new QStandardItemModel(2, 2, table);
        model->setData(model->index(0, 0), QStringLiteral("Value"));
        table->setModel(model);
        tabs->addTab(list, QStringLiteral("List"));
        tabs->addTab(table, QStringLiteral("Table"));
        layout->addWidget(tabs);
        panel.resize(500, 500);
        panel.show();
        for (auto mode : {AppThemeMode::Light, AppThemeMode::Dark}) {
            prefs.setThemeMode(mode);
            QCOMPARE(theApp()->theme().isDark(), mode == AppThemeMode::Dark);
            check->setChecked(true);
            button->setDown(true);
            list->setCurrentRow(0);
            tabs->setCurrentIndex(0);
            QVERIFY(!panel.grab().isNull());
            button->setDown(false);
            button->setEnabled(false);
            check->setChecked(false);
            tabs->setCurrentIndex(1);
            table->selectRow(0);
            edit->setFocus();
            QVERIFY(!panel.grab().isNull());
            button->setEnabled(true);
        }
    }
    prefs.setThemeMode(original);
#else
    QSKIP("The Qlementine application style is not available in this build.");
#endif
}


/// \brief Applies a map row color and a project-tree command through their context menus.
void TestForms::contextMenusApplyActions()
{
    Fixture f;
    auto* form = f.create<FormDataMapView>(ProjectFormKind::DataMap);
    auto* model = form->findChild<DataMapDataModel*>();
    auto* table = control<QTableView>(form, "tableView");
    model->addEntry({1, QModbusDataUnit::HoldingRegisters, 0}, DataMapEntry());
    f.Mdi.resize(900, 500);
    f.Mdi.show();
    form->show();
    QCoreApplication::processEvents();
    const QPoint point = table->visualRect(table->model()->index(0, 0)).center();
    QTimer::singleShot(0, [] {
        if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget()))
            QTest::mouseClick(menu, Qt::LeftButton, {}, menu->actionGeometry(menu->actions().at(8)).center());
    });
    QVERIFY(QMetaObject::invokeMethod(form, "on_tableView_customContextMenuRequested", Q_ARG(QPoint, point)));
    QVERIFY(model->data(model->index(0, 0), DataMapRole::RowColor).value<QColor>().isValid());
    QTimer::singleShot(0, [] {
        if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget()))
            QTest::mouseClick(menu, Qt::LeftButton, {}, menu->actionGeometry(menu->actions().at(6)).center());
    });
    QVERIFY(QMetaObject::invokeMethod(form, "on_tableView_customContextMenuRequested", Q_ARG(QPoint, point)));
    QVERIFY(!model->data(model->index(0, 0), DataMapRole::RowColor).value<QColor>().isValid());

    ProjectTreeWidget tree;
    QWidget data;
    data.setWindowTitle(QStringLiteral("Data item"));
    tree.addForm(ProjectFormType::Data, &data);
    tree.resize(400, 400);
    tree.show();
    tree.expandAll();
    QCoreApplication::processEvents();
    const auto items = tree.findItems(QStringLiteral("Data item"), Qt::MatchRecursive);
    QCOMPARE(items.size(), 1);
    qRegisterMetaType<ProjectFormType>();
    QSignalSpy created(&tree, &ProjectTreeWidget::formCreateRequested);
    const QPoint itemPoint = tree.visualItemRect(items.first()).center();
    QTimer::singleShot(0, [] {
        if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget()))
            QTest::mouseClick(menu, Qt::LeftButton, {}, menu->actionGeometry(menu->actions().at(1)).center());
    });
    QVERIFY(QMetaObject::invokeMethod(&tree, "on_contextMenu", Q_ARG(QPoint, itemPoint)));
    QCOMPARE(created.count(), 1);
    QCOMPARE(created.first().first().value<ProjectFormType>(), ProjectFormType::DataMap);
    QSignalSpy renamed(&tree, &ProjectTreeWidget::formRenamed);
    items.first()->setText(0, QStringLiteral("New name"));
    QCOMPARE(data.windowTitle(), QStringLiteral("New name"));
    QCOMPARE(renamed.count(), 1);
    items.first()->setText(0, QStringLiteral("   "));
    QCOMPARE(items.first()->text(0), QStringLiteral("New name"));
}


/// \brief Routes script commands, searches and modal setup actions from the main window.
void TestForms::mainWindowActionsAndScriptCommands()
{
    QTemporaryDir profile;
    MainWindow host(profile.path(), false);
    control<QAction>(&host, "actionNewDataView")->trigger();
    control<QAction>(&host, "actionNewScript")->trigger();
    auto* script = host.findChild<FormScriptView*>();
    auto* tree = host.findChild<ProjectTreeWidget*>();
    QVERIFY(script && tree);
    script->setScript(QStringLiteral("var x = 1;"));
    ProjectFormRef ref{ProjectFormType::Script, script};
    QSignalSpy running(script, &FormScriptView::scriptRunning);
    QSignalSpy stopped(script, &FormScriptView::scriptStopped);
    emit tree->formActivated(ref);
    emit tree->formRunScriptRequested(ref);
    QCOMPARE(running.count(), 1);
    emit tree->formStopScriptRequested(ref);
    QTRY_VERIFY(stopped.count() > 0);
    emit tree->runAllScriptsRequested();
    QCOMPARE(running.count(), 2);
    emit tree->stopAllScriptsRequested();
    QVERIFY(QMetaObject::invokeMethod(&host, "on_findActionTriggered"));
    QVERIFY(QMetaObject::invokeMethod(&host, "on_replaceActionTriggered"));
    host.appendConsoleMessage(QStringLiteral("test"), QStringLiteral("message"), ConsoleOutput::MessageType::Log);
    host.showOutputConsole();
    host.showHelpContext(QString());
    host.updateHelpWidgetState();
    host.applyConnections(ModbusDefinitions(), {});
    for (const char* action : {"actionAbout", "actionPreferences", "actionMbDefinitions",
                                "actionForceCoils", "actionForceDiscretes",
                                "actionPresetInputRegs", "actionPresetHoldingRegs"}) {
        QTimer::singleShot(0, [] {
            if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                dialog->reject();
        });
        control<QAction>(&host, action)->trigger();
    }
    control<QAction>(&host, "actionSnifferMode")->setChecked(true);
    QVERIFY(host.mbMultiServer().snifferMode());
    control<QAction>(&host, "actionSnifferMode")->setChecked(false);
    QVERIFY(!host.mbMultiServer().snifferMode());
    const QString path = profile.filePath("actions.omsim");
    QVERIFY(host.saveProject(path));
    QVERIFY(host.closeProject());
}


/// \brief Adds requested ranges from each supported Modbus operation to the data map.
void TestForms::mapAutomaticallyAddsRequestedRegisters()
{
    Fixture f;
    auto* form = f.create<FormDataMapView>(ProjectFormKind::DataMap);
    auto* model = form->findChild<DataMapDataModel*>();
    form->setAutoAddOnRequest(true);
    struct Request { QModbusPdu::FunctionCode code; const char* data; };
    const QList<Request> requests = {
        {QModbusPdu::ReadCoils, "00000002"},
        {QModbusPdu::ReadDiscreteInputs, "00000002"},
        {QModbusPdu::ReadHoldingRegisters, "00000002"},
        {QModbusPdu::ReadInputRegisters, "00000002"},
        {QModbusPdu::WriteSingleCoil, "0003ff00"},
        {QModbusPdu::WriteSingleRegister, "0003002a"},
        {QModbusPdu::WriteMultipleCoils, "000400020101"},
        {QModbusPdu::WriteMultipleRegisters, "000400020400010002"},
        {QModbusPdu::MaskWriteRegister, "000700ff0001"},
        {QModbusPdu::ReadWriteMultipleRegisters, "00080002000a00020400010002"}
    };
    const ConnectionDetails connection;
    for (const auto& item : requests) {
        const auto request = ModbusMessage::create(
            QModbusRequest(item.code, QByteArray::fromHex(item.data)), ModbusMessage::Tcp,
            1, 1, QDateTime::currentDateTime(), true);
        const int before = model->rowCount();
        emit f.Server.requestOnConnection(connection, request);
        QVERIFY(model->rowCount() > before);
    }
}


#if defined(HAVE_QLEMENTINE_APP_STYLE)
namespace {
class ExposedQlementineStyle : public QlementineAppStyle
{
public:
    using QlementineAppStyle::buttonBackgroundColor;
    using QlementineAppStyle::buttonForegroundColor;
    using QlementineAppStyle::iconForegroundColor;
    using QlementineAppStyle::listItemBackgroundColor;
    using QlementineAppStyle::listItemForegroundColor;
    using QlementineAppStyle::splitterColor;
    using QlementineAppStyle::tabBackgroundColor;
    using QlementineAppStyle::tabBarBackgroundColor;
    using QlementineAppStyle::tabForegroundColor;
    using QlementineAppStyle::tableHeaderBgColor;
    using QlementineAppStyle::tableHeaderFgColor;
    using QlementineAppStyle::tableLineColor;
    using QlementineAppStyle::textFieldBackgroundColor;
    using QlementineAppStyle::textFieldBorderColor;
    using QlementineAppStyle::toolBarBackgroundColor;
    using QlementineAppStyle::toolBarBorderColor;
    using QlementineAppStyle::toolTipForegroundColor;
    using QlementineAppStyle::toolButtonBackgroundColor;
    using QlementineAppStyle::toolButtonForegroundColor;
};

/// \brief Verifies that interactive widget states produce usable theme colors.
template<class Style> void checkStyleColors(Style& style)
{
    using namespace oclero::qlementine;
    QStandardItemModel model(1, 1);
    for (auto mouse : {MouseState::Normal, MouseState::Hovered, MouseState::Pressed,
                       MouseState::Disabled, MouseState::Transparent}) {
        for (auto role : {ColorRole::Primary, ColorRole::Secondary}) {
            QVERIFY(style.buttonBackgroundColor(mouse, role).isValid());
            QVERIFY(style.buttonForegroundColor(mouse, role).isValid());
            QVERIFY(style.iconForegroundColor(mouse, role).isValid());
            QVERIFY(style.toolButtonBackgroundColor(mouse, role).isValid());
            QVERIFY(style.toolButtonForegroundColor(mouse, role).isValid());
        }
        QVERIFY(style.splitterColor(mouse).isValid());
        QVERIFY(style.tabBarBackgroundColor(mouse).isValid());
        for (auto selected : {SelectionState::Selected, SelectionState::NotSelected}) {
            QVERIFY(style.tabBackgroundColor(mouse, selected).isValid());
            QVERIFY(style.tabForegroundColor(mouse, selected).isValid());
            for (auto focus : {FocusState::Focused, FocusState::NotFocused})
                for (auto active : {ActiveState::Active, ActiveState::NotActive}) {
                    QVERIFY(style.listItemBackgroundColor(mouse, selected, focus, active, model.index(0, 0)).isValid());
                    QVERIFY(style.listItemForegroundColor(mouse, selected, focus, active).isValid());
                }
        }
        for (auto checked : {CheckState::Checked, CheckState::NotChecked, CheckState::Indeterminate}) {
            QVERIFY(style.tableHeaderBgColor(mouse, checked).isValid());
            QVERIFY(style.tableHeaderFgColor(mouse, checked).isValid());
        }
        for (auto status : {Status::Default, Status::Info, Status::Success, Status::Warning, Status::Error}) {
            QVERIFY(style.textFieldBackgroundColor(mouse, status).isValid());
            for (auto focus : {FocusState::Focused, FocusState::NotFocused})
                QVERIFY(style.textFieldBorderColor(mouse, focus, status).isValid());
        }
    }
    QVERIFY(style.tableLineColor().isValid());
    QVERIFY(style.toolBarBackgroundColor().isValid());
    QVERIFY(style.toolBarBorderColor().isValid());
    QVERIFY(style.toolTipForegroundColor().isValid());
}
}
#endif

/// \brief Keeps colors valid for normal, disabled and selected controls in both themes.
void TestForms::styleColorsCoverInteractionStates()
{
#if defined(HAVE_QLEMENTINE_APP_STYLE)
    auto& prefs = AppPreferences::instance();
    const auto original = prefs.themeMode();
    ExposedQlementineStyle generic;
    MacAppStyle mac;
    for (auto mode : {AppThemeMode::Light, AppThemeMode::Dark}) {
        prefs.setThemeMode(mode);
        checkStyleColors(generic);
        checkStyleColors(mac);
    }
    prefs.setThemeMode(original);
#else
    QSKIP("The Qlementine application style is not available in this build.");
#endif
}


namespace {
QStringList* capturedLog = nullptr;

/// \brief Captures logging while preserving the previous Qt message handler.
void captureLogMessage(QtMsgType, const QMessageLogContext&, const QString& message)
{
    if (capturedLog)
        capturedLog->append(message);
}

class LogCapture
{
public:
    LogCapture() : previous(qInstallMessageHandler(captureLogMessage)) { capturedLog = &messages; }
    ~LogCapture() { capturedLog = nullptr; qInstallMessageHandler(previous); }
    QStringList messages;
private:
    QtMessageHandler previous;
};
}

/// \brief Records source-specific writes, connection events and preference changes.
void TestForms::logsServerAndPreferenceEvents()
{
    QObject context;
    ModbusMultiServer server;
    AppLogger::setupModbusMultiServerLogging(server, &context);
    auto& prefs = AppPreferences::instance();
    AppLogger::setupAppPreferencesLogging(prefs, &context);
    LogCapture log;
    ConnectionDetails details;
    details.Type = ConnectionType::Tcp;
    details.TcpParams.IPAddress = QStringLiteral("127.0.0.1");
    details.TcpParams.ServicePort = 1502;
    emit server.connected(details);
    QVERIFY(log.messages.last().contains(QStringLiteral("127.0.0.1:1502")));
    details.Type = ConnectionType::Serial;
    details.SerialParams.PortName = QStringLiteral("COM77");
    emit server.disconnected(details);
    QVERIFY(log.messages.last().contains(QStringLiteral("COM77")));
    details.Type = ConnectionType::RtuTcp;
    emit server.connected(details);
    ModbusClientInfo client;
    client.Address = QStringLiteral("192.0.2.42");
    client.Port = 1234;
    client.Connection = details;
    emit server.clientConnected(client);
    QVERIFY(log.messages.last().contains(QStringLiteral("192.0.2.42:1234")));
    emit server.clientDisconnected(client);
    emit server.connectionError(QStringLiteral("test connection failure"));
    QVERIFY(log.messages.last().contains(QStringLiteral("test connection failure")));
    emit server.errorOccured(7, QStringLiteral("test device failure"));
    QVERIFY(log.messages.last().contains(QStringLiteral("test device failure")));
    emit server.deviceIdAdded(7);
    emit server.deviceIdRemoved(7);
    const auto type = QModbusDataUnit::HoldingRegisters;
    emit server.descriptionChanged(7, type, 3, QStringLiteral("Temperature"), WriteSource::User);
    QVERIFY(log.messages.last().contains(QStringLiteral("Temperature")));
    const auto count = log.messages.size();
    emit server.descriptionChanged(7, type, 3, QStringLiteral("Ignored"), WriteSource::Internal);
    QCOMPARE(log.messages.size(), count);
    emit server.descriptionChanged(7, type, 3, QString(), WriteSource::User);
    const QModbusDataUnit words(type, 0, QVector<quint16>(20, 42));
    emit server.dataChanged(7, words, WriteSource::User, client);
    QVERIFY(log.messages.last().contains(QStringLiteral("42")));
    QVERIFY(log.messages.last().contains(QStringLiteral("...")));
    emit server.dataChanged(7, words, WriteSource::ModbusClient, client);
    QVERIFY(log.messages.last().contains(QStringLiteral("192.0.2.42:1234")));
    client.Address.clear();
    emit server.dataChanged(7, words, WriteSource::ModbusClient, client);
    emit server.dataChanged(7, QModbusDataUnit(type, 0, QVector<quint16>{7}), WriteSource::User, client);
    QVERIFY(log.messages.last().contains(QStringLiteral("7")));
    for (const QString& name : {QStringLiteral("Font"), QStringLiteral("FontZoom"),
                                QStringLiteral("BackgroundColor"), QStringLiteral("ForegroundColor"),
                                QStringLiteral("AddressColor"), QStringLiteral("CommentColor"),
                                QStringLiteral("CheckForUpdates"), QStringLiteral("ShowWelcomeDialog"),
                                QStringLiteral("Language"), QStringLiteral("ScriptFont"),
                                QStringLiteral("CodeAutoComplete"), QStringLiteral("AutoShowConsoleOutput"),
                                QStringLiteral("ConsoleMaxLines"), QStringLiteral("DataView.ColumnsDistance"),
                                QStringLiteral("DataView.LeadingZeros"), QStringLiteral("TrafficView.LogLimit"),
                                QStringLiteral("TrafficView.AutoScroll")})
        emit prefs.settingChanged(name, QStringLiteral("false"), QStringLiteral("true"));
    emit prefs.globalSettingChanged(QStringLiteral("GlobalAddressBase"), QStringLiteral("true"), QStringLiteral("false"));
    emit prefs.globalSettingChanged(QStringLiteral("GlobalHexView"), QStringLiteral("false"), QStringLiteral("true"));
    emit prefs.globalSettingChanged(QStringLiteral("ThemeMode"), QStringLiteral("light"), QStringLiteral("dark"));
    QVERIFY(log.messages.size() > 30);
}


/// \brief Reorders mapped rows and propagates backend timestamps and descriptions.
void TestForms::mapDragAndBackendUpdates()
{
    Fixture f;
    auto* form = f.create<FormDataMapView>(ProjectFormKind::DataMap);
    auto* model = form->findChild<DataMapDataModel*>();
    for (quint16 address : {0, 1, 2})
        model->addEntry({1, QModbusDataUnit::HoldingRegisters, address}, DataMapEntry());
    std::unique_ptr<QMimeData> mime(model->mimeData({model->index(0, ColUnit)}));
    QVERIFY(mime);
    QVERIFY(model->canDropMimeData(mime.get(), Qt::MoveAction, 3, 0, {}));
    QVERIFY(!model->canDropMimeData(mime.get(), Qt::CopyAction, 3, 0, {}));
    QVERIFY(model->dropMimeData(mime.get(), Qt::MoveAction, 3, 0, {}));
    QCOMPARE(model->keyForRow(2).Address, quint16(0));
    QVERIFY(!model->moveRows({}, -1, 1, {}, 0));
    QVERIFY(!model->moveRows({}, 0, 2, {}, 3));
    const auto timestamp = QDateTime::currentDateTime();
    f.Server.setTimestamp(1, QModbusDataUnit::HoldingRegisters, 0, timestamp);
    QTRY_COMPARE(model->entries().value(model->keyForRow(2)).timestamp, timestamp);
    f.Server.setDescription(1, QModbusDataUnit::HoldingRegisters, 0, QStringLiteral("Pressure"));
    QTRY_COMPARE(model->index(2, ColComment).data().toString(), QStringLiteral("Pressure"));
    f.Server.clearTimestamps();
    model->refreshTimestamps();
    QCOMPARE(model->entries().value(model->keyForRow(2)).timestamp,
             f.Server.timestamp(1, QModbusDataUnit::HoldingRegisters, 0));
}

/// \brief Filters, copies and clears console messages of each severity.
void TestForms::consoleFiltersAndCopiesMessages()
{
    ConsoleOutput console;
    console.setMaxLines(5);
    console.addMessage(QStringLiteral("log"), ConsoleOutput::MessageType::Log, QStringLiteral("script"));
    console.addMessage(QStringLiteral("debug"), ConsoleOutput::MessageType::Debug);
    console.addMessage(QStringLiteral("warning"), ConsoleOutput::MessageType::Warning);
    console.addMessage(QStringLiteral("error"), ConsoleOutput::MessageType::Error);
    console.flush();
    auto* list = control<QListWidget>(&console, "listWidget");
    QCOMPARE(list->count(), 4);
    control<QAction>(&console, "actionFilterWarn")->setChecked(false);
    QVERIFY(list->item(2)->isHidden());
    control<QAction>(&console, "actionFilterWarn")->setChecked(true);
    control<QAction>(&console, "actionFilterError")->setChecked(false);
    QVERIFY(list->item(3)->isHidden());
    control<QAction>(&console, "actionFilterError")->setChecked(true);
    control<QAction>(&console, "actionFilterLog")->setChecked(false);
    QVERIFY(list->item(0)->isHidden());
    control<QAction>(&console, "actionFilterLog")->setChecked(true);
    QVERIFY(QMetaObject::invokeMethod(&console, "copyAllToClipboard"));
    QVERIFY(QApplication::clipboard()->text().contains(QStringLiteral("warning")));
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("console.txt"));
    selectNextFile(path);
    control<QAction>(&console, "actionExport")->trigger();
    QFile exported(path);
    QVERIFY(exported.open(QIODevice::ReadOnly));
    QVERIFY(exported.readAll().contains("warning"));
    console.show();
    QVERIFY(!console.grab().isNull());
    translate(&console);
    confirm(QMessageBox::No);
    control<QAction>(&console, "actionClear")->trigger();
    QVERIFY(!console.isEmpty());
    confirm(QMessageBox::Yes);
    control<QAction>(&console, "actionClear")->trigger();
    QVERIFY(console.isEmpty());
}

/// \brief Restores older embedded register values and annotations in data-view XML.
void TestForms::restoresLegacyDataEmbeddedInXml()
{
    Fixture f;
    auto* form = f.create<FormDataView>(ProjectFormKind::Data);
    const QByteArray xml = R"xml(
<FormDataView Title="Old Data" DataType="UInt16" RegisterOrder="LSRF" ByteOrder="Direct">
 <DataViewDefinitions DeviceId="1" PointType="HoldingRegisters" PointAddress="1" Length="3"/>
 <AddressDescriptionMap><Description DeviceId="1" Type="4" Address="0">Sensor</Description></AddressDescriptionMap>
 <AddressColorMap><Color DeviceId="1" Type="4" Address="0" Value="#112233"/></AddressColorMap>
 <ModbusDataUnit><Value Address="1">42</Value><Value Address="2">7</Value></ModbusDataUnit>
</FormDataView>)xml";
    QVERIFY(restoreXml(form, xml));
    QCOMPARE(form->registerOrder(), RegisterOrder::LSRF);
    const auto data = f.Server.data(1, QModbusDataUnit::HoldingRegisters, 0, 3);
    QCOMPARE(data.value(0), quint16(42));
    QCOMPARE(data.value(1), quint16(7));
}


/// \brief Mirrors script documents in a split panel and preserves independent traffic views.
void TestForms::splitViewsKeepScriptAndTrafficState()
{
    Fixture f;
    f.Mdi.resize(900, 500);
    f.Mdi.setViewMode(QMdiArea::TabbedView);
    f.Mdi.show();
    auto* script = f.create<FormScriptView>(ProjectFormKind::Script);
    script->setScript(QStringLiteral("var pressure = 42;"));
    script->setFormName(QStringLiteral("Automation"));
    f.Mdi.setActiveSubWindow(qobject_cast<QMdiSubWindow*>(script->parentWidget()));
    f.Mdi.setSplitViewEnabled(true);
    QVERIFY(f.Project.isSplitTabbedView());
    QCOMPARE(f.Project.duplicatePrimaryTabsToSecondary(), 1);
    auto* secondary = f.Project.secondaryArea();
    QVERIFY(secondary);
    QCOMPARE(secondary->localSubWindowList().size(), 1);
    auto* peer = qobject_cast<FormScriptView*>(secondary->localSubWindowList().first()->widget());
    QVERIFY(peer);
    QCOMPARE(peer->script(), script->script());
    script->setScript(QStringLiteral("var pressure = 100;"));
    QCOMPARE(peer->script(), script->script());
    script->setFormName(QStringLiteral("Shared"));
    QCOMPARE(peer->windowTitle(), QStringLiteral("Shared"));
    script->setPeerRunning(true);
    QVERIFY(f.Project.isScriptRunningOnSplitPair(script));
    f.Project.updateSplitPairScriptIcons(script);
    script->setPeerRunning(false);
    f.Project.removeSplitAutoClonesFromSecondary();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(secondary->localSubWindowList().isEmpty());
    f.Mdi.setSplitViewEnabled(false);
    auto* traffic = f.create<FormTrafficView>(ProjectFormKind::Traffic);
    f.Mdi.setActiveSubWindow(qobject_cast<QMdiSubWindow*>(traffic->parentWidget()));
    f.Mdi.setSplitViewEnabled(true);
    QCOMPARE(f.Project.duplicatePrimaryTabsToSecondary(), 1);
    secondary = f.Project.secondaryArea();
    auto* trafficPeer = qobject_cast<FormTrafficView*>(secondary->localSubWindowList().first()->widget());
    QVERIFY(trafficPeer);
    auto dd = traffic->displayDefinition();
    dd.UnitFilter = 7;
    traffic->setDisplayDefinition(dd);
    QVERIFY(trafficPeer->displayDefinition().UnitFilter != 7);
    QVERIFY(f.Project.canMoveFormToOtherPanel(traffic));
    f.Project.moveFormToOtherPanel(traffic);
    QCOMPARE(f.Mdi.subWindowList().size(), 2);
}


/// \brief Tracks tab titles and reports a drag released outside the workspace.
void TestForms::tabBarTracksAndDragsWindows()
{
    QWidget panel;
    panel.resize(400, 200);
    MdiTabBar tabs(&panel);
    tabs.setGeometry(0, 0, 400, 30);
    QMdiSubWindow first;
    QMdiSubWindow second;
    first.setWindowTitle(QStringLiteral("First[*]"));
    second.setWindowTitle(QStringLiteral("Second"));
    tabs.addSubWindow(&first);
    tabs.addSubWindow(&second);
    tabs.addSubWindow(&first);
    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.currentSubWindow(), &first);
    QCOMPARE(tabs.tabText(tabs.indexOfSubWindow(&first)), QStringLiteral("First"));
    first.setWindowTitle(QStringLiteral("Renamed"));
    QCOMPARE(tabs.tabText(tabs.indexOfSubWindow(&first)), QStringLiteral("Renamed"));
    tabs.setCurrentSubWindow(&second);
    QCOMPARE(tabs.currentSubWindow(), &second);
    QVERIFY(!tabs.subWindowAt(-1));
    panel.show();
    QCoreApplication::processEvents();
    QSignalSpy dragged(&tabs, &MdiTabBar::tabDraggedOutside);
    const QPoint inside = tabs.tabRect(tabs.indexOfSubWindow(&first)).center();
    QTest::mousePress(&tabs, Qt::LeftButton, {}, inside);
    const QPoint outside(450, 250);
    QMouseEvent move(QEvent::MouseMove, outside, tabs.mapToGlobal(outside), Qt::NoButton,
                     Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&tabs, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, outside, tabs.mapToGlobal(outside), Qt::LeftButton,
                        Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&tabs, &release);
    QCOMPARE(dragged.count(), 1);
    QCOMPARE(dragged.first().first().value<QMdiSubWindow*>(), &first);
    tabs.removeSubWindow(&first);
    QCOMPARE(tabs.count(), 1);
    QCOMPARE(tabs.indexOfSubWindow(&first), -1);
}


namespace {
/// \brief Selects a file in the next standard file dialog.
void selectNextFile(const QString& path)
{
    QTimer::singleShot(0, [path] {
        if (auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget())) {
            dialog->selectFile(path);
            QMetaObject::invokeMethod(dialog, "accept");
        }
    });
}
}

/// \brief Saves, imports a script and respects cancellation before closing a project.
void TestForms::mainWindowSavesImportsAndConfirms()
{
    QTemporaryDir profile;
    const QString scriptPath = profile.filePath(QStringLiteral("example.js"));
    QFile scriptFile(scriptPath);
    QVERIFY(scriptFile.open(QIODevice::WriteOnly));
    scriptFile.write("var imported = 42;");
    scriptFile.close();
    auto& prefs = AppPreferences::instance();
    const bool welcome = prefs.showWelcomeDialog();
    prefs.setShowWelcomeDialog(false);
    MainWindow host(profile.path(), false);
    control<QAction>(&host, "actionNewDataView")->trigger();
    const QString path = profile.filePath(QStringLiteral("saved.omsim"));
    selectNextFile(path);
    QVERIFY(QMetaObject::invokeMethod(&host, "on_actionSaveProjectAs_triggered"));
    QVERIFY(QFile::exists(path));
    selectNextFile(scriptPath);
    QVERIFY(QMetaObject::invokeMethod(&host, "on_actionImportScript_triggered"));
    auto* script = host.findChild<FormScriptView*>();
    QVERIFY(script);
    QCOMPARE(script->script(), QStringLiteral("var imported = 42;"));
    confirm(QMessageBox::Cancel);
    QVERIFY(!host.closeProject());
    confirm(QMessageBox::Save);
    QVERIFY(host.closeProject());
    QVERIFY(host.loadProject(path));
    QVERIFY(host.findChild<FormScriptView*>());
    QVERIFY(host.closeProject());
    prefs.setShowWelcomeDialog(welcome);
}


/// \brief Tracks connected endpoints and persists the settings offered by connection menus.
void TestForms::connectionMenusAndStatusFollowServer()
{
    ModbusMultiServer server;
    MenuConnect connectMenu(MenuConnect::ConnectMenu, server);
    MenuConnect disconnectMenu(MenuConnect::DisconnectMenu, server);
    MainStatusBar status(server);
    status.setCheckForUpdates(false);
    status.show();
    ConnectionDetails tcp;
    tcp.Type = ConnectionType::Tcp;
    tcp.TcpParams.IPAddress = QStringLiteral("127.0.0.1");
    tcp.TcpParams.ServicePort = 1502;
    ConnectionDetails rtu = tcp;
    rtu.Type = ConnectionType::RtuTcp;
    ConnectionDetails serial;
    serial.Type = ConnectionType::Serial;
    serial.SerialParams.PortName = QStringLiteral("COM77");
    QVERIFY(connectMenu.canConnect(tcp));
    QVERIFY(connectMenu.canConnect(rtu));
    connectMenu.updateConnectionDetails({tcp, rtu, serial});
    QTemporaryDir dir;
    QSettings settings(dir.filePath("connections.ini"), QSettings::IniFormat);
    settings << &connectMenu;
    MenuConnect restored(MenuConnect::ConnectMenu, server);
    settings >> &restored;
    ConnectionDetails selected;
    QObject::connect(&restored, &MenuConnect::connectAction, &restored,
                     [&selected](ConnectionDetails& value) { selected = value; });
    static_cast<QMenu&>(restored).actions().first()->trigger();
    QCOMPARE(selected.TcpParams.ServicePort, quint16(1502));
    QCOMPARE(static_cast<QMenu&>(disconnectMenu).actions().size(), 0);
    emit server.connected(tcp);
    emit server.connected(rtu);
    emit server.connected(serial);
    QCOMPARE(static_cast<QMenu&>(disconnectMenu).actions().size(), 3);
    QSignalSpy disconnected(&disconnectMenu, &MenuConnect::disconnectAction);
    static_cast<QMenu&>(disconnectMenu).actions().first()->trigger();
    QCOMPARE(disconnected.count(), 1);
    emit server.deviceIdsChanged({1, 7});
    ModbusDefinitions definitions;
    definitions.ErrorSimulations.setNoResponse(true);
    definitions.ErrorSimulations.setResponseIncorrectId(true);
    definitions.ErrorSimulations.setResponseIllegalFunction(true);
    definitions.ErrorSimulations.setResponseDeviceBusy(true);
    definitions.ErrorSimulations.setResponseIncorrectCrc(true);
    definitions.ErrorSimulations.setResponseDelay(true);
    definitions.ErrorSimulations.setResponseDelayTime(100);
    emit server.definitionsChanged(definitions);
    emit server.snifferModeChanged(true);
    emit server.request({});
    emit server.response({}, {});
    emit server.clientConnected(ModbusClientInfo());
    translate(&status);
    translate(&connectMenu);
    translate(&disconnectMenu);
    QVERIFY(!status.grab().isNull());
    emit server.disconnected(tcp);
    emit server.disconnected(rtu);
    emit server.disconnected(serial);
    QCOMPARE(static_cast<QMenu&>(disconnectMenu).actions().size(), 0);
    emit server.snifferModeChanged(false);
    definitions.ErrorSimulations = ModbusErrorSimulations();
    emit server.definitionsChanged(definitions);
}


namespace {
/// \brief Chooses a context-menu action and optionally confirms its follow-up dialog.
void chooseMenuAction(int index, bool confirmDelete = false)
{
    QTimer::singleShot(0, [index, confirmDelete] {
        if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget())) {
            if (confirmDelete)
                confirm(QMessageBox::Yes);
            QTest::mouseClick(menu, Qt::LeftButton, {}, menu->actionGeometry(menu->actions().at(index)).center());
        }
    });
}
}

/// \brief Dispatches per-script and group commands from the project tree.
void TestForms::scriptTreeContextCommands()
{
    Fixture f;
    auto* script = f.create<FormScriptView>(ProjectFormKind::Script);
    script->setFormName(QStringLiteral("Tree script"));
    script->setScript(QStringLiteral("var x = 1;"));
    f.Tree.updateFormTitle(script);
    f.Tree.resize(450, 450);
    f.Tree.show();
    f.Tree.expandAll();
    QCoreApplication::processEvents();
    const auto items = f.Tree.findItems(QStringLiteral("Tree script"), Qt::MatchRecursive);
    QCOMPARE(items.size(), 1);
    const QPoint itemPos = f.Tree.visualItemRect(items.first()).center();
    const QPoint rootPos = f.Tree.visualItemRect(f.Tree.topLevelItem(3)).center();
    QSignalSpy run(&f.Tree, &ProjectTreeWidget::formRunScriptRequested);
    QSignalSpy stop(&f.Tree, &ProjectTreeWidget::formStopScriptRequested);
    QSignalSpy runAll(&f.Tree, &ProjectTreeWidget::runAllScriptsRequested);
    QSignalSpy stopAll(&f.Tree, &ProjectTreeWidget::stopAllScriptsRequested);
    QSignalSpy deleteAll(&f.Tree, &ProjectTreeWidget::deleteAllFormsRequested);
    chooseMenuAction(5);
    QVERIFY(QMetaObject::invokeMethod(&f.Tree, "on_contextMenu", Q_ARG(QPoint, itemPos)));
    QCOMPARE(run.count(), 1);
    f.Tree.setFormScriptRunning(script, true);
    chooseMenuAction(6);
    QVERIFY(QMetaObject::invokeMethod(&f.Tree, "on_contextMenu", Q_ARG(QPoint, itemPos)));
    QCOMPARE(stop.count(), 1);
    f.Tree.setFormScriptRunning(script, false);
    chooseMenuAction(7);
    QVERIFY(QMetaObject::invokeMethod(&f.Tree, "on_contextMenu", Q_ARG(QPoint, rootPos)));
    QCOMPARE(runAll.count(), 1);
    script->setPeerRunning(true);
    chooseMenuAction(8);
    QVERIFY(QMetaObject::invokeMethod(&f.Tree, "on_contextMenu", Q_ARG(QPoint, rootPos)));
    QCOMPARE(stopAll.count(), 1);
    script->setPeerRunning(false);
    chooseMenuAction(5, true);
    QVERIFY(QMetaObject::invokeMethod(&f.Tree, "on_contextMenu", Q_ARG(QPoint, rootPos)));
    QCOMPARE(deleteAll.count(), 1);
}


/// \brief Persists script text, run settings and cursor state in the document XML.
void TestForms::scriptDocumentXmlRoundTrip()
{
    ScriptDocument original(QStringLiteral("Controller"));
    ScriptSettings settings;
    settings.Mode = RunMode::Once;
    settings.Interval = 1500;
    settings.RunOnStartup = true;
    original.setSettings(settings);
    original.setScript(QStringLiteral("if (a < b) { value = 42; }"));
    original.setCursorPosition(8);
    original.setScrollPosition(3);
    QByteArray bytes;
    QXmlStreamWriter writer(&bytes);
    writer << &original;
    ScriptDocument restored(QStringLiteral("Other"));
    QSignalSpy nameChanged(&restored, &ScriptDocument::nameChanged);
    QSignalSpy settingsChanged(&restored, &ScriptDocument::settingsChanged);
    QXmlStreamReader reader(bytes);
    QVERIFY(reader.readNextStartElement());
    reader >> &restored;
    QVERIFY(!reader.hasError());
    QCOMPARE(restored.name(), original.name());
    QCOMPARE(restored.script(), original.script());
    QCOMPARE(restored.settings(), original.settings());
    QCOMPARE(restored.cursorPosition(), 8);
    QCOMPARE(restored.scrollPosition(), 3);
    QCOMPARE(nameChanged.count(), 1);
    QCOMPARE(settingsChanged.count(), 1);
    QXmlStreamReader invalid(QByteArray("<Other/>"));
    QVERIFY(invalid.readNextStartElement());
    invalid >> &restored;
    QCOMPARE(restored.name(), original.name());
}

/// \brief Renders selected and wrapped HTML and accepts empty list items.
void TestForms::htmlDelegateRendersSelections()
{
    QListView view;
    QStandardItemModel model(3, 1);
    model.setData(model.index(0, 0), QStringLiteral("<b>Pressure</b> <i>normal</i>"));
    model.setData(model.index(1, 0), QStringLiteral("<span>Temperature</span>"));
    view.setModel(&model);
    view.setItemDelegate(new HtmlDelegate(&view));
    view.setWordWrap(true);
    view.resize(300, 150);
    view.show();
    view.selectionModel()->select(model.index(0, 0), QItemSelectionModel::Select);
    QVERIFY(view.sizeHintForIndex(model.index(0, 0)).height() > 0);
    QVERIFY(view.sizeHintForIndex(model.index(2, 0)).height() > 0);
    QVERIFY(!view.grab().isNull());
}


/// \brief Navigates search results in help content using the keyboard and buttons.
void TestForms::helpSearchNavigatesAndCloses()
{
    HelpWidget help;
    auto* browser = help.findChild<HelpBrowser*>();
    auto* search = help.findChild<QLineEdit*>();
    QVERIFY(browser && search);
    browser->setHtml(QStringLiteral("<p>pressure temperature pressure</p>"));
    help.resize(400, 300);
    help.show();
    help.showFind();
    QVERIFY(search->isVisible());
    QTest::keyClicks(search, "pressure");
    QCOMPARE(browser->textCursor().selectedText(), QStringLiteral("pressure"));
    QTest::keyClick(search, Qt::Key_Return);
    QCOMPARE(browser->textCursor().selectedText(), QStringLiteral("pressure"));
    QTest::keyClick(search, Qt::Key_Return, Qt::ShiftModifier);
    QCOMPARE(browser->textCursor().selectedText(), QStringLiteral("pressure"));
    translate(&help);
    QTest::keyClick(search, Qt::Key_Escape);
    QVERIFY(!search->isVisible());
    QTest::keyClick(&help, Qt::Key_F, Qt::ControlModifier);
    QVERIFY(search->isVisible());
}


/// \brief Writes edited values and routes simulation updates into the matching data view.
void TestForms::dataViewWritesAndSimulatesValues()
{
    Fixture f;
    auto* form = f.create<FormDataView>(ProjectFormKind::Data);
    form->setDataType(DataType::UInt16);
    auto dd = form->displayDefinition();
    dd.DeviceId = 1;
    dd.PointType = QModbusDataUnit::HoldingRegisters;
    dd.PointAddress = 1;
    dd.Length = 8;
    form->setDisplayDefinition(dd);
    QTimer::singleShot(0, [] {
        if (auto* dialog = qobject_cast<DialogWriteRegister*>(QApplication::activeModalWidget())) {
            if (auto* value = dialog->findChild<NumericLineEdit*>(QStringLiteral("lineEditValue")))
                value->setValue(42);
            dialog->accept();
        }
    });
    QVERIFY(QMetaObject::invokeMethod(form, "on_outputWidget_itemDoubleClicked",
                                      Q_ARG(quint16, 1), Q_ARG(QVariant, QVariant(0))));
    QCOMPARE(f.Server.data(1, dd.PointType, 0, 1).value(0), quint16(42));
    emit f.Simulator.dataSimulated(DataType::UInt16, RegisterOrder::MSRF, 1, dd.PointType, 0, QVariant(77));
    QCOMPARE(f.Server.data(1, dd.PointType, 0, 1).value(0), quint16(77));
    emit f.Simulator.simulationStarted(DataType::UInt16, RegisterOrder::MSRF, 1, dd.PointType, {0});
    emit f.Simulator.simulationStopped(DataType::UInt16, RegisterOrder::MSRF, 1, dd.PointType, {0});
    emit f.Simulator.dataSimulated(DataType::UInt16, RegisterOrder::MSRF, 2, dd.PointType, 0, QVariant(99));
    QCOMPARE(f.Server.data(1, dd.PointType, 0, 1).value(0), quint16(77));
    dd.PointType = QModbusDataUnit::Coils;
    form->setDisplayDefinition(dd);
    QTimer::singleShot(0, [] {
        if (auto* dialog = qobject_cast<DialogWriteStatusRegister*>(QApplication::activeModalWidget()))
            dialog->accept();
    });
    QVERIFY(QMetaObject::invokeMethod(form, "on_outputWidget_itemDoubleClicked",
                                      Q_ARG(quint16, 1), Q_ARG(QVariant, QVariant(true))));
}

int main(int argc, char** argv)
{
    Application app(argc, argv);
    TestForms test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_forms.moc"
