// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_appproject.cpp
/// \brief Integration tests for project form and split-view lifecycle management.
///

#include <QFile>
#include <QRegularExpression>
#include <QListView>
#include <QTableView>
#include <QAbstractItemDelegate>
#include <QImage>
#include <QPainter>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QPointer>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "appproject.h"
#include "applogger.h"
#include "apppreferences.h"
#include "application.h"
#include "widgets/mdiareaex.h"
#include "widgets/pointtypecombobox.h"
#include "widgets/numericlineedit.h"
#include "widgets/outputdatawidget.h"
#include "dialogs/dialogwriteregister.h"
#include "widgets/projecttreewidget.h"
#include "datasimulator.h"
#include "dialogs/dialogmodbusdefinitions.h"
#include "formdataview.h"
#include "formdatamapview.h"
#include "mainwindow.h"
#include "modbusmultiserver.h"
#include "projectformmetadata.h"

class TestAppProject : public QObject
{
    Q_OBJECT

private slots:
    void logsEnronAddressSpaces_data();
    void logsEnronAddressSpaces();
    void mapsEnronRequests_data();
    void mapsEnronRequests();
    void editsAndRestoresEnronDataMap();
    void togglesEnronRegisterChoices();
    void compactsEnronRowsAcrossFormatChanges();
    void enronDisplayFormats_data();
    void enronDisplayFormats();
    void enronRegisterChoices_data();
    void enronRegisterChoices();
    void roundTripsEnronRegisterChoices();
    void editsEnronRegisterByLogicalAddress();
    void configuresEnronMode();
    void roundTripsEnronMode();
    void createsAndNumbersEveryFormKind();
    void closesReopensAndDeletesCanonicalForm();
    void respectsDeletionLock();
    void closingProjectResetsSnifferMode();
    void createsAndRemovesSplitClone();
    void roundTripsOpenAndClosedForms();
    void preservesZeroBasedStartingAddressAcrossReopen();
    void preservesZeroBasedStartingAddressZeroAcrossReopen();
    void clampsZeroStartingAddressForOneBasedProject();
    void opensProjectBeforeRestoringServerState();
    void rejectsMalformedProjectWithoutChangingState();
    void loadsProjectWithChildlessDefinitionsAndConnection();
    void preservesXmlCommentsAcrossRoundTrip();
    void keepsCommentsOnRepeatedSave();
    void dropsCommentsAfterCloseProject();
    void omitsTimestampsWhenPreferenceDisabled();
    void splitsUserStateIntoSeparateFile();
    void restoresWindowGeometryFromUserStateFile();
    void keepsTabbedWindowsMaximizedWhenLoadingIntoVisibleArea();
    void opensProjectWithoutUserStateFile();
    void removesStaleUserStateFile();
    void roundTripsSeveralFormsOfTheSameKind();
    void storesUserStateOnCloseWithoutSavingProject();
    void opensShippedDemoProjects();
    void keepsShippedDemoProjectsFreeOfUserState();
};

namespace {

struct ProjectFixture
{
    QTemporaryDir Profile;
    MainWindow Host{Profile.path(), false};
    MdiAreaEx Mdi;
    ModbusMultiServer Server;
    DataSimulator Simulator;
    ProjectTreeWidget Tree;
    AppProject Project{&Mdi, Server, &Simulator, &Tree, &Host};
};

///
/// \brief The project text used by the comment tests, annotated in the prolog, between
/// elements, inside a container and in the epilog.
///
const char* kAnnotatedProject =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!-- Pump station, hand written -->\n"
    "<OpenModSim Version=\"2.0-dev\">\n"
    "    <ModbusDefinitions AddrSpace=\"6-Digits\"/>\n"
    "    <!-- network settings -->\n"
    "    <Connections>\n"
    "        <ConnectionDetails ConnectionType=\"Tcp\">\n"
    "            <TcpConnectionParams IPAddress=\"0.0.0.0\" ServicePort=\"502\"/>\n"
    "        </ConnectionDetails>\n"
    "    </Connections>\n"
    "    <AddressSpace>\n"
    "        <AddressDescriptionMap>\n"
    "            <!-- telemetry block -->\n"
    "            <Description DeviceId=\"1\" Type=\"4\" Address=\"0\"><![CDATA[Sine]]></Description>\n"
    "        </AddressDescriptionMap>\n"
    "    </AddressSpace>\n"
    "    <ViewSettings ViewMode=\"1\" SplitView=\"0\"/>\n"
    "    <Forms>\n"
    "        <FormDataView Panel=\"L\" Title=\"Data1\" DataType=\"UInt16\" RegisterOrder=\"MSRF\" Codepage=\"\" ByteOrder=\"Direct\">\n"
    "            <Window Maximized=\"true\" Minimized=\"false\" Left=\"0\" Top=\"0\" Width=\"610\" Height=\"331\"/>\n"
    "            <DataViewDefinitions DeviceId=\"1\" PointType=\"HoldingRegisters\" PointAddress=\"1\" Length=\"8\" DataViewColumnsDistance=\"25\" LeadingZeros=\"true\"/>\n"
    "            <AddressColorMap/>\n"
    "        </FormDataView>\n"
    "    </Forms>\n"
    "</OpenModSim>\n"
    "<!-- end of project -->\n";

///
/// \brief A tabbed project whose forms carry a restored, offset window geometry.
///
const char* kTabbedProjectWithOffsetWindows =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<OpenModSim Version=\"2.0-dev\">\n"
    "    <ModbusDefinitions AddrSpace=\"6-Digits\"/>\n"
    "    <ViewSettings ViewMode=\"1\" SplitView=\"0\"/>\n"
    "    <Forms>\n"
    "        <FormDataView Panel=\"L\" Title=\"Data1\" DataType=\"UInt16\" RegisterOrder=\"MSRF\" Codepage=\"\" ByteOrder=\"Direct\">\n"
    "            <Window Maximized=\"false\" Minimized=\"false\" Left=\"320\" Top=\"125\" Width=\"400\" Height=\"300\"/>\n"
    "            <DataViewDefinitions DeviceId=\"1\" PointType=\"HoldingRegisters\" PointAddress=\"1\" Length=\"8\" DataViewColumnsDistance=\"1\"/>\n"
    "        </FormDataView>\n"
    "        <FormDataMapView Panel=\"L\" Title=\"Map1\">\n"
    "            <Window Maximized=\"false\" Minimized=\"false\" Left=\"320\" Top=\"125\" Width=\"400\" Height=\"300\"/>\n"
    "            <DataMapViewDefinitions ZeroBasedAddress=\"false\" HexView=\"false\"/>\n"
    "        </FormDataMapView>\n"
    "    </Forms>\n"
    "</OpenModSim>\n";

///
/// \brief The demo projects shipped with the application, with the number of forms each
/// one is expected to open.
/// \return One entry per demo, as path and form count.
///
QList<QPair<QString, int>> demoProjects()
{
    const QString directory = QStringLiteral(OMODSIM_DEMOS_DIR);
    return {
        { directory + QStringLiteral("/demo_wave_generator.omsim"), 3 },
        { directory + QStringLiteral("/demo_plc_simulator.omsim"), 6 }
    };
}

///
/// \brief Writes a project file.
/// \param path Destination path.
/// \param content Project text.
///
void writeProjectFile(const QString& path, const char* content)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(content);
}

///
/// \brief Describes where each comment of a project file sits, as "parent path|following
/// element|text". The following element is "END" for a comment closing its parent.
/// \param path The project file to inspect.
/// \return One entry per comment, in document order.
///
QStringList commentPlacements(const QString& path)
{
    QStringList placements;
    QStringList elements;
    QStringList pending;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return placements;

    QXmlStreamReader xml(&file);
    const auto flush = [&placements, &pending, &elements](const QString& following) {
        for (const auto& text : pending)
            placements.append(elements.join(QLatin1Char('/')) + QLatin1Char('|') + following
                              + QLatin1Char('|') + text.trimmed());
        pending.clear();
    };

    while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::Comment:
            pending.append(xml.text().toString());
            break;
        case QXmlStreamReader::StartElement:
            flush(xml.name().toString());
            elements.append(xml.name().toString());
            break;
        case QXmlStreamReader::EndElement:
            flush(QStringLiteral("END"));
            elements.removeLast();
            break;
        default:
            break;
        }
    }
    flush(QStringLiteral("END"));

    return placements;
}

}

/// \brief Keeps ordinary tables distinguishable from Enron maps at the same backing addresses.
void TestAppProject::logsEnronAddressSpaces_data()
{
    QTest::addColumn<int>("group");
    QTest::newRow("holding") << 0;
    QTest::newRow("Int32") << 1;
    QTest::newRow("Float32") << 2;
}

