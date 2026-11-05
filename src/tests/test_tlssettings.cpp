// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_tlssettings.cpp
/// \brief Tests the TCP security settings dialog.
///

#include <QCheckBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidget>
#include <QGroupBox>
#include "elidinglineedit.h"
#include "tlscertificateswidget.h"
#include "dialogmodbusdefinitions.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QTimer>
#include <QTest>
#include <QTranslator>
#include "application.h"
#include "macappstyle.h"
#include "qlementineappstyle.h"
#include <QSslSocket>
#include "dialogselectserviceport.h"
#include "numericlineedit.h"

class TestTlsSettings : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void savesTlsSettings();
    void preservesCustomPorts();
    void excludesTlsFromRtuTcp();
    void cancelPreservesSettings();
    void generatesServerCertificateInSettings();
    void offersCertificatesTab();
    void keepsCertificateControlsAccessible_data();
    void keepsCertificateControlsAccessible();
    void keepsSettingsSpacingCompact_data();
    void keepsSettingsSpacingCompact();
    void usesCompactDefaultHeight();
    void elidesHostNamesWithoutChangingValue();
private:
    QTranslator _translator;
};

///
/// \brief Exercises the translated interface with the production style when available.
///
void TestTlsSettings::initTestCase()
{
    QVERIFY(_translator.load(QFINDTESTDATA("../app/translations/omodsim_ru.qm")));
    qApp->installTranslator(&_translator);
#if defined(HAVE_QLEMENTINE_APP_STYLE)
#ifdef Q_OS_MACOS
    qApp->setStyle(new MacAppStyle());
#else
    qApp->setStyle(new QlementineAppStyle());
#endif
#endif
}

///
/// \brief Saves TLS paths and selects the standard secure port.
///
void TestTlsSettings::savesTlsSettings()
{
    TcpConnectionParams params;
    DialogSelectServicePort dialog(params);
    dialog.findChild<QCheckBox*>("tlsGroup")->setChecked(true);
    QCOMPARE(dialog.findChild<NumericLineEdit*>("lineEditPort")->value<int>(), 802);
    dialog.accept();
    QVERIFY(params.TlsEnabled);
    QCOMPARE(params.ServicePort, quint16(802));

    DialogSelectServicePort reopened(params);
    QVERIFY(reopened.findChild<QCheckBox*>("tlsGroup")->isChecked());
    QVERIFY(!dialog.findChild<QLineEdit*>("certificatePath"));
    reopened.findChild<QCheckBox*>("tlsGroup")->setChecked(false);
    QCOMPARE(reopened.findChild<NumericLineEdit*>("lineEditPort")->value<int>(), 502);
}

///
/// \brief Keeps a user supplied port when TLS is toggled.
///
void TestTlsSettings::preservesCustomPorts()
{
    TcpConnectionParams params;
    params.ServicePort = 1502;
    DialogSelectServicePort dialog(params);
    dialog.findChild<QCheckBox*>("tlsGroup")->setChecked(true);
    QCOMPARE(dialog.findChild<NumericLineEdit*>("lineEditPort")->value<int>(), 1502);
    dialog.findChild<QCheckBox*>("tlsGroup")->setChecked(false);
    QCOMPARE(dialog.findChild<NumericLineEdit*>("lineEditPort")->value<int>(), 1502);
}

///
/// \brief Hides TLS options for the RTU over TCP transport.
///
void TestTlsSettings::excludesTlsFromRtuTcp()
{
    TcpConnectionParams params;
    params.TlsEnabled = true;
    DialogSelectServicePort dialog(params, nullptr, false);
    QVERIFY(dialog.findChild<QCheckBox*>("tlsGroup")->isHidden());
    dialog.accept();
    QVERIFY(!params.TlsEnabled);
}

///
/// \brief Leaves connection settings unchanged when editing is cancelled.
///
void TestTlsSettings::cancelPreservesSettings()
{
    TcpConnectionParams params;
    const auto original = params;
    DialogSelectServicePort dialog(params);
    dialog.findChild<QCheckBox*>("tlsGroup")->setChecked(true);
    dialog.reject();
    QCOMPARE(params, original);
}

