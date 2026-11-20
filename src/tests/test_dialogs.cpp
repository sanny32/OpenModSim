// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dialogs.cpp
/// \brief Unit tests for dialog controls and value propagation.
///

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFile>
#include <QFontComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPrinter>
#include <QSpinBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include "apppreferences.h"
#include "application.h"
#include "widgets/aboutdatawidget.h"
#include "dialogs/dialogabout.h"
#include "dialogs/dialogautosimulation.h"
#include "dialogs/dialogcoilsimulation.h"
#include "dialogs/dialogforcemultipleregisters.h"
#include "dialogs/dialogforcestatusregisters.h"
#include "dialogs/dialogmsgparser.h"
#include "dialogs/dialogpreferences.h"
#include "dialogs/dialogprintsettings.h"
#include "dialogs/dialogsetupserialport.h"
#include "dialogs/dialogwelcome.h"
#include "dialogs/dialogwindowsmanager.h"
#include "dialogs/dialogwritestatusregister.h"
#include "dialogs/dialogwriteregister.h"
#include "widgets/bytelisttextedit.h"
#include "widgets/numericlineedit.h"
#include "widgets/simulationmodecombobox.h"

class TestDialogs : public QObject
{
    Q_OBJECT

private slots:
    void forceMultipleRegisters_data();
    void forceMultipleRegisters();
    void forceMultipleInputRegisters();
    void forceStatusRegisters_data();
    void forceStatusRegisters();
    void autoSimulation_data();
    void autoSimulation();
    void autoSimulationCanBeDisabled();
    void coilSimulation();
    void writeStatusRegister();
    void messageParser_data();
    void messageParser();
    void aboutDialog();
    void preferencesDialog();
    void printSettings();
    void welcomeDialog();
    void windowsManager();
    void setupSerialPort();
    void writeRegister_data();
    void writeRegister();
};

namespace {

template<typename T>
T* child(QObject* parent, const char* name)
{
    auto* result = parent->findChild<T*>(QLatin1String(name));
    Q_ASSERT(result != nullptr);
    return result;
}

ModbusWriteParams writeParams(DataType type)
{
    ModbusWriteParams params = {};
    params.DeviceId = 1;
    params.Address = 1;
    params.DataMode = type;
    params.Order = ByteOrder::Direct;
    params.ZeroBasedAddress = false;
    params.Value = QVariant::fromValue(QVector<quint16>{1, 2, 3, 4, 5, 6, 7, 8});
    return params;
}

void sendLanguageChange(QWidget& widget)
{
    QEvent event(QEvent::LanguageChange);
    QApplication::sendEvent(&widget, &event);
}

void closeModalOnNextEvent()
{
    QTimer::singleShot(0, [] {
        if (auto* widget = QApplication::activeModalWidget())
            widget->close();
    });
}

}

void TestDialogs::forceMultipleRegisters_data()
{
    QTest::addColumn<int>("dataType");
    QTest::newRow("binary") << static_cast<int>(DataType::Binary);
    QTest::newRow("hex") << static_cast<int>(DataType::Hex);
    QTest::newRow("int16") << static_cast<int>(DataType::Int16);
    QTest::newRow("uint16") << static_cast<int>(DataType::UInt16);
    QTest::newRow("int32") << static_cast<int>(DataType::Int32);
    QTest::newRow("uint32") << static_cast<int>(DataType::UInt32);
    QTest::newRow("int64") << static_cast<int>(DataType::Int64);
    QTest::newRow("uint64") << static_cast<int>(DataType::UInt64);
    QTest::newRow("float32") << static_cast<int>(DataType::Float32);
    QTest::newRow("float64") << static_cast<int>(DataType::Float64);
}