/// \brief Logs logical groups, addresses and counts on both registration and removal.
void TestAppProject::logsEnronAddressSpaces()
{
    QFETCH(int, group);
    ModbusMultiServer server;
    QObject context;
    AppLogger::setupModbusMultiServerLogging(server, &context);
    auto definitions = server.getModbusDefinitions();
    definitions.EnronModbus = true;
    server.setModbusDefinitions(definitions);
    const auto enron = static_cast<EnronModbus::RegisterType>(group);
    const int address = group == 2 ? 7003 : 5003;
    const int logicalAddress = group ? address - 1 : address;
    const int count = group ? 2 : 4;
    const QString name = group
        ? QCoreApplication::translate("DataMapDataModel", group == 1 ? "Enron Registers Int32" : "Enron Registers Float32")
        : registerTypeName(QModbusDataUnit::HoldingRegisters);
    const auto id = QUuid::createUuid();
    const QString added = QCoreApplication::translate("MainWindow",
        "Address space added: unit %1, %2, starting address %3, length %4")
        .arg(1).arg(name).arg(logicalAddress).arg(count);
    QTest::ignoreMessage(QtInfoMsg, QRegularExpression(QRegularExpression::escape(added)));
    server.addUnitMap(id, 1, QModbusDataUnit::HoldingRegisters, address, 4, enron);
    QVERIFY(server.data(1, QModbusDataUnit::HoldingRegisters, address, 4).isValid());
    const QString removed = QCoreApplication::translate("MainWindow",
        "Address space removed: unit %1, %2, starting address %3, length %4")
        .arg(1).arg(name).arg(logicalAddress).arg(count);
    QTest::ignoreMessage(QtInfoMsg, QRegularExpression(QRegularExpression::escape(removed)));
    server.removeUnitMap(id, 1);
    const QString ordinaryAdded = QCoreApplication::translate("MainWindow",
        "Address space added: unit %1, %2, starting address %3, length %4")
        .arg(1).arg(registerTypeName(QModbusDataUnit::HoldingRegisters)).arg(address).arg(4);
    QTest::ignoreMessage(QtInfoMsg, QRegularExpression(QRegularExpression::escape(ordinaryAdded)));
    server.addUnitMap(id, 1, QModbusDataUnit::HoldingRegisters, address, 4);
    server.clearAddressSpace();
    QTest::ignoreMessage(QtInfoMsg, QRegularExpression(QRegularExpression::escape(ordinaryAdded)));
    server.addUnitMap(id, 1, QModbusDataUnit::HoldingRegisters, address, 4);
    server.removeUnitMap(id, 1);
}

/// \brief Covers request mapping for both Enron ranges and ordinary Modbus.
void TestAppProject::mapsEnronRequests_data()
{
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<int>("base");
    QTest::addColumn<int>("group");
    QTest::newRow("ordinary") << false << 7001 << 0;
    QTest::newRow("5x") << true << 5001 << 1;
    QTest::newRow("7x") << true << 7001 << 2;
}

/// \brief Maps consecutive protocol addresses and appends the next native register.
void TestAppProject::mapsEnronRequests()
{
    QFETCH(bool, enabled);
    QFETCH(int, base);
    QFETCH(int, group);
    ProjectFixture fixture;
    auto definitions = fixture.Server.getModbusDefinitions();
    definitions.EnronModbus = enabled;
    fixture.Server.setModbusDefinitions(definitions);
    auto* form = qobject_cast<FormDataMapView*>(fixture.Project.createMdiChild(ProjectFormKind::DataMap));
    QVERIFY(form);
    form->setAutoAddOnRequest(true);
    auto* model = form->findChild<DataMapDataModel*>();
    auto* table = form->findChild<QTableView*>("tableView");
    QVERIFY(model);
    QVERIFY(table);
    fixture.Server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, base, 6);
    const QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, base,
                              QVector<quint16>{0x1234, 0x5678, 0x2345, 0x6789, 0, 0});
    fixture.Server.setData(1, unit, WriteSource::User);
    const auto request = ModbusMessage::create(QModbusRequest(QModbusPdu::ReadHoldingRegisters,
        quint16(base), quint16(2)), ModbusMessage::Tcp, 1, 1, QDateTime::currentDateTime(), true);
    fixture.Server.requestOnConnection({}, request);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->keyForRow(0).Address, base);
    QCOMPARE(model->keyForRow(1).Address, base + (enabled ? 2 : 1));
    QCOMPARE(model->index(1, ColType).data(DataMapRole::EnronType).toInt(), group);
    QCOMPARE(model->index(1, ColAddress).data().toInt(), base + 1 + (enabled || form->zeroBasedAddress() ? 0 : 1));
    table->setCurrentIndex(table->model()->index(1, ColType));
    QVERIFY(QMetaObject::invokeMethod(form, "on_actionAdd_triggered", Qt::DirectConnection));
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->keyForRow(2).Address, base + (enabled ? 4 : 2));
    QCOMPARE(model->index(2, ColType).data(DataMapRole::EnronType).toInt(), group);
    fixture.Server.requestOnConnection({}, request);
    QCOMPARE(model->rowCount(), 3);
}

/// \brief Preserves Enron groups and protocol addresses through table editing and project reload.
void TestAppProject::editsAndRestoresEnronDataMap()
{
    ProjectFixture fixture;
    auto* form = qobject_cast<FormDataMapView*>(fixture.Project.createMdiChild(ProjectFormKind::DataMap));
    QVERIFY(form);
    auto* model = form->findChild<DataMapDataModel*>();
    auto* table = form->findChild<QTableView*>("tableView");
    auto* filter = form->findChild<QComboBox*>("filterTypeCombo");
    QVERIFY(model);
    QVERIFY(table);
    QVERIFY(filter);
    model->addEntry({1, QModbusDataUnit::HoldingRegisters, 0}, {});
    const auto typeIndex = table->model()->index(0, ColType);
    auto* delegate = table->itemDelegateForColumn(ColType);
    QScopedPointer<QWidget> editor(delegate->createEditor(table, {}, typeIndex));
    auto* types = qobject_cast<PointTypeComboBox*>(editor.data());
    QVERIFY(types);
    QCOMPARE(types->count(), 4);
    QCOMPARE(filter->count(), 5);

    auto definitions = fixture.Server.getModbusDefinitions();
    definitions.EnronModbus = true;
    fixture.Server.setModbusDefinitions(definitions);
    QTRY_COMPARE(types->count(), 6);
    QCOMPARE(filter->count(), 7);
    QCOMPARE(filter->itemText(5), QCoreApplication::translate("FormDataMapView", "Enron Registers Int32"));
    QCOMPARE(filter->itemText(6), QCoreApplication::translate("FormDataMapView", "Enron Registers Float32"));
    types->setCurrentEnronType(EnronModbus::RegisterType::Group7x);
    delegate->setModelData(types, table->model(), typeIndex);
    QCOMPARE(model->index(0, ColType).data(DataMapRole::EnronType).toInt(), 2);
    QCOMPARE(model->index(0, ColAddress).data().toInt(), 7001);

    const auto addressIndex = table->model()->index(0, ColAddress);
    auto* addressDelegate = table->itemDelegateForColumn(ColAddress);
    QScopedPointer<QWidget> addressEditor(addressDelegate->createEditor(table, {}, addressIndex));
    auto* address = qobject_cast<NumericLineEdit*>(addressEditor.data());
    QVERIFY(address);
    addressDelegate->setEditorData(address, addressIndex);
    QCOMPARE(address->range<int>().from(), 7001);
    QCOMPARE(address->range<int>().to(), 7999);
    QCOMPARE(address->value<int>(), 7001);
    address->setValue(7002);
    addressDelegate->setModelData(address, table->model(), addressIndex);
    QCOMPARE(model->keyForRow(0).Address, 7003);
    QCOMPARE(model->index(0, ColAddress).data().toInt(), 7002);
    QVERIFY(model->setData(model->index(0, ColDataType), "UInt32"));
    QVERIFY(model->setData(model->index(0, ColValue), "305419896"));

    QTemporaryDir files;
    const auto saved = files.filePath("enron-map.omsim");
    QVERIFY(fixture.Project.saveProject(saved));
    filter->setCurrentIndex(6);
    QCOMPARE(table->model()->rowCount(), 1);
    definitions.EnronModbus = false;
    fixture.Server.setModbusDefinitions(definitions);
    QTRY_COMPARE(types->count(), 4);
    QCOMPARE(filter->count(), 5);
    QCOMPARE(filter->currentIndex(), 0);
    QCOMPARE(table->model()->rowCount(), 1);
    definitions.EnronModbus = true;
    fixture.Server.setModbusDefinitions(definitions);
    QTRY_COMPARE(types->count(), 6);
    QCOMPARE(model->index(0, ColAddress).data().toInt(), 7002);
    editor.reset();
    addressEditor.reset();

    fixture.Project.closeProject();
    QVERIFY(fixture.Project.loadProject(saved).Success);
    auto* restored = fixture.Mdi.findChild<FormDataMapView*>();
    QVERIFY(restored);
    auto* restoredModel = restored->findChild<DataMapDataModel*>();
    QVERIFY(restoredModel);
    QCOMPARE(restoredModel->rowCount(), 1);
    QCOMPARE(restoredModel->index(0, ColType).data(DataMapRole::EnronType).toInt(), 2);
    QCOMPARE(restoredModel->index(0, ColAddress).data().toInt(), 7002);
    QCOMPARE(restoredModel->keyForRow(0).Address, 7003);
    QCOMPARE(restoredModel->index(0, ColDataType).data().toString(), QString("UInt32"));
    const auto words = fixture.Server.data(1, QModbusDataUnit::HoldingRegisters, 7003, 2);
    QCOMPARE(words.value(0), quint16(0x1234));
    QCOMPARE(words.value(1), quint16(0x5678));
}

