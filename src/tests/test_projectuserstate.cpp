// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_projectuserstate.cpp
/// \brief Unit tests for splitting project documents into project data and user state.
///

#include <QTest>
#include <QXmlStreamReader>

#include "projectuserstate.h"

class TestProjectUserState : public QObject
{
    Q_OBJECT

private slots:
    void derivesUserStatePath();
    void movesWholeUserElements();
    void movesFusedAttributes();
    void keepsProjectDataInProjectDocument();
    void roundTripsThroughSplitAndMerge();
    void writesNoUserDocumentWithoutUserState();
    void mergeWithoutUserDocumentIsIdentity();
    void mergeReplacesInlineStateOfLegacyDocument();
    void keepsFormsApartByTitle();
    void preservesCdataAndComments();
    void returnsProjectUnchangedOnMalformedUserDocument();
    void ignoresUserStateForFormMissingFromProject();
    void leavesFormWithoutUserStateAtDefaults();
    void ignoresUserStateOfRenamedForm();
    void keepsSeveralFormsOfTheSameKindApart();
};

namespace {

const char* kProject =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<OpenModSim Version=\"2.0\">"
    "<ModbusDefinitions AddrSpace=\"6-Digits\"/>"
    "<ViewSettings ViewMode=\"1\" SplitView=\"0\" ActivePanel=\"L\"/>"
    "<Forms>"
    "<FormDataView Panel=\"L\" Title=\"Data1\" DataType=\"UInt16\">"
    "<Window Maximized=\"true\" Left=\"10\" Top=\"20\" Width=\"600\" Height=\"300\"/>"
    "<DataViewDefinitions DeviceId=\"1\" PointType=\"HoldingRegisters\" PointAddress=\"1\" Length=\"8\""
    " DataViewColumnsDistance=\"25\" LeadingZeros=\"true\"/>"
    "</FormDataView>"
    "<FormScriptView Panel=\"L\" Title=\"Script1\">"
    "<Window Maximized=\"false\" Left=\"5\" Top=\"5\" Width=\"400\" Height=\"200\"/>"
    "<Zoom Value=\"120%\"/>"
    "<JScriptControl><Script CursorPosition=\"42\" ScrollPosition=\"7\">"
    "<![CDATA[var x = 1 < 2;]]>"
    "</Script></JScriptControl>"
    "</FormScriptView>"
    "</Forms>"
    "<TabOrder Panel=\"L\"><TabRef title=\"Data1\"/></TabOrder>"
    "</OpenModSim>";

///
/// \brief Collects every element name of a document.
/// \param document The XML to inspect.
/// \return The element names in document order.
///
QStringList elementNames(const QByteArray& document)
{
    QStringList names;
    QXmlStreamReader xml(document);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement)
            names.append(xml.name().toString());
    }
    return names;
}

///
/// \brief Reads one attribute of the first element with the given name.
/// \param document The XML to inspect.
/// \param element The element name.
/// \param attribute The attribute name.
/// \return The attribute value, or a null string.
///
QString attributeOf(const QByteArray& document, const QString& element, const QString& attribute)
{
    QXmlStreamReader xml(document);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == element)
            return xml.attributes().value(attribute).toString();
    }
    return {};
}

}

/// \brief Verifies the user-state file sits next to the project file.
void TestProjectUserState::derivesUserStatePath()
{
    QCOMPARE(projectUserStatePath(QStringLiteral("/tmp/pump.omsim")),
             QStringLiteral("/tmp/.pump.omsim.user"));
    QCOMPARE(projectUserStatePath(QStringLiteral("pump.omsim")),
             QStringLiteral(".pump.omsim.user"));
}

/// \brief Verifies window geometry, view settings, tab order and zoom leave the project file.
void TestProjectUserState::movesWholeUserElements()
{
    const auto split = splitProjectUserState(kProject);
    const auto projectNames = elementNames(split.Project);
    const auto userNames = elementNames(split.User);

    for (const auto& name : { "Window", "ViewSettings", "TabOrder", "Zoom" }) {
        QVERIFY2(!projectNames.contains(QLatin1String(name)), name);
        QVERIFY2(userNames.contains(QLatin1String(name)), name);
    }
    QCOMPARE(userNames.count(QStringLiteral("Window")), 2);
}