void TestDialogs::forceMultipleRegisters()
{
    QFETCH(int, dataType);
    const auto type = static_cast<DataType>(dataType);
    auto params = writeParams(type);
    DialogForceMultipleRegisters dialog(params, QModbusDataUnit::HoldingRegisters, 8, true);

    auto* formats = child<QComboBox>(&dialog, "comboBoxDisplayFormat");
    auto* registerOrder = child<QComboBox>(&dialog, "comboBoxRegisterOrder");
    auto* byteOrder = child<QComboBox>(&dialog, "comboBoxByteOrder");
    auto* presets = child<QComboBox>(&dialog, "comboBoxPreset");
    auto* apply = child<QPushButton>(&dialog, "pushButtonValue");
    formats->setCurrentIndex(formats->findData(dataType));

    for (int preset = 0; preset < presets->count(); ++preset) {
        if (preset == 1 && isMultiRegisterType(type))
            continue;
        presets->setCurrentIndex(preset);
        apply->click();
    }

    registerOrder->setCurrentIndex(1);
    byteOrder->setCurrentIndex(1);
    child<NumericLineEdit>(&dialog, "lineEditDeviceId")->setValue(2);
    child<NumericLineEdit>(&dialog, "lineEditAddress")->setValue(2);
    child<NumericLineEdit>(&dialog, "lineEditLength")->setValue(6);
    child<NumericLineEdit>(&dialog, "lineEditLength")->setValue(8);

    auto* table = child<QTableWidget>(&dialog, "tableWidget");
    if (auto* editor = qobject_cast<NumericLineEdit*>(table->cellWidget(0, 0)))
        editor->setValue(9);

    sendLanguageChange(dialog);
    dialog.accept();

    QCOMPARE(dialog.result(), QDialog::Accepted);
    QCOMPARE(params.DeviceId, 2U);
    QCOMPARE(params.Address, 2);
    QCOMPARE(params.Value.value<QVector<quint16>>().size(), 8);
}

void TestDialogs::forceMultipleInputRegisters()
{
    auto params = writeParams(DataType::Ansi);
    DialogForceMultipleRegisters dialog(params, QModbusDataUnit::InputRegisters, 8, false);
    QVERIFY(dialog.windowTitle().contains(QStringLiteral("INPUT")));
    QCOMPARE(params.DataMode, DataType::Hex);
}

void TestDialogs::forceStatusRegisters_data()
{
    QTest::addColumn<int>("registerType");
    QTest::newRow("coils") << static_cast<int>(QModbusDataUnit::Coils);
    QTest::newRow("discrete-inputs") << static_cast<int>(QModbusDataUnit::DiscreteInputs);
}

void TestDialogs::forceStatusRegisters()
{
    QFETCH(int, registerType);
    auto params = writeParams(DataType::Binary);
    DialogForceStatusRegisters dialog(params, static_cast<QModbusDataUnit::RegisterType>(registerType), 10, true);

    child<QPushButton>(&dialog, "pushButton0")->click();
    child<QPushButton>(&dialog, "pushButton1")->click();
    child<QPushButton>(&dialog, "pushButtonRandom")->click();
    child<NumericLineEdit>(&dialog, "lineEditDeviceId")->setValue(3);
    child<NumericLineEdit>(&dialog, "lineEditAddress")->setValue(2);
    child<NumericLineEdit>(&dialog, "lineEditLength")->setValue(9);

    auto* table = child<QTableWidget>(&dialog, "tableWidget");
    emit table->itemDoubleClicked(table->item(0, 0));
    emit table->itemDoubleClicked(nullptr);
    sendLanguageChange(dialog);
    dialog.accept();

    QCOMPARE(dialog.result(), QDialog::Accepted);
    QCOMPARE(params.DeviceId, 3U);
    QCOMPARE(params.Value.value<QVector<quint16>>().size(), 9);
}