/// \brief Changes the available choices without duplicates or losing ordinary selections.
void TestAppProject::togglesEnronRegisterChoices()
{
    PointTypeComboBox combo;
    QCOMPARE(combo.count(), 4);
    combo.setCurrentPointType(QModbusDataUnit::InputRegisters);
    combo.setCurrentEnronType(EnronModbus::RegisterType::Group7x);
    QCOMPARE(combo.currentPointType(), QModbusDataUnit::InputRegisters);
    QCOMPARE(combo.currentEnronType(), EnronModbus::RegisterType::None);
    combo.setEnronEnabled(false);
    QCOMPARE(combo.count(), 4);

    for (auto mode : {EnronModbus::RegisterType::Group5x, EnronModbus::RegisterType::Group7x}) {
        combo.setCurrentPointType(QModbusDataUnit::InputRegisters);
        combo.setEnronEnabled(true);
        combo.setEnronEnabled(true);
        QCOMPARE(combo.count(), 6);
        QCOMPARE(combo.currentPointType(), QModbusDataUnit::InputRegisters);
        combo.setCurrentEnronType(mode);
        QCOMPARE(combo.currentEnronType(), mode);
        QCOMPARE(combo.currentText(), mode == EnronModbus::RegisterType::Group5x
            ? QCoreApplication::translate("PointTypeComboBox", "Enron Registers Int32 (Read-Write)")
            : QCoreApplication::translate("PointTypeComboBox", "Enron Registers Float32 (Read-Write)"));
        combo.setEnronEnabled(false);
        combo.setEnronEnabled(false);
        QCOMPARE(combo.count(), 4);
        QCOMPARE(combo.currentPointType(), QModbusDataUnit::HoldingRegisters);
        QCOMPARE(combo.currentEnronType(), EnronModbus::RegisterType::None);
        QCOMPARE(combo.findData(int(mode), Qt::UserRole + 1), -1);
    }
}

/// \brief Rebuilds visible rows across display widths without changing the stored range.
void TestAppProject::compactsEnronRowsAcrossFormatChanges()
{
    ProjectFixture fixture;
    auto definitions = fixture.Server.getModbusDefinitions();
    definitions.EnronModbus = true;
    fixture.Server.setModbusDefinitions(definitions);
    auto* form = qobject_cast<FormDataView*>(fixture.Project.createMdiChild(ProjectFormKind::Data));
    QVERIFY(form);
    form->findChild<PointTypeComboBox*>("comboBoxModbusPointType")->setCurrentEnronType(EnronModbus::RegisterType::Group5x);
    form->findChild<NumericLineEdit*>("lineEditLength")->setValue(3);
    auto* output = form->findChild<OutputDataWidget*>("outputWidget");
    auto* view = output->findChild<QListView*>("listView");
    QCOMPARE(form->displayDefinition().Length, quint16(6));
    QCOMPARE(view->model()->rowCount(), 3);
    form->setDataType(DataType::UInt64);
    QCOMPARE(view->model()->rowCount(), 2);
    QVERIFY(view->model()->data(view->model()->index(1, 0), CaptureRole).toString().trimmed().isEmpty());
    form->setDataType(DataType::UInt16);
    QCOMPARE(view->model()->rowCount(), 6);
    form->setDataType(DataType::UInt32);
    QCOMPARE(view->model()->rowCount(), 3);
    QCOMPARE(form->displayDefinition().Length, quint16(6));
    QCOMPARE(form->data().size(), 6);
    definitions.EnronModbus = false;
    fixture.Server.setModbusDefinitions(definitions);
    QTRY_COMPARE(form->displayDefinition().EnronType, EnronModbus::RegisterType::None);
    QCOMPARE(view->model()->rowCount(), 6);
}

/// \brief Covers every display action independently of the selected Enron group.
void TestAppProject::enronDisplayFormats_data()
{
    QTest::addColumn<int>("group");
    QTest::addColumn<DataType>("type");
    QTest::addColumn<QString>("action");
    const QList<QPair<DataType, QString>> formats = {
        {DataType::Binary, "actionDisplayBinary"}, {DataType::Hex, "actionDisplayHex"},
        {DataType::Ansi, "actionDisplayAnsi"}, {DataType::Int16, "actionDisplayInt16"},
        {DataType::UInt16, "actionDisplayUInt16"}, {DataType::Int32, "actionDisplayInt32"},
        {DataType::UInt32, "actionDisplayUInt32"}, {DataType::Int64, "actionDisplayInt64"},
        {DataType::UInt64, "actionDisplayUInt64"}, {DataType::Float32, "actionDisplayFloat32"},
        {DataType::Float64, "actionDisplayFloat64"}
    };
    for (int group : {1, 2}) {
        for (const auto& format : formats) {
            const auto name = QString("%1-%2").arg(group).arg(format.second);
            QTest::newRow(qPrintable(name)) << group << format.first << format.second;
        }
    }
}

/// \brief Reinterprets shared register bits without changing their group, address or storage.
void TestAppProject::enronDisplayFormats()
{
    QFETCH(int, group);
    QFETCH(DataType, type);
    QFETCH(QString, action);
    auto& prefs = AppPreferences::instance();
    const bool previousHex = prefs.globalHexView();
    const auto restoreHex = qScopeGuard([&prefs, previousHex] { prefs.setGlobalHexView(previousHex); });
    prefs.setGlobalHexView(false);
    ProjectFixture fixture;
    auto definitions = fixture.Server.getModbusDefinitions();
    definitions.EnronModbus = true;
    fixture.Server.setModbusDefinitions(definitions);
    auto* form = qobject_cast<FormDataView*>(fixture.Project.createMdiChild(ProjectFormKind::Data));
    QVERIFY(form);
    form->setAddressBase(AddressBase::Base0);
    auto* combo = form->findChild<PointTypeComboBox*>("comboBoxModbusPointType");
    const auto enron = static_cast<EnronModbus::RegisterType>(group);
    combo->setCurrentEnronType(enron);
    form->findChild<NumericLineEdit*>("lineEditLength")->setValue(2);
    auto* displayAction = form->findChild<QAction*>(action);
    QVERIFY(displayAction);
    QVERIFY(displayAction->isEnabled());
    displayAction->trigger();
    QCOMPARE(form->dataType(), type);
    QCOMPARE(form->displayDefinition().EnronType, enron);
    QCOMPARE(form->findChild<NumericLineEdit*>("lineEditAddress")->value<int>(), int(EnronModbus::baseAddress(enron)));
    const QVector<quint16> words{0x3f80, 0, 0x4000, 0};
    fixture.Server.setData(1, QModbusDataUnit(QModbusDataUnit::HoldingRegisters,
        EnronModbus::baseAddress(enron), words), WriteSource::User);
    auto* model = form->findChild<OutputDataListModel*>();
    QVERIFY(model);
    QTRY_COMPARE(form->data(), words);
    auto* output = form->findChild<OutputDataWidget*>("outputWidget");
    auto* view = output->findChild<QListView*>("listView");
    QVERIFY(view);
    const int visibleRows = 4 / registersCount(type);
    QCOMPARE(view->model()->rowCount(), visibleRows);
    QCOMPARE(output->rowCount(), visibleRows);
    QSet<QString> visibleAddresses;
    for (int row = 0; row < visibleRows; ++row)
        visibleAddresses.insert(view->model()->data(view->model()->index(row, 0), AddressStringRole).toString());
    QCOMPARE(visibleAddresses.size(), visibleRows);
    QImage page(1000, 1000, QImage::Format_ARGB32);
    QPainter painter(&page);
    QCOMPARE(output->paint(page.rect(), painter, 0), visibleRows);
    const auto index = model->index(0);
    QVERIFY(!model->data(index, CaptureRole).toString().trimmed().isEmpty());
    const QString protocolAddress = QString::number(EnronModbus::baseAddress(enron));
    const bool wordFormat = registersCount(type) == 1;
    QCOMPARE(model->data(index, AddressStringRole).toString(), protocolAddress
             + (wordFormat ? QStringLiteral(".H") : QString()));
    if (wordFormat) {
        QCOMPARE(model->data(model->index(1), AddressStringRole).toString(), protocolAddress + QStringLiteral(".L"));
        QCOMPARE(model->data(index, ValueRole).toUInt(), uint(0x3f80));
    } else if (type == DataType::Float32) {
        QCOMPARE(model->data(index, ValueRole).toFloat(), 1.0f);
        QCOMPARE(model->data(model->index(2), ValueRole).toFloat(), 2.0f);
    } else if (type == DataType::Float64) {
        QCOMPARE(model->data(index, ValueRole).toDouble(), 0x1.000004p-7);
    } else if (registersCount(type) == 2) {
        QCOMPARE(model->data(index, ValueRole).toUInt(), uint(0x3f800000));
    } else {
        QCOMPARE(model->data(index, ValueRole).toULongLong(), qulonglong(0x3f80000040000000));
    }
    if (type == DataType::Float32) {
        output->applyFindByValue(QStringLiteral("2"));
        QVERIFY(output->findNextByValue(QStringLiteral("2")));
        QCOMPARE(view->currentIndex().row(), 1);
        output->clearFindByValue();
    }
    QObject::disconnect(output, &OutputDataWidget::itemDoubleClicked, nullptr, nullptr);
    QSignalSpy editSpy(output, &OutputDataWidget::itemDoubleClicked);
    QVERIFY(QMetaObject::invokeMethod(output, "on_listView_doubleClicked", Qt::DirectConnection,
        Q_ARG(QModelIndex, view->model()->index(visibleRows - 1, 0))));
    QCOMPARE(editSpy.count(), 1);
    QCOMPARE(editSpy.first().at(0).toUInt(), uint(EnronModbus::baseAddress(enron)
        + (visibleRows - 1) * registersCount(type)));
    if (type == DataType::Float32)
        QCOMPARE(editSpy.first().at(1).toFloat(), 2.0f);
    form->setRegisterOrder(RegisterOrder::LSRF);
    QCOMPARE(form->registerOrder(), RegisterOrder::LSRF);
    form->setByteOrder(ByteOrder::Swapped);
    QCOMPARE(form->byteOrder(), ByteOrder::Swapped);
    prefs.setGlobalHexView(true);
    QTRY_COMPARE(model->data(index, AddressStringRole).toString(), QStringLiteral("0x%1").arg(EnronModbus::baseAddress(enron), 4, 16, QLatin1Char('0')).toUpper()
                 + (wordFormat ? QStringLiteral(".H") : QString()));
    QCOMPARE(form->findChild<NumericLineEdit*>("lineEditAddress")->value<int>(), int(EnronModbus::baseAddress(enron)));
}

