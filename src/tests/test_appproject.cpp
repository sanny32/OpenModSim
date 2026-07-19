// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_appproject.cpp
/// \brief Integration tests for project form and split-view lifecycle management.
///

#include <QFile>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "appproject.h"
#include "apppreferences.h"
#include "application.h"
#include "controls/mdiareaex.h"
#include "controls/projecttreewidget.h"
#include "datasimulator.h"
#include "mainwindow.h"
#include "modbusmultiserver.h"
#include "projectformmetadata.h"

class TestAppProject : public QObject
{
    Q_OBJECT

private slots:
    void createsAndNumbersEveryFormKind();
    void closesReopensAndDeletesCanonicalForm();
    void respectsDeletionLock();
    void createsAndRemovesSplitClone();
    void roundTripsOpenAndClosedForms();
    void rejectsMalformedProjectWithoutChangingState();
    void loadsProjectWithChildlessDefinitionsAndConnection();
    void preservesXmlCommentsAcrossRoundTrip();
    void keepsCommentsOnRepeatedSave();
    void dropsCommentsAfterCloseProject();
    void omitsTimestampsWhenPreferenceDisabled();
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

/// \brief Verifies creation, UUID assignment and per-kind display counters.
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

int main(int argc, char** argv)
{
    Application app(argc, argv);
    TestAppProject test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_appproject.moc"