/// \brief Verifies attributes sharing an element with project data are moved on their own.
void TestProjectUserState::movesFusedAttributes()
{
    const auto split = splitProjectUserState(kProject);

    QCOMPARE(attributeOf(split.Project, QStringLiteral("Script"), QStringLiteral("CursorPosition")), QString());
    QCOMPARE(attributeOf(split.User, QStringLiteral("Script"), QStringLiteral("CursorPosition")), QStringLiteral("42"));
    QCOMPARE(attributeOf(split.User, QStringLiteral("Script"), QStringLiteral("ScrollPosition")), QStringLiteral("7"));

    QCOMPARE(attributeOf(split.Project, QStringLiteral("DataViewDefinitions"), QStringLiteral("DataViewColumnsDistance")), QString());
    QCOMPARE(attributeOf(split.User, QStringLiteral("DataViewDefinitions"), QStringLiteral("DataViewColumnsDistance")), QStringLiteral("25"));
    QCOMPARE(attributeOf(split.User, QStringLiteral("DataViewDefinitions"), QStringLiteral("LeadingZeros")), QStringLiteral("true"));
}

/// \brief Verifies the project half keeps everything that is not user state.
void TestProjectUserState::keepsProjectDataInProjectDocument()
{
    const auto split = splitProjectUserState(kProject);
    const auto names = elementNames(split.Project);

    QVERIFY(names.contains(QStringLiteral("ModbusDefinitions")));
    QVERIFY(names.contains(QStringLiteral("FormDataView")));
    QVERIFY(names.contains(QStringLiteral("DataViewDefinitions")));
    QCOMPARE(attributeOf(split.Project, QStringLiteral("DataViewDefinitions"), QStringLiteral("Length")), QStringLiteral("8"));
    QCOMPARE(attributeOf(split.Project, QStringLiteral("FormDataView"), QStringLiteral("DataType")), QStringLiteral("UInt16"));
}

/// \brief Verifies splitting and merging restores every value the reader needs.
void TestProjectUserState::roundTripsThroughSplitAndMerge()
{
    const auto split = splitProjectUserState(kProject);
    const auto merged = mergeProjectUserState(split.Project, split.User);

    QCOMPARE(attributeOf(merged, QStringLiteral("ViewSettings"), QStringLiteral("ActivePanel")), QStringLiteral("L"));
    QCOMPARE(attributeOf(merged, QStringLiteral("Window"), QStringLiteral("Left")), QStringLiteral("10"));
    QCOMPARE(attributeOf(merged, QStringLiteral("Zoom"), QStringLiteral("Value")), QStringLiteral("120%"));
    QCOMPARE(attributeOf(merged, QStringLiteral("Script"), QStringLiteral("CursorPosition")), QStringLiteral("42"));
    QCOMPARE(attributeOf(merged, QStringLiteral("DataViewDefinitions"), QStringLiteral("DataViewColumnsDistance")), QStringLiteral("25"));
    QCOMPARE(attributeOf(merged, QStringLiteral("TabOrder"), QStringLiteral("Panel")), QStringLiteral("L"));

    const auto names = elementNames(merged);
    QCOMPARE(names.count(QStringLiteral("Window")), 2);
    QCOMPARE(names.count(QStringLiteral("ViewSettings")), 1);
}

/// \brief Verifies ViewSettings lands before Forms, which the reader depends on.
void TestProjectUserState::keepsFormsApartByTitle()
{
    const auto split = splitProjectUserState(kProject);
    const auto merged = mergeProjectUserState(split.Project, split.User);
    const auto names = elementNames(merged);

    QVERIFY(names.indexOf(QStringLiteral("ViewSettings")) < names.indexOf(QStringLiteral("Forms")));

    // The second window belongs to the script form, not to the data form.
    QXmlStreamReader xml(merged);
    QString currentForm;
    QHash<QString, QString> widthByForm;
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;
        if (xml.name().toString().startsWith(QLatin1String("Form")))
            currentForm = xml.attributes().value(QStringLiteral("Title")).toString();
        else if (xml.name() == QLatin1String("Window"))
            widthByForm.insert(currentForm, xml.attributes().value(QStringLiteral("Width")).toString());
    }

    QCOMPARE(widthByForm.value(QStringLiteral("Data1")), QStringLiteral("600"));
    QCOMPARE(widthByForm.value(QStringLiteral("Script1")), QStringLiteral("400"));
}

/// \brief Verifies a project without any user state produces no user document.
void TestProjectUserState::writesNoUserDocumentWithoutUserState()
{
    const QByteArray bare =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<OpenModSim Version=\"2.0\"><ModbusDefinitions AddrSpace=\"6-Digits\"/></OpenModSim>";

    const auto split = splitProjectUserState(bare);
    QVERIFY(split.User.isEmpty());
    QVERIFY(elementNames(split.Project).contains(QStringLiteral("ModbusDefinitions")));
}