/// \brief Exercises logical register input for both Enron groups and address bases.
void TestAppProject::enronRegisterChoices_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<bool>("zeroBased");
    for (int mode : {1, 2}) {
        QTest::newRow(mode == 1 ? "int-base0" : "float-base0") << mode << true;
        QTest::newRow(mode == 1 ? "int-base1" : "float-base1") << mode << false;
    }
}

/// \brief Keeps UI counts and labels logical while editing the correct backing words.
void TestAppProject::enronRegisterChoices()
{
    QFETCH(int, mode);
    QFETCH(bool, zeroBased);
    auto& prefs = AppPreferences::instance();
    const bool previousHex = prefs.globalHexView();
    const auto restoreHex = qScopeGuard([&prefs, previousHex] { prefs.setGlobalHexView(previousHex); });
    prefs.setGlobalHexView(false);
    ProjectFixture fixture;
    auto* form = qobject_cast<FormDataView*>(fixture.Project.createMdiChild(ProjectFormKind::Data));
    QVERIFY(form);
    form->setAddressBase(zeroBased ? AddressBase::Base0 : AddressBase::Base1);
    auto* combo = form->findChild<PointTypeComboBox*>("comboBoxModbusPointType");
    auto* address = form->findChild<NumericLineEdit*>("lineEditAddress");
    auto* length = form->findChild<NumericLineEdit*>("lineEditLength");
    QVERIFY(combo);
    const auto enron = static_cast<EnronModbus::RegisterType>(mode);
    QCOMPARE(combo->count(), 4);
    QCOMPARE(combo->findData(mode, Qt::UserRole + 1), -1);
    auto definitions = fixture.Server.getModbusDefinitions();
    definitions.EnronModbus = true;
    fixture.Server.setModbusDefinitions(definitions);
    QTRY_COMPARE(combo->count(), 6);
    QVERIFY(combo->findData(mode, Qt::UserRole + 1) >= 0);
    QSignalSpy maps(&fixture.Server, &ModbusMultiServer::unitMapAdded);
    combo->setCurrentEnronType(enron);
    QTRY_VERIFY(!maps.isEmpty());
    QCOMPARE(maps.last().last().value<EnronModbus::RegisterType>(), enron);
    const int base = EnronModbus::baseAddress(enron);
    QCOMPARE(address->value<int>(), base);
    QCOMPARE(address->range<int>().from(), base);
    address->setValue(base + 1);
    length->setValue(2);
    const auto dd = form->displayDefinition();
    QCOMPARE(dd.EnronType, enron);
    QCOMPARE(dd.PointAddress, quint16(base + 2 + (zeroBased ? 0 : 1)));
    QCOMPARE(dd.Length, quint16(4));
    QCOMPARE(form->dataType(), mode == 1 ? DataType::Int32 : DataType::Float32);
    form->setDataType(DataType::UInt16);
    QCOMPARE(form->dataType(), DataType::UInt16);
    form->setByteOrder(ByteOrder::Swapped);
    QCOMPARE(form->byteOrder(), ByteOrder::Swapped);
    form->setRegisterOrder(RegisterOrder::LSRF);
    QCOMPARE(form->registerOrder(), RegisterOrder::LSRF);
    form->setDataType(DataType::UInt32);
    auto* model = form->findChild<OutputDataListModel*>();
    QVERIFY(model);
    QCOMPARE(model->data(model->index(0), AddressStringRole).toString(), QString::number(base + 1));
    QCOMPARE(model->data(model->index(2), AddressStringRole).toString(), QString::number(base + 2));
    QSignalSpy definitionSpy(form, &FormDataView::definitionChanged);
    definitions.AddrSpace = AddressSpace::Addr5Digits;
    fixture.Server.setModbusDefinitions(definitions);
    QTRY_VERIFY(!definitionSpy.isEmpty());
    QCOMPARE(model->data(model->index(0), AddressStringRole).toString(), QString::number(base + 1));
    QCOMPARE(form->dataType(), DataType::UInt32);
    QCOMPARE(form->byteOrder(), ByteOrder::Swapped);
    QCOMPARE(form->registerOrder(), RegisterOrder::LSRF);
    QCOMPARE(model->data(model->index(2), AddressRole).toUInt(), uint(base + 4 + (zeroBased ? 0 : 1)));

    form->setAddressBase(zeroBased ? AddressBase::Base1 : AddressBase::Base0);
    QCOMPARE(address->value<int>(), base + 1);
    form->setAddressBase(zeroBased ? AddressBase::Base0 : AddressBase::Base1);
    address->setValue(base + EnronModbus::RangeLength - 1);
    QCOMPARE(length->range<int>().to(), 1);
    QCOMPARE(length->value<int>(), 1);
    QCOMPARE(form->displayDefinition().Length, quint16(2));

    definitions.EnronModbus = false;
    fixture.Server.setModbusDefinitions(definitions);
    QTRY_COMPARE(form->displayDefinition().EnronType, EnronModbus::RegisterType::None);
    QCOMPARE(combo->count(), 4);
    QCOMPARE(combo->findData(mode, Qt::UserRole + 1), -1);
    QCOMPARE(form->displayDefinition().Length, quint16(2));
    QCOMPARE(combo->currentPointType(), QModbusDataUnit::HoldingRegisters);
}

/// \brief Preserves the Enron selection and logical range in project and settings serialization.
void TestAppProject::roundTripsEnronRegisterChoices()
{
    ProjectFixture fixture;
    auto definitions = fixture.Server.getModbusDefinitions();
    definitions.EnronModbus = true;
    fixture.Server.setModbusDefinitions(definitions);
    auto* form = qobject_cast<FormDataView*>(fixture.Project.createMdiChild(ProjectFormKind::Data));
    QVERIFY(form);
    form->setAddressBase(AddressBase::Base0);
    form->findChild<PointTypeComboBox*>("comboBoxModbusPointType")->setCurrentEnronType(EnronModbus::RegisterType::Group7x);
    form->findChild<NumericLineEdit*>("lineEditAddress")->setValue(7002);
    form->findChild<NumericLineEdit*>("lineEditLength")->setValue(2);
    form->setDataType(DataType::UInt32);
    form->setRegisterOrder(RegisterOrder::LSRF);
    form->setByteOrder(ByteOrder::Swapped);
    const auto expected = form->displayDefinition();
    QTemporaryDir files;
    QSettings settings(files.filePath("view.ini"), QSettings::IniFormat);
    settings << expected;
    DataViewDefinitions restored;
    settings >> restored;
    QCOMPARE(restored, expected);
    QSettings viewSettings(files.filePath("form.ini"), QSettings::IniFormat);
    form->saveSettings(viewSettings);
    auto* settingsView = qobject_cast<FormDataView*>(fixture.Project.createMdiChild(ProjectFormKind::Data));
    QVERIFY(settingsView);
    settingsView->loadSettings(viewSettings);
    QCOMPARE(settingsView->dataType(), DataType::UInt32);
    QCOMPARE(settingsView->registerOrder(), RegisterOrder::LSRF);
    QCOMPARE(settingsView->byteOrder(), ByteOrder::Swapped);
    fixture.Project.deleteForm(settingsView);
    const auto saved = files.filePath("enron-view.omsim");
    QVERIFY(fixture.Project.saveProject(saved));
    fixture.Project.closeProject();
    definitions.EnronModbus = false;
    fixture.Server.setModbusDefinitions(definitions);
    QVERIFY(fixture.Project.loadProject(saved).Success);
    const auto forms = fixture.Project.forms(ProjectFormKind::Data);
    QCOMPARE(forms.size(), 1);
    auto* loaded = qobject_cast<FormDataView*>(forms.first());
    QVERIFY(loaded);
    QCOMPARE(loaded->displayDefinition().EnronType, EnronModbus::RegisterType::Group7x);
    QCOMPARE(loaded->displayDefinition().Length, quint16(4));
    QCOMPARE(loaded->findChild<NumericLineEdit*>("lineEditAddress")->value<int>(), 7002);
    QCOMPARE(loaded->findChild<NumericLineEdit*>("lineEditLength")->value<int>(), 2);
    QCOMPARE(loaded->dataType(), DataType::UInt32);
    QCOMPARE(loaded->registerOrder(), RegisterOrder::LSRF);
    QCOMPARE(loaded->byteOrder(), ByteOrder::Swapped);
    auto* combo = loaded->findChild<PointTypeComboBox*>("comboBoxModbusPointType");
    QCOMPARE(combo->count(), 4);
    // Project settings are applied after the data views have been constructed.
    definitions.EnronModbus = true;
    fixture.Server.setModbusDefinitions(definitions);
    QTRY_COMPARE(combo->currentEnronType(), EnronModbus::RegisterType::Group7x);
    QCOMPARE(combo->count(), 6);
    QCOMPARE(loaded->findChild<NumericLineEdit*>("lineEditAddress")->value<int>(), 7002);
}

