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

int main(int argc, char** argv)
{
    Application app(argc, argv);
    TestAppProject test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_appproject.moc"