void TestDialogs::autoSimulation_data()
{
    QTest::addColumn<int>("dataType");
    QTest::newRow("binary") << static_cast<int>(DataType::Binary);
    QTest::newRow("hex") << static_cast<int>(DataType::Hex);
    QTest::newRow("ansi") << static_cast<int>(DataType::Ansi);
    QTest::newRow("int16") << static_cast<int>(DataType::Int16);
    QTest::newRow("uint16") << static_cast<int>(DataType::UInt16);
    QTest::newRow("int32") << static_cast<int>(DataType::Int32);
    QTest::newRow("uint32") << static_cast<int>(DataType::UInt32);
    QTest::newRow("int64") << static_cast<int>(DataType::Int64);
    QTest::newRow("uint64") << static_cast<int>(DataType::UInt64);
    QTest::newRow("float32") << static_cast<int>(DataType::Float32);
    QTest::newRow("float64") << static_cast<int>(DataType::Float64);
}

void TestDialogs::autoSimulation()
{
    QFETCH(int, dataType);
    const auto type = static_cast<DataType>(dataType);
    const QList<SimulationMode> modes = {
        SimulationMode::Random, SimulationMode::Increment, SimulationMode::Decrement
    };

    for (const auto mode : modes) {
        ModbusSimulationParams params;
        DialogAutoSimulation dialog(type, params);
        auto* enabled = child<QCheckBox>(&dialog, "checkBoxEnabled");
        auto* modeCombo = child<SimulationModeComboBox>(&dialog, "comboBoxSimulationType");
        enabled->setChecked(true);
        modeCombo->setCurrentSimulationMode(mode);
        if (modeCombo->currentSimulationMode() != mode)
            continue;

        child<NumericLineEdit>(&dialog, "lineEditLowLimit")->setValue(1);
        child<NumericLineEdit>(&dialog, "lineEditHighLimit")->setValue(100);
        child<NumericLineEdit>(&dialog, "lineEditInterval")->setValue(2000);
        sendLanguageChange(dialog);
        dialog.accept();

        QCOMPARE(params.Mode, mode);
        QCOMPARE(params.Interval, 2000U);
        QCOMPARE(params.DataMode, type);
    }
}

void TestDialogs::autoSimulationCanBeDisabled()
{
    ModbusSimulationParams params;
    params.Mode = SimulationMode::Random;
    DialogAutoSimulation dialog(DataType::UInt16, params);
    child<QCheckBox>(&dialog, "checkBoxEnabled")->setChecked(false);
    dialog.accept();
    QCOMPARE(params.Mode, SimulationMode::Off);
}

void TestDialogs::coilSimulation()
{
    ModbusSimulationParams params;
    DialogCoilSimulation dialog(params);
    auto* enabled = child<QCheckBox>(&dialog, "checkBoxEnabled");
    auto* modes = child<SimulationModeComboBox>(&dialog, "comboBoxSimulationType");
    enabled->setChecked(true);
    modes->setCurrentSimulationMode(SimulationMode::Random);
    child<NumericLineEdit>(&dialog, "lineEditInterval")->setValue(500);
    dialog.accept();
    QCOMPARE(params.Mode, SimulationMode::Random);
    QCOMPARE(params.RandomParams.Range.from(), 0);
    QCOMPARE(params.RandomParams.Range.to(), 1);

    DialogCoilSimulation disabled(params);
    child<QCheckBox>(&disabled, "checkBoxEnabled")->setChecked(false);
    disabled.accept();
    QCOMPARE(params.Mode, SimulationMode::Off);
}

void TestDialogs::writeStatusRegister()
{
    auto params = writeParams(DataType::Binary);
    params.Value = false;
    DialogWriteStatusRegister dialog(params, QModbusDataUnit::Coils, true);
    child<NumericLineEdit>(&dialog, "lineEditNode")->setValue(4);
    child<NumericLineEdit>(&dialog, "lineEditAddress")->setValue(5);
    child<QRadioButton>(&dialog, "radioButtonOn")->setChecked(true);
    sendLanguageChange(dialog);
    dialog.accept();

    QCOMPARE(params.DeviceId, 4U);
    QCOMPARE(params.Address, 5);
    QCOMPARE(params.Value.toBool(), true);
}