/// \brief Converts a logical address edited in the write dialog back to an independent word pair.
void TestAppProject::editsEnronRegisterByLogicalAddress()
{
    ModbusMultiServer server;
    server.addUnitMap(QUuid::createUuid(), 1, QModbusDataUnit::HoldingRegisters, 7001, 6);
    ModbusWriteParams params;
    params.DeviceId = 1;
    params.Address = 7004;
    params.ZeroBasedAddress = false;
    params.DataMode = DataType::Float32;
    params.Value = 1.0f;
    params.Order = ByteOrder::Direct;
    params.EnronType = EnronModbus::RegisterType::Group7x;
    params.Server = &server;
    DialogWriteRegister dialog(params, QModbusDataUnit::HoldingRegisters, false, nullptr, nullptr);
    auto* address = dialog.findChild<NumericLineEdit*>("lineEditAddress");
    QCOMPARE(address->value<int>(), 7002);
    address->setValue(7003);
    dialog.findChild<NumericLineEdit*>("lineEditValue")->setValue(2.0f);
    dialog.accept();
    QCOMPARE(params.Address, quint16(7006));
    server.writeRegister(QModbusDataUnit::HoldingRegisters, params);
    QCOMPARE(server.data(1, QModbusDataUnit::HoldingRegisters, 7001, 6).values(),
             QVector<quint16>({0, 0, 0, 0, 0x4000, 0}));
    params.DataMode = DataType::UInt16;
    params.Address = 7003;
    params.Value = 0;
    DialogWriteRegister lowWord(params, QModbusDataUnit::HoldingRegisters, false, nullptr, nullptr);
    auto* lowAddress = lowWord.findChild<NumericLineEdit*>("lineEditAddress");
    QCOMPARE(lowAddress->value<int>(), 7001);
    lowAddress->setValue(7002);
    lowWord.findChild<NumericLineEdit*>("lineEditValue")->setValue(0xabcd);
    lowWord.accept();
    QCOMPARE(params.Address, quint16(7005));
    server.writeRegister(QModbusDataUnit::HoldingRegisters, params);
    QCOMPARE(server.data(1, QModbusDataUnit::HoldingRegisters, 7001, 6).values(),
             QVector<quint16>({0, 0, 0, 0xabcd, 0x4000, 0}));
}

/// \brief Applies Enron mode through server settings and preserves it when edits are cancelled.
void TestAppProject::configuresEnronMode()
{
    ModbusMultiServer server;
    DialogModbusDefinitions dialog(server);
    auto* checkbox = dialog.findChild<QCheckBox*>("checkBoxEnronModbus");
    auto* buttons = dialog.findChild<QDialogButtonBox*>("buttonBox");
    QVERIFY(checkbox);
    QVERIFY(buttons);
    QVERIFY(!checkbox->isChecked());
    checkbox->setChecked(true);
    QVERIFY(buttons->button(QDialogButtonBox::Apply)->isEnabled());
    buttons->button(QDialogButtonBox::Apply)->click();
    QVERIFY(server.getModbusDefinitions().EnronModbus);
    QVERIFY(!buttons->button(QDialogButtonBox::Apply)->isEnabled());

    DialogModbusDefinitions cancelled(server);
    auto* restored = cancelled.findChild<QCheckBox*>("checkBoxEnronModbus");
    QVERIFY(restored->isChecked());
    restored->setChecked(false);
    cancelled.reject();
    QVERIFY(server.getModbusDefinitions().EnronModbus);

    checkbox->setChecked(false);
    dialog.accept();
    QVERIFY(!server.getModbusDefinitions().EnronModbus);
}

/// \brief Restores the Enron server setting when reopening a project.
void TestAppProject::roundTripsEnronMode()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const auto source = files.filePath("source.omsim");
    const auto saved = files.filePath("enron.omsim");
    writeProjectFile(source, kAnnotatedProject);
    QVERIFY(fixture.Project.loadProject(source).Success);
    auto definitions = fixture.Server.getModbusDefinitions();
    definitions.EnronModbus = true;
    fixture.Server.setModbusDefinitions(definitions);
    QVERIFY(fixture.Project.saveProject(saved));
    fixture.Project.closeProject();
    definitions.EnronModbus = false;
    fixture.Host.mbMultiServer().setModbusDefinitions(definitions);
    QVERIFY(fixture.Project.loadProject(saved).Success);
    QVERIFY(fixture.Host.mbMultiServer().getModbusDefinitions().EnronModbus);
}

void TestAppProject::createsAndNumbersEveryFormKind()
{
    ProjectFixture fixture;
    QSignalSpy created(&fixture.Project, &AppProject::formCreated);

    auto* data1 = fixture.Project.createMdiChild(ProjectFormKind::Data);
    auto* data2 = fixture.Project.createMdiChild(ProjectFormKind::Data);
    auto* traffic = fixture.Project.createMdiChild(ProjectFormKind::Traffic);
    auto* script = fixture.Project.createMdiChild(ProjectFormKind::Script);
    auto* map = fixture.Project.createMdiChild(ProjectFormKind::DataMap);

    QVERIFY(data1);
    QVERIFY(data2);
    QVERIFY(traffic);
    QVERIFY(script);
    QVERIFY(map);
    QCOMPARE(created.count(), 5);
    QCOMPARE(data1->windowTitle(), QStringLiteral("Data1"));
    QCOMPARE(data2->windowTitle(), QStringLiteral("Data2"));
    QCOMPARE(traffic->windowTitle(), QStringLiteral("Traffic1"));
    QCOMPARE(script->windowTitle(), QStringLiteral("Script1"));
    QCOMPARE(map->windowTitle(), QStringLiteral("Map1"));
    QVERIFY(!projectFormId(data1).isNull());
    QVERIFY(projectFormId(data1) != projectFormId(data2));
}

/// \brief Verifies that closing parks a form and reopening preserves its identity.
void TestAppProject::closesReopensAndDeletesCanonicalForm()
{
    ProjectFixture fixture;
    auto* form = fixture.Project.createMdiChild(ProjectFormKind::Data);
    QVERIFY(form);
    const auto id = projectFormId(form);
    QPointer<QWidget> guardedForm = form;
    QSignalSpy closed(&fixture.Project, &AppProject::formClosed);
    QSignalSpy opened(&fixture.Project, &AppProject::formOpened);
    QSignalSpy deleted(&fixture.Project, &AppProject::formDeleted);

    fixture.Project.closeMdiChild(form);
    QCOMPARE(closed.count(), 1);
    QVERIFY(fixture.Project.isFormClosed(form));

    fixture.Project.rewrapMdiChild(form);
    QCOMPARE(opened.count(), 1);
    QVERIFY(!fixture.Project.isFormClosed(form));
    QCOMPARE(projectFormId(form), id);

    fixture.Project.deleteForm(form);
    QCOMPARE(deleted.count(), 1);
    QVERIFY(guardedForm.isNull());
}

/// \brief Verifies that locked forms cannot be permanently deleted.
void TestAppProject::respectsDeletionLock()
{
    ProjectFixture fixture;
    auto* form = fixture.Project.createMdiChild(ProjectFormKind::DataMap);
    QVERIFY(form);
    QPointer<QWidget> guardedForm = form;
    form->setProperty(ProjectFormMetadata::DeleteLocked, true);

    fixture.Project.deleteForm(form);
    QVERIFY(!guardedForm.isNull());

    form->setProperty(ProjectFormMetadata::DeleteLocked, false);
    fixture.Project.deleteForm(form);
    QVERIFY(guardedForm.isNull());
}