///
/// \brief Generates the shared identity from the certificate settings page.
///
void TestTlsSettings::generatesServerCertificateInSettings()
{
    if (!QSslSocket::supportsSsl())
        QSKIP("Qt TLS backend is unavailable");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TlsCertificatesWidget widget(nullptr, directory.path());
    auto* exportButton = widget.findChild<QPushButton*>("exportButton");
    QVERIFY(!exportButton->isEnabled());
    widget.findChild<QLineEdit*>("commonName")->setText("Modbus test server");
    widget.findChild<QLineEdit*>("hostNames")->setText("localhost, 127.0.0.1, ::1, server-with-a-long-name.example.test");
    widget.findChild<QPushButton*>("generateButton")->click();
    const auto certificate = TlsCertificateStore(directory.path()).serverCertificate();
    QVERIFY(!certificate.isNull());
    QCOMPARE(certificate.subjectInfo(QSslCertificate::CommonName), QStringList{"Modbus test server"});
    QVERIFY(certificate.subjectAlternativeNames().values(QSsl::DnsEntry).contains("server-with-a-long-name.example.test"));
    QVERIFY(exportButton->isEnabled());
    QVERIFY(widget.findChild<QPlainTextEdit*>("serverDetails")->toPlainText().contains("Modbus test server"));
    QVERIFY(!widget.findChild<QPlainTextEdit*>("serverDetails")->toPlainText().contains("PRIVATE KEY"));
}

///
/// \brief Exposes the shared certificate page through Modbus Definitions.
///
void TestTlsSettings::offersCertificatesTab()
{
    ModbusMultiServer server;
    DialogModbusDefinitions dialog(server);
    auto* tabs = dialog.findChild<QTabWidget*>("tabWidget");
    auto* page = dialog.findChild<TlsCertificatesWidget*>();
    QVERIFY(page);
    auto* tab = dialog.findChild<QWidget*>("tabCertificates");
    QVERIFY(tabs->indexOf(tab) >= 0);
    QVERIFY(tab->isAncestorOf(page));
    QVERIFY(page->findChild<QGroupBox*>("serverGroup")->title().isEmpty());
    QVERIFY(!page->findChild<QWidget*>("changesHint"));
    QVERIFY(!page->findChild<QTableWidget*>("trustedCertificates"));
    QVERIFY(!page->findChild<QPushButton*>("importButton"));
    QVERIFY(!page->findChild<QPushButton*>("removeButton"));
}

///
/// \brief Covers normal and enlarged text on a height constrained display.
///
void TestTlsSettings::keepsCertificateControlsAccessible_data()
{
    QTest::addColumn<int>("fontSize");
    QTest::newRow("normal-text") << 12;
    QTest::newRow("large-text") << 18;
}

///
/// \brief Keeps dialog actions visible while every certificate control can be scrolled into view.
///
void TestTlsSettings::keepsCertificateControlsAccessible()
{
    QFETCH(int, fontSize);
    ModbusMultiServer server;
    DialogModbusDefinitions dialog(server);
    QFont font = dialog.font();
    font.setPointSize(fontSize);
    dialog.setFont(font);
    dialog.setMaximumHeight(420);
    auto* tabs = dialog.findChild<QTabWidget*>("tabWidget");
    tabs->setCurrentWidget(dialog.findChild<QWidget*>("tabCertificates"));
    dialog.show();
    QCoreApplication::processEvents();
    QVERIFY(dialog.height() <= 420);
    QVERIFY(dialog.height() <= dialog.screen()->availableGeometry().height() - 64);
    auto* buttons = dialog.findChild<QDialogButtonBox*>("buttonBox");
    for (auto standard : {QDialogButtonBox::Ok, QDialogButtonBox::Cancel, QDialogButtonBox::Apply}) {
        auto* button = buttons->button(standard);
        QVERIFY(dialog.rect().contains(QRect(button->mapTo(&dialog, QPoint()), button->size())));
    }
    auto* scroll = dialog.findChild<QScrollArea*>("certificateScrollArea");
    QVERIFY(scroll);
    for (auto name : {"generateButton", "exportButton"}) {
        auto* button = dialog.findChild<QPushButton*>(name);
        scroll->ensureWidgetVisible(button);
        QCoreApplication::processEvents();
        const QRect visible(button->mapTo(scroll->viewport(), QPoint()), button->size());
        QVERIFY2(scroll->viewport()->rect().contains(visible), name);
    }
}

///
/// \brief Checks spacing with normal and enlarged translated text.
///
void TestTlsSettings::keepsSettingsSpacingCompact_data()
{
    QTest::addColumn<int>("fontSize");
    QTest::newRow("normal-text") << 12;
    QTest::newRow("large-text") << 18;
}