/// \brief Verifies a missing user document leaves the project untouched.
void TestProjectUserState::mergeWithoutUserDocumentIsIdentity()
{
    QCOMPARE(mergeProjectUserState(kProject, {}), QByteArray(kProject));
}

/// \brief Verifies a document written before the split, which still carries the state
/// inline, does not end up with both copies after merging.
void TestProjectUserState::mergeReplacesInlineStateOfLegacyDocument()
{
    const auto split = splitProjectUserState(kProject);

    // kProject is itself in the legacy shape: it still holds Window and ViewSettings.
    const auto merged = mergeProjectUserState(kProject, split.User);
    const auto names = elementNames(merged);

    QCOMPARE(names.count(QStringLiteral("Window")), 2);
    QCOMPARE(names.count(QStringLiteral("ViewSettings")), 1);
    QCOMPARE(names.count(QStringLiteral("TabOrder")), 1);
    QCOMPARE(attributeOf(merged, QStringLiteral("Window"), QStringLiteral("Left")), QStringLiteral("10"));
}

/// \brief Verifies script bodies and comments survive both passes.
void TestProjectUserState::preservesCdataAndComments()
{
    const QByteArray annotated =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<OpenModSim Version=\"2.0\"><Forms>"
        "<!-- generator -->"
        "<FormScriptView Panel=\"L\" Title=\"Script1\">"
        "<Window Left=\"1\"/>"
        "<JScriptControl><Script CursorPosition=\"3\"><![CDATA[if (a < b) { }]]></Script></JScriptControl>"
        "</FormScriptView></Forms></OpenModSim>";

    const auto split = splitProjectUserState(annotated);
    QVERIFY(split.Project.contains("<![CDATA[if (a < b) { }]]>"));
    QVERIFY(split.Project.contains("<!-- generator -->"));

    const auto merged = mergeProjectUserState(split.Project, split.User);
    QVERIFY(merged.contains("<![CDATA[if (a < b) { }]]>"));
    QVERIFY(merged.contains("<!-- generator -->"));
    QCOMPARE(attributeOf(merged, QStringLiteral("Script"), QStringLiteral("CursorPosition")), QStringLiteral("3"));
}

/// \brief Verifies a damaged user document costs the layout, not the project.
void TestProjectUserState::returnsProjectUnchangedOnMalformedUserDocument()
{
    const QByteArray broken = "<OpenModSimUser><Forms></OpenModSimUser>";
    QCOMPARE(mergeProjectUserState(kProject, broken), QByteArray(kProject));
}

/// \brief Verifies user state left over for a form the project no longer has is dropped
/// instead of reappearing as a stray element.
void TestProjectUserState::ignoresUserStateForFormMissingFromProject()
{
    const QByteArray project =
        "<OpenModSim><Forms>"
        "<FormDataView Panel=\"L\" Title=\"Data1\"/>"
        "</Forms></OpenModSim>";
    const QByteArray user =
        "<OpenModSimUser><Forms>"
        "<FormDataView Title=\"Data1\"><Window Width=\"600\"/></FormDataView>"
        "<FormDataView Title=\"Gone\"><Window Width=\"999\"/></FormDataView>"
        "</Forms></OpenModSimUser>";

    const auto merged = mergeProjectUserState(project, user);

    QCOMPARE(elementNames(merged).count(QStringLiteral("Window")), 1);
    QCOMPARE(attributeOf(merged, QStringLiteral("Window"), QStringLiteral("Width")), QStringLiteral("600"));
    QVERIFY(!merged.contains("999"));
    QVERIFY(!merged.contains("Gone"));
}

/// \brief Verifies a form the user state says nothing about is left untouched, so a project
/// gaining a window on another machine still opens.
void TestProjectUserState::leavesFormWithoutUserStateAtDefaults()
{
    const QByteArray project =
        "<OpenModSim><Forms>"
        "<FormDataView Panel=\"L\" Title=\"Data1\"/>"
        "<FormDataView Panel=\"L\" Title=\"Data2\"/>"
        "</Forms></OpenModSim>";
    const QByteArray user =
        "<OpenModSimUser><Forms>"
        "<FormDataView Title=\"Data2\"><Window Width=\"320\"/></FormDataView>"
        "</Forms></OpenModSimUser>";

    const auto merged = mergeProjectUserState(project, user);

    QXmlStreamReader xml(merged);
    QString currentForm;
    QHash<QString, QString> widthByForm;
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;
        if (xml.name() == QLatin1String("FormDataView"))
            currentForm = xml.attributes().value(QStringLiteral("Title")).toString();
        else if (xml.name() == QLatin1String("Window"))
            widthByForm.insert(currentForm, xml.attributes().value(QStringLiteral("Width")).toString());
    }

    QVERIFY(!widthByForm.contains(QStringLiteral("Data1")));
    QCOMPARE(widthByForm.value(QStringLiteral("Data2")), QStringLiteral("320"));
}