/// \brief Verifies sniffer mode does not leak from a closed project into the next one.
void TestAppProject::closingProjectResetsSnifferMode()
{
    ProjectFixture fixture;
    QVERIFY(fixture.Project.createMdiChild(ProjectFormKind::Traffic));

    fixture.Server.setSnifferMode(true);
    fixture.Project.closeProject();
    QVERIFY(!fixture.Server.snifferMode());
}

/// \brief Verifies split clones use UUID pairing and remain outside the project registry.
void TestAppProject::createsAndRemovesSplitClone()
{
    ProjectFixture fixture;
    fixture.Mdi.show();
    fixture.Mdi.setViewMode(QMdiArea::TabbedView);
    auto* form = fixture.Project.createMdiChild(ProjectFormKind::Data);
    QVERIFY(form);
    form->show();
    auto* primaryWindow = qobject_cast<QMdiSubWindow*>(form->parentWidget());
    QVERIFY(primaryWindow);
    fixture.Mdi.primaryArea()->setActiveSubWindow(primaryWindow);
    QCoreApplication::processEvents();
    fixture.Mdi.setSplitViewEnabled(true);

    QCOMPARE(fixture.Project.duplicatePrimaryTabsToSecondary(), 1);
    auto* secondary = fixture.Project.secondaryArea();
    QVERIFY(secondary);
    QCOMPARE(secondary->localSubWindowList().size(), 1);
    auto* clone = secondary->localSubWindowList().first()->widget();
    QVERIFY(isSplitClone(clone));
    QCOMPARE(splitOriginId(clone), projectFormId(form));
    QCOMPARE(fixture.Project.forms(ProjectFormKind::Data).size(), 1);

    fixture.Project.removeSplitAutoClonesFromSecondary();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(secondary->localSubWindowList().isEmpty());
}

/// \brief Verifies persistence of canonical open and parked forms.
void TestAppProject::roundTripsOpenAndClosedForms()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const QString path = files.filePath(QStringLiteral("roundtrip.omsim"));
    auto* openForm = fixture.Project.createMdiChild(ProjectFormKind::Data);
    auto* closedForm = fixture.Project.createMdiChild(ProjectFormKind::Script);
    QVERIFY(openForm);
    QVERIFY(closedForm);
    fixture.Project.closeMdiChild(closedForm);
    QVERIFY(fixture.Project.saveProject(path));

    fixture.Project.closeProject();
    const auto result = fixture.Project.loadProject(path);
    QVERIFY2(result.Success, qPrintable(result.Error));
    QCOMPARE(fixture.Project.forms(ProjectFormKind::Data).size(), 1);
    QCOMPARE(fixture.Project.forms(ProjectFormKind::Script).size(), 1);
    QCOMPARE(fixture.Project.closedForms().size(), 1);
}

/// \brief Verifies repeated save and reopen preserve a 0-based data view (issue #130).
void TestAppProject::preservesZeroBasedStartingAddressAcrossReopen()
{
    QTemporaryDir files;
    const QString first = files.filePath(QStringLiteral("issue130-first.omsim"));
    const QString second = files.filePath(QStringLiteral("issue130-second.omsim"));
    const auto expectedValues = QVector<quint16>{100, 200, 300};

    auto& prefs = AppPreferences::instance();
    const AddressBase previousBase = prefs.globalAddressBase();
    const auto restoreBase = qScopeGuard([&prefs, previousBase] {
        prefs.setGlobalAddressBase(previousBase);
    });

    {
        ProjectFixture fixture;
        prefs.setGlobalAddressBase(AddressBase::Base0);
        auto* form = qobject_cast<FormDataView*>(fixture.Project.createMdiChild(ProjectFormKind::Data));
        QVERIFY(form);
        form->setAddressBase(AddressBase::Base0);

        auto definition = form->displayDefinition();
        definition.PointType = QModbusDataUnit::HoldingRegisters;
        definition.PointAddress = 10;
        definition.Length = static_cast<quint16>(expectedValues.size());
        form->setDisplayDefinition(definition);

        QModbusDataUnit data(definition.PointType, 10, definition.Length);
        data.setValues(expectedValues);
        fixture.Server.setData(definition.DeviceId, data, WriteSource::User);
        QVERIFY(fixture.Project.saveProject(first));
    }

    {
        ProjectFixture fixture;
        QVERIFY(fixture.Project.loadProject(first).Success);
        QCOMPARE(prefs.globalAddressBase(), AddressBase::Base0);
        auto* form = qobject_cast<FormDataView*>(fixture.Project.forms(ProjectFormKind::Data).constFirst());
        QVERIFY(form);
        form->setAddressBase(prefs.globalAddressBase());
        QCOMPARE(form->displayDefinition().PointAddress, quint16(10));
        QCOMPARE(fixture.Server.data(1, QModbusDataUnit::HoldingRegisters, 10, 3).values(), expectedValues);
        QVERIFY(fixture.Project.saveProject(second));
    }

    {
        ProjectFixture fixture;
        QVERIFY(fixture.Project.loadProject(second).Success);
        QCOMPARE(prefs.globalAddressBase(), AddressBase::Base0);
        auto* form = qobject_cast<FormDataView*>(fixture.Project.forms(ProjectFormKind::Data).constFirst());
        QVERIFY(form);
        form->setAddressBase(prefs.globalAddressBase());
        QCOMPARE(form->displayDefinition().PointAddress, quint16(10));
        QCOMPARE(fixture.Server.data(1, QModbusDataUnit::HoldingRegisters, 10, 3).values(), expectedValues);
    }
}

/// \brief Verifies a 0-based data view starting at address 0 keeps that address across reopen
/// (issue #142).
void TestAppProject::preservesZeroBasedStartingAddressZeroAcrossReopen()
{
    QTemporaryDir files;
    const QString first = files.filePath(QStringLiteral("issue142-first.omsim"));
    const QString second = files.filePath(QStringLiteral("issue142-second.omsim"));
    const auto expectedValues = QVector<quint16>{100, 200, 300};

    auto& prefs = AppPreferences::instance();
    const AddressBase previousBase = prefs.globalAddressBase();
    const auto restoreBase = qScopeGuard([&prefs, previousBase] {
        prefs.setGlobalAddressBase(previousBase);
    });

    {
        ProjectFixture fixture;
        prefs.setGlobalAddressBase(AddressBase::Base0);
        auto* form = qobject_cast<FormDataView*>(fixture.Project.createMdiChild(ProjectFormKind::Data));
        QVERIFY(form);
        form->setAddressBase(AddressBase::Base0);

        auto definition = form->displayDefinition();
        definition.PointType = QModbusDataUnit::HoldingRegisters;
        definition.PointAddress = 0;
        definition.Length = static_cast<quint16>(expectedValues.size());
        form->setDisplayDefinition(definition);
        QCOMPARE(form->displayDefinition().PointAddress, quint16(0));

        QModbusDataUnit data(definition.PointType, 0, definition.Length);
        data.setValues(expectedValues);
        fixture.Server.setData(definition.DeviceId, data, WriteSource::User);
        QVERIFY(fixture.Project.saveProject(first));
    }

    {
        ProjectFixture fixture;
        QVERIFY(fixture.Project.loadProject(first).Success);
        QCOMPARE(prefs.globalAddressBase(), AddressBase::Base0);
        auto* form = qobject_cast<FormDataView*>(fixture.Project.forms(ProjectFormKind::Data).constFirst());
        QVERIFY(form);
        form->setAddressBase(prefs.globalAddressBase());
        QCOMPARE(form->displayDefinition().PointAddress, quint16(0));
        QCOMPARE(fixture.Server.data(1, QModbusDataUnit::HoldingRegisters, 0, 3).values(), expectedValues);
        QVERIFY(fixture.Project.saveProject(second));
    }

    {
        ProjectFixture fixture;
        QVERIFY(fixture.Project.loadProject(second).Success);
        QCOMPARE(prefs.globalAddressBase(), AddressBase::Base0);
        auto* form = qobject_cast<FormDataView*>(fixture.Project.forms(ProjectFormKind::Data).constFirst());
        QVERIFY(form);
        form->setAddressBase(prefs.globalAddressBase());
        QCOMPARE(form->displayDefinition().PointAddress, quint16(0));
        QCOMPARE(fixture.Server.data(1, QModbusDataUnit::HoldingRegisters, 0, 3).values(), expectedValues);
    }
}