void TestDialogs::messageParser_data()
{
    QTest::addColumn<int>("protocol");
    QTest::addColumn<QByteArray>("message");
    QTest::newRow("rtu") << static_cast<int>(ModbusMessage::Rtu)
                         << QByteArray::fromHex("010300000001840a");
    QTest::newRow("tcp") << static_cast<int>(ModbusMessage::Tcp)
                         << QByteArray::fromHex("000100000006010300000001");
}

void TestDialogs::messageParser()
{
    QFETCH(int, protocol);
    QFETCH(QByteArray, message);
    DialogMsgParser dialog(DataType::UInt16, static_cast<ModbusMessage::ProtocolType>(protocol));
    auto* bytes = child<ByteListTextEdit>(&dialog, "bytesData");
    auto* hexView = child<QCheckBox>(&dialog, "hexView");
    hexView->setChecked(true);
    bytes->setValue(message);
    child<QPushButton>(&dialog, "pushButtonParse")->click();
    bytes->clear();
    hexView->setChecked(false);
    QCoreApplication::processEvents();
    sendLanguageChange(dialog);
    QVERIFY(!child<QPushButton>(&dialog, "pushButtonParse")->isEnabled());
}

void TestDialogs::aboutDialog()
{
    DialogAbout dialog;
    auto* updateChecker = child<UpdateChecker>(&dialog, {});
    QVERIFY(dialog.findChildren<AboutDataWidget*>().size() > 0);
    child<QPushButton>(&dialog, "buttonCheckForUpdates");

    emit updateChecker->checkStarted();
    closeModalOnNextEvent();
    emit updateChecker->noUpdatesAvailable();
    QCoreApplication::processEvents();
    closeModalOnNextEvent();
    emit updateChecker->checkFailed(QStringLiteral("network error"));
    QCoreApplication::processEvents();
    closeModalOnNextEvent();
    emit updateChecker->newVersionAvailable(QStringLiteral("9.9.9"), QStringLiteral("https://example.com"));
    QCoreApplication::processEvents();

    sendLanguageChange(dialog);
    dialog.show();
    QVERIFY(dialog.sizeHint().isValid());
}

void TestDialogs::preferencesDialog()
{
    DialogPreferences dialog(nullptr);
    auto* categories = child<QListWidget>(&dialog, "listWidget");
    for (int row = 0; row < categories->count(); ++row)
        categories->setCurrentRow(row);

    for (const char* name : {
        "pushButtonResetBackgroundColor", "pushButtonResetForegroundColor",
        "pushButtonResetAddressColor", "pushButtonResetCommentColor"
    })
        child<QPushButton>(&dialog, name)->click();

    auto* buttons = child<QDialogButtonBox>(&dialog, "buttonBox");
    emit buttons->clicked(buttons->button(QDialogButtonBox::Apply));
    emit buttons->clicked(buttons->button(QDialogButtonBox::Cancel));
    sendLanguageChange(dialog);
    dialog.accept();
    QCOMPARE(dialog.result(), QDialog::Accepted);
}

void TestDialogs::printSettings()
{
    QPrinter printer;
    DialogPrintSettings dialog(&printer);
    child<QRadioButton>(&dialog, "radioButtonLandscape")->click();
    child<QRadioButton>(&dialog, "radioButtonPortrait")->click();
    sendLanguageChange(dialog);
    dialog.accept();
    QCOMPARE(dialog.result(), QDialog::Accepted);
}

void TestDialogs::welcomeDialog()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("project.omsim"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("<OpenModSim/>");
    file.close();

    DialogWelcome dialog({path, directory.filePath(QStringLiteral("missing.omsim"))});
    auto* projects = child<QListWidget>(&dialog, "listProjects");
    QVERIFY(projects->count() > 0);
    child<QCheckBox>(&dialog, "checkBoxDontShow")->setChecked(true);
    QVERIFY(dialog.dontShowAgain());
    QVERIFY(dialog.sizeHint().width() >= dialog.sizeHint().height());
    sendLanguageChange(dialog);
    child<QPushButton>(&dialog, "buttonOpenProject")->click();
    QCOMPARE(dialog.action(), DialogWelcome::Action::OpenFile);
    QCOMPARE(dialog.filePath(), path);

    DialogWelcome newProject({});
    child<QPushButton>(&newProject, "buttonNewProject")->click();
    QCOMPARE(newProject.action(), DialogWelcome::Action::NewProject);

    DialogWelcome doubleClick({path});
    emit child<QListWidget>(&doubleClick, "listProjects")->itemDoubleClicked(
        child<QListWidget>(&doubleClick, "listProjects")->item(0));
    QCOMPARE(doubleClick.action(), DialogWelcome::Action::OpenFile);
}

