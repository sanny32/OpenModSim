// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_projectcomments.cpp
/// \brief Unit tests for collecting and re-injecting project XML comments.
///

#include <QTest>
#include <QXmlStreamReader>

#include "projectcomments.h"

class TestProjectComments : public QObject
{
    Q_OBJECT

private slots:
    void anchorsCommentsAtEveryPosition();
    void survivesElementReorder();
    void dropsCommentWithVanishedAnchor();
    void preservesConsecutiveCommentOrder();
    void distinguishesIdenticalSiblings();
    void preservesCdataSections();
    void dropsCommentInsideTextElement();
    void preservesUnicodeAndSpecialCharacters();
    void ignoresVolatileAttributes();
    void anchorsCommentInEmptyContainer();
    void returnsInputOnMalformedDocument();
    void emptyCommentsLeaveDocumentUntouched();
};

namespace {

///
/// \brief Describes where each comment of a document sits, as "parent path|following
/// element|text". The following element is "END" for a comment closing its parent.
/// \param document The XML to inspect.
/// \return One entry per comment, in document order.
///
QStringList commentPlacements(const QByteArray& document)
{
    QStringList placements;
    QStringList path;
    QStringList pending;
    QXmlStreamReader xml(document);

    const auto flush = [&placements, &pending, &path](const QString& following) {
        for (const auto& text : pending)
            placements.append(path.join(QLatin1Char('/')) + QLatin1Char('|') + following + QLatin1Char('|') + text);
        pending.clear();
    };

    while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::Comment:
            pending.append(xml.text().toString());
            break;
        case QXmlStreamReader::StartElement:
            flush(xml.name().toString());
            path.append(xml.name().toString());
            break;
        case QXmlStreamReader::EndElement:
            flush(QStringLiteral("END"));
            path.removeLast();
            break;
        default:
            break;
        }
    }
    flush(QStringLiteral("END"));

    return placements;
}

///
/// \brief Collects the comments of one document and injects them into another.
/// \param source The document the comments are taken from.
/// \param target The document they are written into.
/// \return The target with comments.
///
QByteArray transfer(const QByteArray& source, const QByteArray& target)
{
    return injectProjectComments(target, collectProjectComments(source));
}

}

/// \brief Verifies comments are anchored in the prolog, before an element, at the end
/// of a parent and in the epilog.
void TestProjectComments::anchorsCommentsAtEveryPosition()
{
    const QByteArray document =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--prolog-->"
        "<OpenModSim Version=\"2.0\">"
        "<!--before connections-->"
        "<Connections/>"
        "<!--end of root-->"
        "</OpenModSim>"
        "<!--epilog-->";

    const auto result = transfer(document, document);
    QCOMPARE(commentPlacements(result), commentPlacements(document));
    QCOMPARE(commentPlacements(result), QStringList({
        QStringLiteral("|OpenModSim|prolog"),
        QStringLiteral("OpenModSim|Connections|before connections"),
        QStringLiteral("OpenModSim|END|end of root"),
        QStringLiteral("|END|epilog")
    }));
}

/// \brief Verifies a comment follows its element when saving reorders the document.
void TestProjectComments::survivesElementReorder()
{
    const QByteArray source =
        "<OpenModSim>"
        "<!--connection settings-->"
        "<Connections/>"
        "<ModbusDefinitions/>"
        "</OpenModSim>";
    const QByteArray target =
        "<OpenModSim>"
        "<ModbusDefinitions/>"
        "<Connections/>"
        "</OpenModSim>";

    QCOMPARE(commentPlacements(transfer(source, target)), QStringList({
        QStringLiteral("OpenModSim|Connections|connection settings")
    }));
}

/// \brief Verifies a comment whose anchor is no longer written is dropped.
void TestProjectComments::dropsCommentWithVanishedAnchor()
{
    const QByteArray source =
        "<OpenModSim><Forms>"
        "<!--gone-->"
        "<FormDataView Title=\"Data9\"/>"
        "<!--kept-->"
        "<FormDataView Title=\"Data1\"/>"
        "</Forms></OpenModSim>";
    const QByteArray target =
        "<OpenModSim><Forms><FormDataView Title=\"Data1\"/></Forms></OpenModSim>";

    const auto result = transfer(source, target);
    QCOMPARE(commentPlacements(result), QStringList({
        QStringLiteral("OpenModSim/Forms|FormDataView|kept")
    }));

    QXmlStreamReader reader(result);
    while (!reader.atEnd())
        reader.readNext();
    QVERIFY(!reader.hasError());
}

/// \brief Verifies several comments sharing an anchor keep their order.
void TestProjectComments::preservesConsecutiveCommentOrder()
{
    const QByteArray document =
        "<OpenModSim><!--first--><!--second--><!--third--><Connections/></OpenModSim>";

    QCOMPARE(commentPlacements(transfer(document, document)), QStringList({
        QStringLiteral("OpenModSim|Connections|first"),
        QStringLiteral("OpenModSim|Connections|second"),
        QStringLiteral("OpenModSim|Connections|third")
    }));
}