/// \brief Verifies a 1-based project still falls back to address 1 when a project file
/// stores the invalid address 0.
void TestAppProject::clampsZeroStartingAddressForOneBasedProject()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const QString path = files.filePath(QStringLiteral("one-based-zero.omsim"));

    auto& prefs = AppPreferences::instance();
    const AddressBase previousBase = prefs.globalAddressBase();
    prefs.setGlobalAddressBase(AddressBase::Base1);
    const auto restoreBase = qScopeGuard([&prefs, previousBase] {
        prefs.setGlobalAddressBase(previousBase);
    });

    writeProjectFile(path,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<OpenModSim Version=\"2.0-dev\">\n"
        "    <ModbusDefinitions AddrSpace=\"6-Digits\"/>\n"
        "    <ViewSettings ViewMode=\"1\" SplitView=\"0\" GlobalZeroBasedAddress=\"0\"/>\n"
        "    <Forms>\n"
        "        <FormDataView Panel=\"L\" Title=\"Data1\" DataType=\"UInt16\" RegisterOrder=\"MSRF\" Codepage=\"\" ByteOrder=\"Direct\">\n"
        "            <Window Maximized=\"true\" Minimized=\"false\" Left=\"0\" Top=\"0\" Width=\"610\" Height=\"331\"/>\n"
        "            <DataViewDefinitions DeviceId=\"1\" PointType=\"HoldingRegisters\" PointAddress=\"0\" Length=\"8\" DataViewColumnsDistance=\"25\" LeadingZeros=\"true\"/>\n"
        "        </FormDataView>\n"
        "    </Forms>\n"
        "</OpenModSim>\n");

    const auto result = fixture.Project.loadProject(path);
    QVERIFY2(result.Success, qPrintable(result.Error));
    auto* form = qobject_cast<FormDataView*>(fixture.Project.forms(ProjectFormKind::Data).constFirst());
    QVERIFY(form);
    QCOMPARE(form->displayDefinition().PointAddress, quint16(1));
}

/// \brief Verifies the project-opened notification precedes restored devices and maps.
void TestAppProject::opensProjectBeforeRestoringServerState()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const QString path = files.filePath(QStringLiteral("ordered.omsim"));
    writeProjectFile(path, kAnnotatedProject);

    QStringList events;
    connect(&fixture.Project, &AppProject::projectOpened, &fixture.Project,
            [&events](const QString&) { events.append(QStringLiteral("project")); },
            Qt::DirectConnection);
    connect(&fixture.Server, &ModbusMultiServer::deviceIdAdded, &fixture.Project,
            [&events](quint8) { events.append(QStringLiteral("device")); },
            Qt::DirectConnection);
    connect(&fixture.Server, &ModbusMultiServer::unitMapAdded, &fixture.Project,
            [&events](QUuid, quint8, QModbusDataUnit::RegisterType, quint16, quint16) {
                events.append(QStringLiteral("map"));
            }, Qt::DirectConnection);

    const auto result = fixture.Project.loadProject(path);
    QVERIFY2(result.Success, qPrintable(result.Error));
    QVERIFY(events.contains(QStringLiteral("device")));
    QVERIFY(events.contains(QStringLiteral("map")));
    QCOMPARE(events.constFirst(), QStringLiteral("project"));
}

/// \brief Verifies malformed XML is rejected before any project state changes.
void TestAppProject::rejectsMalformedProjectWithoutChangingState()
{
    ProjectFixture fixture;
    auto* existing = fixture.Project.createMdiChild(ProjectFormKind::Data);
    QVERIFY(existing);
    QTemporaryDir files;
    const QString path = files.filePath(QStringLiteral("broken.omsim"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("<OpenModSim><Forms><FormDataView></OpenModSim>");
    file.close();

    const auto result = fixture.Project.loadProject(path);
    QVERIFY(!result.Success);
    QVERIFY(!result.Error.isEmpty());
    QCOMPARE(fixture.Project.forms(ProjectFormKind::Data).size(), 1);
    QCOMPARE(fixture.Project.forms(ProjectFormKind::Data).first(), existing);
}

/// \brief Verifies a hand-written project whose ModbusDefinitions and ConnectionDetails
/// carry no child elements is read to the end. Both readers used to consume the closing
/// tag of their own element and then skip the parent, discarding the rest of the file.
void TestAppProject::loadsProjectWithChildlessDefinitionsAndConnection()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const QString path = files.filePath(QStringLiteral("minimal.omsim"));
    writeProjectFile(path,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<OpenModSim Version=\"2.0-dev\">\n"
        "    <ModbusDefinitions AddrSpace=\"6-Digits\"/>\n"
        "    <Connections>\n"
        "        <ConnectionDetails ConnectionType=\"Tcp\"/>\n"
        "    </Connections>\n"
        "    <Forms>\n"
        "        <FormDataView Panel=\"L\" Title=\"Data1\" DataType=\"UInt16\" RegisterOrder=\"MSRF\" Codepage=\"\" ByteOrder=\"Direct\">\n"
        "            <Window Maximized=\"true\" Minimized=\"false\" Left=\"0\" Top=\"0\" Width=\"610\" Height=\"331\"/>\n"
        "            <DataViewDefinitions DeviceId=\"1\" PointType=\"HoldingRegisters\" PointAddress=\"1\" Length=\"8\" DataViewColumnsDistance=\"25\" LeadingZeros=\"true\"/>\n"
        "            <AddressColorMap/>\n"
        "        </FormDataView>\n"
        "    </Forms>\n"
        "</OpenModSim>\n");

    const auto result = fixture.Project.loadProject(path);
    QVERIFY2(result.Success, qPrintable(result.Error));
    QCOMPARE(fixture.Project.forms(ProjectFormKind::Data).size(), 1);
}

/// \brief Verifies XML comments survive a load/save cycle at their original positions.
void TestAppProject::preservesXmlCommentsAcrossRoundTrip()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const QString source = files.filePath(QStringLiteral("annotated.omsim"));
    const QString saved = files.filePath(QStringLiteral("saved.omsim"));
    writeProjectFile(source, kAnnotatedProject);

    const auto result = fixture.Project.loadProject(source);
    QVERIFY2(result.Success, qPrintable(result.Error));
    QVERIFY(fixture.Project.saveProject(saved));

    QCOMPARE(commentPlacements(saved), QStringList({
        QStringLiteral("|OpenModSim|Pump station, hand written"),
        QStringLiteral("OpenModSim|Connections|network settings"),
        QStringLiteral("OpenModSim/AddressSpace/AddressDescriptionMap|Description|telemetry block"),
        QStringLiteral("|END|end of project")
    }));

    QFile file(saved);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.readAll().contains("<![CDATA[Sine]]>"));
}

/// \brief Verifies the comment store is not consumed by a save.
void TestAppProject::keepsCommentsOnRepeatedSave()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const QString source = files.filePath(QStringLiteral("annotated.omsim"));
    const QString first = files.filePath(QStringLiteral("first.omsim"));
    const QString second = files.filePath(QStringLiteral("second.omsim"));
    writeProjectFile(source, kAnnotatedProject);

    QVERIFY(fixture.Project.loadProject(source).Success);
    QVERIFY(fixture.Project.saveProject(first));
    QVERIFY(fixture.Project.saveProject(second));

    QCOMPARE(commentPlacements(second), commentPlacements(first));
    QCOMPARE(commentPlacements(second).size(), 4);
}

/// \brief Verifies closing a project discards its comments.
void TestAppProject::dropsCommentsAfterCloseProject()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const QString source = files.filePath(QStringLiteral("annotated.omsim"));
    const QString saved = files.filePath(QStringLiteral("saved.omsim"));
    writeProjectFile(source, kAnnotatedProject);

    QVERIFY(fixture.Project.loadProject(source).Success);
    fixture.Project.closeProject();
    QVERIFY(fixture.Project.saveProject(saved));

    QVERIFY(commentPlacements(saved).isEmpty());
}

/// \brief Verifies the timestamp preference reaches the written project file.
void TestAppProject::omitsTimestampsWhenPreferenceDisabled()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const QString source = files.filePath(QStringLiteral("annotated.omsim"));
    const QString withStamps = files.filePath(QStringLiteral("with.omsim"));
    const QString withoutStamps = files.filePath(QStringLiteral("without.omsim"));
    writeProjectFile(source, kAnnotatedProject);

    QVERIFY(fixture.Project.loadProject(source).Success);
    fixture.Server.setTimestamp(1, QModbusDataUnit::HoldingRegisters, 0, QDateTime::currentDateTime());

    auto& prefs = AppPreferences::instance();
    const bool restore = prefs.saveRegisterTimestamps();

    prefs.setSaveRegisterTimestamps(true);
    QVERIFY(fixture.Project.saveProject(withStamps));
    prefs.setSaveRegisterTimestamps(false);
    QVERIFY(fixture.Project.saveProject(withoutStamps));
    prefs.setSaveRegisterTimestamps(restore);

    QFile enabled(withStamps);
    QVERIFY(enabled.open(QIODevice::ReadOnly));
    QVERIFY(enabled.readAll().contains("<AddressTimestampMap"));

    QFile disabled(withoutStamps);
    QVERIFY(disabled.open(QIODevice::ReadOnly));
    QVERIFY(!disabled.readAll().contains("AddressTimestampMap"));
}