void TestDialogs::windowsManager()
{
    QMdiSubWindow window;
    window.setWindowTitle(QStringLiteral("Registers"));
    QAction activate(this);
    activate.setData(QVariant::fromValue(&window));
    QAction save(this);
    QSignalSpy activateSpy(&activate, &QAction::triggered);
    QSignalSpy saveSpy(&save, &QAction::triggered);

    DialogWindowsManager dialog({&activate}, &save);
    auto* list = child<QListWidget>(&dialog, "listWidget");
    list->setCurrentRow(0);
    emit list->itemDoubleClicked(list->item(0));
    QCOMPARE(activateSpy.count(), 1);

    DialogWindowsManager saveDialog({&activate}, &save);
    child<QListWidget>(&saveDialog, "listWidget")->setCurrentRow(0);
    child<QPushButton>(&saveDialog, "pushButtonSave")->click();
    QCOMPARE(saveSpy.count(), 1);

    DialogWindowsManager closeDialog({&activate}, nullptr);
    child<QListWidget>(&closeDialog, "listWidget")->setCurrentRow(0);
    child<QPushButton>(&closeDialog, "pushButtonClose")->click();
    QCOMPARE(child<QListWidget>(&closeDialog, "listWidget")->count(), 0);
}

void TestDialogs::setupSerialPort()
{
    SerialConnectionParams params;
    params.PortName = QStringLiteral("test-port");
    DialogSetupSerialPort dialog(params);
    auto* flowControl = child<QComboBox>(&dialog, "comboBoxFlowControl");
    for (int index = 0; index < flowControl->count(); ++index)
        flowControl->setCurrentIndex(index);
    sendLanguageChange(dialog);
    dialog.accept();
    QCOMPARE(dialog.result(), QDialog::Accepted);
    QCOMPARE(params.PortName, QStringLiteral("test-port"));
}


/// \brief Supplies all supported value formats to the single-register dialog.
void TestDialogs::writeRegister_data()
{
    QTest::addColumn<int>("dataType");
    for (const auto type : {DataType::Binary, DataType::Hex, DataType::Ansi, DataType::Int16,
                            DataType::UInt16, DataType::Int32, DataType::UInt32, DataType::Int64,
                            DataType::UInt64, DataType::Float32, DataType::Float64})
        QTest::newRow(qPrintable(enumToString(type))) << int(type);
}

/// \brief Preserves edited unit, address and value across each numeric format.
void TestDialogs::writeRegister()
{
    QFETCH(int, dataType);
    ModbusWriteParams params{};
    params.DeviceId = 1;
    params.Address = 1;
    params.DataMode = static_cast<DataType>(dataType);
    params.Value = 42;
    params.Codepage = QStringLiteral("UTF-8");
    DialogWriteRegister dialog(params, QModbusDataUnit::HoldingRegisters, false);
    child<NumericLineEdit>(&dialog, "lineEditNode")->setValue(2);
    child<NumericLineEdit>(&dialog, "lineEditAddress")->setValue(3);
    child<NumericLineEdit>(&dialog, "lineEditValue")->setValue(17);
    if (registersCount(params.DataMode) == 1)
        sendLanguageChange(dialog);
    dialog.accept();
    QCOMPARE(params.DeviceId, 2);
    QCOMPARE(params.Address, 3);
    QCOMPARE(params.Value.toInt(), 17);
}

int main(int argc, char** argv)
{
    Application app(argc, argv);
    TestDialogs test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_dialogs.moc"