/// \brief Verifies a renamed form falls back to defaults rather than inheriting the
/// geometry of whichever form now sits in its place.
void TestProjectUserState::ignoresUserStateOfRenamedForm()
{
    const QByteArray project =
        "<OpenModSim><Forms>"
        "<FormDataView Panel=\"L\" Title=\"Renamed\"/>"
        "</Forms></OpenModSim>";
    const QByteArray user =
        "<OpenModSimUser><Forms>"
        "<FormDataView Title=\"OldName\"><Window Width=\"777\"/></FormDataView>"
        "</Forms></OpenModSimUser>";

    const auto merged = mergeProjectUserState(project, user);

    QVERIFY(!merged.contains("777"));
    QVERIFY(!elementNames(merged).contains(QStringLiteral("Window")));
}

/// \brief Verifies several forms of the same kind stay distinct. Their children share both
/// an element name and a signature, so an anchor path built from names collapsed all of
/// them onto one key: every form's state piled up on the first, and the merged document
/// repeated an attribute until the reader rejected it.
void TestProjectUserState::keepsSeveralFormsOfTheSameKindApart()
{
    const QByteArray project =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<OpenModSim Version=\"2.0\"><Forms>"
        "<FormDataView Panel=\"L\" Title=\"Setpoints\">"
        "<Window Width=\"100\"/>"
        "<DataViewDefinitions DeviceId=\"1\" PointType=\"HoldingRegisters\" Length=\"3\""
        " DataViewColumnsDistance=\"1\" LeadingZeros=\"true\"/>"
        "</FormDataView>"
        "<FormDataView Panel=\"L\" Title=\"ProcessValues\">"
        "<Window Width=\"200\"/>"
        "<DataViewDefinitions DeviceId=\"1\" PointType=\"InputRegisters\" Length=\"4\""
        " DataViewColumnsDistance=\"2\" LeadingZeros=\"false\"/>"
        "</FormDataView>"
        "<FormDataView Panel=\"L\" Title=\"Commands\">"
        "<Window Width=\"300\"/>"
        "<DataViewDefinitions DeviceId=\"1\" PointType=\"Coils\" Length=\"4\""
        " DataViewColumnsDistance=\"3\" LeadingZeros=\"true\"/>"
        "</FormDataView>"
        "</Forms></OpenModSim>";

    const auto split = splitProjectUserState(project);
    const auto merged = mergeProjectUserState(split.Project, split.User);

    QXmlStreamReader reader(merged);
    while (!reader.atEnd())
        reader.readNext();
    QVERIFY2(!reader.hasError(), qPrintable(reader.errorString()));

    QCOMPARE(elementNames(merged).count(QStringLiteral("Window")), 3);

    QXmlStreamReader xml(merged);
    QString form;
    QHash<QString, QString> widthByForm;
    QHash<QString, QString> distanceByForm;
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;
        if (xml.name() == QLatin1String("FormDataView"))
            form = xml.attributes().value(QStringLiteral("Title")).toString();
        else if (xml.name() == QLatin1String("Window"))
            widthByForm.insert(form, xml.attributes().value(QStringLiteral("Width")).toString());
        else if (xml.name() == QLatin1String("DataViewDefinitions"))
            distanceByForm.insert(form, xml.attributes().value(QStringLiteral("DataViewColumnsDistance")).toString());
    }

    QCOMPARE(widthByForm.value(QStringLiteral("Setpoints")), QStringLiteral("100"));
    QCOMPARE(widthByForm.value(QStringLiteral("ProcessValues")), QStringLiteral("200"));
    QCOMPARE(widthByForm.value(QStringLiteral("Commands")), QStringLiteral("300"));
    QCOMPARE(distanceByForm.value(QStringLiteral("Setpoints")), QStringLiteral("1"));
    QCOMPARE(distanceByForm.value(QStringLiteral("ProcessValues")), QStringLiteral("2"));
    QCOMPARE(distanceByForm.value(QStringLiteral("Commands")), QStringLiteral("3"));
}

QTEST_GUILESS_MAIN(TestProjectUserState)
#include "test_projectuserstate.moc"