/// \brief Verifies saving a project written in the old single-file shape produces a clean
/// project file plus a user-state file beside it.
void TestAppProject::splitsUserStateIntoSeparateFile()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const QString source = files.filePath(QStringLiteral("annotated.omsim"));
    const QString saved = files.filePath(QStringLiteral("saved.omsim"));
    writeProjectFile(source, kAnnotatedProject);

    QVERIFY(fixture.Project.loadProject(source).Success);
    QVERIFY(fixture.Project.saveProject(saved));

    const QString userPath = projectUserStatePath(saved);
    QVERIFY(QFile::exists(userPath));

    QFile project(saved);
    QVERIFY(project.open(QIODevice::ReadOnly));
    const auto projectText = project.readAll();
    QVERIFY(!projectText.contains("<Window"));
    QVERIFY(!projectText.contains("<ViewSettings"));
    QVERIFY(projectText.contains("<FormDataView"));
    QVERIFY(projectText.contains("<ModbusDefinitions"));

    QFile user(userPath);
    QVERIFY(user.open(QIODevice::ReadOnly));
    const auto userText = user.readAll();
    QVERIFY(userText.contains("OpenModSimUser"));
    QVERIFY(userText.contains("<Window"));
    QVERIFY(userText.contains("<ViewSettings"));
}

/// \brief Verifies the window size stored in the user-state file comes back on load.
void TestAppProject::restoresWindowGeometryFromUserStateFile()
{
    QTemporaryDir files;
    const QString source = files.filePath(QStringLiteral("annotated.omsim"));
    const QString saved = files.filePath(QStringLiteral("saved.omsim"));
    writeProjectFile(source, kAnnotatedProject);

    {
        ProjectFixture fixture;
        QVERIFY(fixture.Project.loadProject(source).Success);
        QVERIFY(fixture.Project.saveProject(saved));
    }

    QFile user(projectUserStatePath(saved));
    QVERIFY(user.open(QIODevice::ReadOnly));
    QVERIFY(user.readAll().contains("Width="));
    user.close();

    ProjectFixture reopened;
    QVERIFY(reopened.Project.loadProject(saved).Success);
    QCOMPARE(reopened.Project.forms(ProjectFormKind::Data).size(), 1);

    const QString resaved = files.filePath(QStringLiteral("resaved.omsim"));
    QVERIFY(reopened.Project.saveProject(resaved));

    QFile resavedUser(projectUserStatePath(resaved));
    QVERIFY(resavedUser.open(QIODevice::ReadOnly));
    QVERIFY(resavedUser.readAll().contains("<Window"));
}

/// \brief Verifies that loading into an already visible tabbed area does not move maximized
/// windows to the stored Left/Top, which left the last tab drawn over the others.
void TestAppProject::keepsTabbedWindowsMaximizedWhenLoadingIntoVisibleArea()
{
    QTemporaryDir files;
    const QString source = files.filePath(QStringLiteral("tabbed.omsim"));
    writeProjectFile(source, kTabbedProjectWithOffsetWindows);

    ProjectFixture fixture;
    fixture.Mdi.resize(900, 600);
    fixture.Mdi.show();
    QVERIFY(QTest::qWaitForWindowExposed(&fixture.Mdi));

    QVERIFY(fixture.Project.loadProject(source).Success);
    QCoreApplication::processEvents();

    auto* area = fixture.Mdi.primaryArea();
    const auto windows = area->localSubWindowList();
    QCOMPARE(windows.size(), 2);
    for (auto* wnd : windows) {
        QVERIFY2(wnd->isMaximized(), qPrintable(wnd->windowTitle()));
        QCOMPARE(wnd->geometry(), area->viewport()->rect());
    }
}

/// \brief Verifies a project checked out without its user-state file still opens.
void TestAppProject::opensProjectWithoutUserStateFile()
{
    QTemporaryDir files;
    const QString source = files.filePath(QStringLiteral("annotated.omsim"));
    const QString saved = files.filePath(QStringLiteral("saved.omsim"));
    writeProjectFile(source, kAnnotatedProject);

    {
        ProjectFixture fixture;
        QVERIFY(fixture.Project.loadProject(source).Success);
        QVERIFY(fixture.Project.saveProject(saved));
    }

    QVERIFY(QFile::remove(projectUserStatePath(saved)));

    ProjectFixture reopened;
    const auto result = reopened.Project.loadProject(saved);
    QVERIFY2(result.Success, qPrintable(result.Error));
    QCOMPARE(reopened.Project.forms(ProjectFormKind::Data).size(), 1);
}

/// \brief Verifies an existing user-state file is replaced rather than appended to or left
/// with content from an earlier save. Every project has view settings, so the file is
/// always written; the empty-document branch only guards against a future project shape
/// that carries no user state at all.
void TestAppProject::removesStaleUserStateFile()
{
    ProjectFixture fixture;
    QTemporaryDir files;
    const QString saved = files.filePath(QStringLiteral("empty.omsim"));
    const QString userPath = projectUserStatePath(saved);

    QFile stale(userPath);
    QVERIFY(stale.open(QIODevice::WriteOnly));
    stale.write("<OpenModSimUser><Marker Stale=\"yes\"/></OpenModSimUser>");
    stale.close();

    QVERIFY(fixture.Project.saveProject(saved));

    QFile written(userPath);
    QVERIFY(written.open(QIODevice::ReadOnly));
    const auto text = written.readAll();
    QVERIFY(!text.contains("Stale"));
    QVERIFY(text.contains("<ViewSettings"));
}

/// \brief Verifies a project holding several forms of one kind survives save and reload.
/// Their user state used to collapse onto the first form, and the reloaded document was
/// rejected outright for repeating an attribute.
void TestAppProject::roundTripsSeveralFormsOfTheSameKind()
{
    QTemporaryDir files;
    const QString saved = files.filePath(QStringLiteral("many.omsim"));
    const QString resaved = files.filePath(QStringLiteral("many2.omsim"));

    {
        ProjectFixture fixture;
        for (int i = 0; i < 3; ++i)
            QVERIFY(fixture.Project.createMdiChild(ProjectFormKind::Data));
        QCOMPARE(fixture.Project.forms(ProjectFormKind::Data).size(), 3);
        QVERIFY(fixture.Project.saveProject(saved));
    }

    ProjectFixture reopened;
    const auto result = reopened.Project.loadProject(saved);
    QVERIFY2(result.Success, qPrintable(result.Error));
    QCOMPARE(reopened.Project.forms(ProjectFormKind::Data).size(), 3);

    QVERIFY(reopened.Project.saveProject(resaved));
    const auto again = reopened.Project.validateProject(resaved);
    QVERIFY2(again.Success, qPrintable(again.Error));
}

/// \brief Verifies closing the project stores the layout on its own. Arranging windows
/// never marks the project modified, so nothing would otherwise prompt or trigger a save.
void TestAppProject::storesUserStateOnCloseWithoutSavingProject()
{
    QTemporaryDir files;
    const QString path = files.filePath(QStringLiteral("layout.omsim"));
    const QString userPath = projectUserStatePath(path);

    {
        ProjectFixture fixture;
        QVERIFY(fixture.Project.createMdiChild(ProjectFormKind::Data));
        QVERIFY(fixture.Project.saveProject(path));
    }

    QVERIFY(QFile::remove(userPath));

    ProjectFixture reopened;
    QVERIFY(reopened.Project.loadProject(path).Success);

    // No saveProject() here: closing alone has to put the layout back on disk.
    reopened.Project.closeProject();

    QVERIFY(QFile::exists(userPath));
    QFile user(userPath);
    QVERIFY(user.open(QIODevice::ReadOnly));
    const auto text = user.readAll();
    QVERIFY(text.contains("OpenModSimUser"));
    QVERIFY(text.contains("<ViewSettings"));
}

/// \brief Verifies the demo projects shipped with the application still open. They are
/// hand-maintained files, so nothing else would notice them drifting away from what the
/// reader expects.
void TestAppProject::opensShippedDemoProjects()
{
    for (const auto& demo : demoProjects()) {
        ProjectFixture fixture;
        const auto result = fixture.Project.loadProject(demo.first);
        QVERIFY2(result.Success, qPrintable(demo.first + QStringLiteral(": ") + result.Error));

        int forms = 0;
        for (auto kind : { ProjectFormKind::Data, ProjectFormKind::Script,
                           ProjectFormKind::DataMap, ProjectFormKind::Traffic })
            forms += fixture.Project.forms(kind).size();
        QCOMPARE(forms, demo.second);

        fixture.Project.closeProject();
    }
}

/// \brief Verifies the shipped demos carry no user state. Their .omsim.user file is never
/// distributed, so anything left in the project file would be one machine's layout frozen
/// into every installation.
void TestAppProject::keepsShippedDemoProjectsFreeOfUserState()
{
    for (const auto& demo : demoProjects()) {
        QFile file(demo.first);
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(demo.first));
        const auto text = file.readAll();

        for (const auto* marker : { "<ViewSettings", "<TabOrder", "<Window", "<Colors",
                                    "<Font", "<Zoom", "<ColumnWidths",
                                    "CursorPosition", "ScrollPosition", "LeadingZeros" }) {
            QVERIFY2(!text.contains(marker), qPrintable(demo.first + QStringLiteral(": ")
                                                        + QString::fromLatin1(marker)));
        }
    }
}

int main(int argc, char** argv)
{
    Application app(argc, argv);
    TestAppProject test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_appproject.moc"