/// \brief Verifies siblings with an identical signature are told apart by position.
void TestProjectComments::distinguishesIdenticalSiblings()
{
    const QByteArray document =
        "<OpenModSim><Connections>"
        "<ConnectionDetails ConnectionType=\"Tcp\"/>"
        "<!--the second one-->"
        "<ConnectionDetails ConnectionType=\"Tcp\"/>"
        "</Connections></OpenModSim>";

    const auto result = transfer(document, document);
    QCOMPARE(commentPlacements(result), commentPlacements(document));
    QCOMPARE(result.count("<!--the second one-->"), 1);
}

/// \brief Verifies the injection pass does not turn CDATA into escaped text.
void TestProjectComments::preservesCdataSections()
{
    const QByteArray document =
        "<OpenModSim><AddressSpace><AddressDescriptionMap>"
        "<!--wave outputs-->"
        "<Description DeviceId=\"1\" Type=\"4\" Address=\"0\"><![CDATA[Sine <fast> & loud]]></Description>"
        "</AddressDescriptionMap></AddressSpace></OpenModSim>";

    const auto result = transfer(document, document);
    QVERIFY(result.contains("<![CDATA[Sine <fast> & loud]]>"));
    QCOMPARE(commentPlacements(result), QStringList({
        QStringLiteral("OpenModSim/AddressSpace/AddressDescriptionMap|Description|wave outputs")
    }));
}

/// \brief Verifies comments inside a text-carrying element are dropped rather than
/// re-inserted next to its content.
void TestProjectComments::dropsCommentInsideTextElement()
{
    const QByteArray source =
        "<OpenModSim><Forms><FormScriptView Title=\"S1\"><Script>"
        "<!--lost--><![CDATA[var x = 1;]]>"
        "</Script></FormScriptView></Forms></OpenModSim>";
    const QByteArray target =
        "<OpenModSim><Forms><FormScriptView Title=\"S1\"><Script>"
        "<![CDATA[var x = 1;]]>"
        "</Script></FormScriptView></Forms></OpenModSim>";

    QVERIFY(collectProjectComments(source).isEmpty());

    const auto result = transfer(source, target);
    QVERIFY(result.contains("<![CDATA[var x = 1;]]>"));
    QVERIFY(commentPlacements(result).isEmpty());
}

/// \brief Verifies comment text survives Unicode and XML-significant characters.
void TestProjectComments::preservesUnicodeAndSpecialCharacters()
{
    const QString text = QStringLiteral(" Насос №1: 25 °C, a < b & c \"q\" ");
    const QByteArray document =
        "<OpenModSim><!--" + text.toUtf8() + "--><Connections/></OpenModSim>";

    const auto result = transfer(document, document);
    QCOMPARE(commentPlacements(result), QStringList({
        QStringLiteral("OpenModSim|Connections|") + text
    }));

    QXmlStreamReader reader(result);
    while (!reader.atEnd())
        reader.readNext();
    QVERIFY(!reader.hasError());
}

/// \brief Verifies anchors ignore attributes that change between sessions: the root
/// version, the panel a form sits on and its window geometry.
void TestProjectComments::ignoresVolatileAttributes()
{
    const QByteArray source =
        "<OpenModSim Version=\"1.9\"><Forms>"
        "<!--live values-->"
        "<FormDataView Panel=\"L\" Title=\"Data1\" Left=\"0\" Top=\"0\"/>"
        "</Forms></OpenModSim>";
    const QByteArray target =
        "<OpenModSim Version=\"2.0-dev\"><Forms>"
        "<FormDataView Panel=\"R\" Title=\"Data1\" Left=\"120\" Top=\"64\"/>"
        "</Forms></OpenModSim>";

    QCOMPARE(commentPlacements(transfer(source, target)), QStringList({
        QStringLiteral("OpenModSim/Forms|FormDataView|live values")
    }));
}

/// \brief Verifies a comment is the only content an otherwise empty container keeps.
void TestProjectComments::anchorsCommentInEmptyContainer()
{
    const QByteArray document =
        "<OpenModSim><AddressSpace><ModbusSimulationMap>"
        "<!--no simulations-->"
        "</ModbusSimulationMap></AddressSpace></OpenModSim>";

    QCOMPARE(commentPlacements(transfer(document, document)), QStringList({
        QStringLiteral("OpenModSim/AddressSpace/ModbusSimulationMap|END|no simulations")
    }));
}

/// \brief Verifies malformed input is passed through instead of producing a broken file.
void TestProjectComments::returnsInputOnMalformedDocument()
{
    const QByteArray broken = "<OpenModSim><Forms></OpenModSim>";
    QVERIFY(collectProjectComments(broken).isEmpty());

    const ProjectComments comments = collectProjectComments("<OpenModSim><!--c--><Forms/></OpenModSim>");
    QVERIFY(!comments.isEmpty());
    QCOMPARE(injectProjectComments(broken, comments), broken);
}

/// \brief Verifies an empty comment list leaves the document byte-identical.
void TestProjectComments::emptyCommentsLeaveDocumentUntouched()
{
    const QByteArray document = "<OpenModSim><Connections/></OpenModSim>";
    QCOMPARE(injectProjectComments(document, {}), document);
}

QTEST_GUILESS_MAIN(TestProjectComments)
#include "test_projectcomments.moc"