///
/// \brief Prevents nested page margins and spare dialog height from separating related controls.
///
void TestTlsSettings::keepsSettingsSpacingCompact()
{
    QFETCH(int, fontSize);
    ModbusMultiServer server;
    DialogModbusDefinitions dialog(server);
    QFont font = dialog.font();
    font.setPointSize(fontSize);
    dialog.setFont(font);
    auto* tabs = dialog.findChild<QTabWidget*>("tabWidget");
    auto* certificateTab = dialog.findChild<QWidget*>("tabCertificates");
    tabs->setCurrentWidget(certificateTab);
    dialog.show();
    QCoreApplication::processEvents();
    auto* group = dialog.findChild<QWidget*>("serverGroup");
    const QRect groupRect(group->mapTo(certificateTab, QPoint()), group->size());
    QVERIFY(groupRect.left() <= 12);
    QVERIFY(certificateTab->width() - groupRect.right() - 1 <= 12);
    auto* details = dialog.findChild<QPlainTextEdit*>("serverDetails");
    const QRect detailsRect(details->mapTo(group, QPoint()), details->size());
    QVERIFY(detailsRect.left() <= 20);
    QVERIFY(group->width() - detailsRect.right() - 1 <= 20);
    auto* validity = dialog.findChild<QWidget*>("validDays");
    auto* generate = dialog.findChild<QPushButton*>("generateButton");
    const int actionGap = generate->mapTo(group, QPoint()).y()
        - validity->mapTo(group, QPoint(0, validity->height())).y();
    QVERIFY(actionGap <= 24);

    tabs->setCurrentWidget(dialog.findChild<QWidget*>("tabErrSimulations"));
    QCoreApplication::processEvents();
    QCheckBox* previous = nullptr;
    for (auto name : {"checkBoxErrIncorrentId", "checkBoxIllegalFunc", "checkBoxBusy", "checkBoxCrcErr",
                      "checkBoxDelay", "checkBoxRandomDelay", "checkBoxNoResponse"}) {
        auto* checkbox = dialog.findChild<QCheckBox*>(name);
        if (previous) {
            const int gap = checkbox->y() - previous->y() - previous->height();
            QVERIFY2(gap >= 0 && gap <= 16, name);
        }
        previous = checkbox;
    }
}

///
/// \brief Avoids reserved blank space while keeping the tab controls and dialog actions visible.
///
void TestTlsSettings::usesCompactDefaultHeight()
{
    ModbusMultiServer server;
    DialogModbusDefinitions dialog(server);
    dialog.show();
    QCoreApplication::processEvents();
    QVERIFY(dialog.height() < 480);
    auto* certificateTab = dialog.findChild<QWidget*>("tabCertificates");
    dialog.findChild<QTabWidget*>("tabWidget")->setCurrentWidget(certificateTab);
    QCoreApplication::processEvents();
    auto* scroll = dialog.findChild<QScrollArea*>("certificateScrollArea");
    QCOMPARE(scroll->verticalScrollBar()->maximum(), 0);
    auto* tabs = dialog.findChild<QTabWidget*>("tabWidget");
    auto* errorTab = dialog.findChild<QWidget*>("tabErrSimulations");
    tabs->setCurrentWidget(errorTab);
    QCoreApplication::processEvents();
    auto* lastOption = dialog.findChild<QCheckBox*>("checkBoxNoResponse");
    QVERIFY(errorTab->rect().contains(lastOption->geometry()));
    auto* buttons = dialog.findChild<QDialogButtonBox*>("buttonBox");
    QVERIFY(dialog.rect().contains(QRect(buttons->mapTo(&dialog, QPoint()), buttons->size())));
}

///
/// \brief Abbreviates long host lists while preserving their full value and normal editing history.
///
void TestTlsSettings::elidesHostNamesWithoutChangingValue()
{
    QTemporaryDir directory;
    TlsCertificatesWidget widget(nullptr, directory.path());
    widget.resize(460, 340);
    auto* hosts = widget.findChild<ElidingLineEdit*>("hostNames");
    QVERIFY(hosts);
    const QString full = "controller-with-a-long-hostname.example.test, localhost, 127.0.0.1, ::1";
    hosts->setText(full);
    widget.show();
    auto* name = widget.findChild<QLineEdit*>("commonName");
    name->setFocus();
    QCoreApplication::processEvents();
    QVERIFY(!hosts->hasFocus());
    QVERIFY(hosts->visibleText().endsWith(QChar(0x2026)));
    QVERIFY(hosts->visibleText().startsWith("controller"));
    QCOMPARE(hosts->text(), full);
    hosts->setFocus();
    QTRY_VERIFY(hosts->hasFocus());
    QCOMPARE(hosts->visibleText(), full);
    hosts->setCursorPosition(full.size());
    QTest::keyClick(hosts, Qt::Key_X);
    QCOMPARE(hosts->text(), full + "x");
    name->setFocus();
    QCoreApplication::processEvents();
    hosts->undo();
    QCOMPARE(hosts->text(), full);
    hosts->setText("localhost");
    QCOMPARE(hosts->visibleText(), QString("localhost"));
}

///
/// \brief Runs the layout tests with the application theme service.
///
int main(int argc, char** argv)
{
    Application app(argc, argv);
    TestTlsSettings test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_tlssettings.moc"
