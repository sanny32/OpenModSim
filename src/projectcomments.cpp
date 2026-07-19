// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectcomments.cpp
/// \brief Implements collection and re-injection of XML comments across a project round-trip.
///

#include <QHash>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "projectcomments.h"

namespace {

constexpr QLatin1Char kFieldSeparator('\x1f');
constexpr QLatin1Char kPathSeparator('\x1e');
constexpr QLatin1Char kAnchorSeparator('\x1d');

///
/// \brief identifyingAttributes lists the attributes that distinguish one sibling from
/// another. Attributes carrying mutable state are deliberately absent, since an anchor
/// built from them would break as soon as the data changes: the root Version, window
/// geometry and cursor positions, and Panel, which save() rewrites to the left panel for
/// every closed form.
///
const QStringList& identifyingAttributes()
{
    static const QStringList attributes = {
        QStringLiteral("DeviceId"),
        QStringLiteral("Type"),
        QStringLiteral("Address"),
        QStringLiteral("Title"),
        QStringLiteral("AutoClone"),
        QStringLiteral("ConnectionType"),
        QStringLiteral("title")
    };
    return attributes;
}

///
/// \brief elementSignature builds the identity of the element the reader is positioned on.
/// \param xml The reader positioned on a start element.
/// \return The element name followed by its identifying attributes.
///
QString elementSignature(const QXmlStreamReader& xml)
{
    QString signature = xml.name().toString();
    const auto attributes = xml.attributes();
    for (const auto& name : identifyingAttributes()) {
        if (!attributes.hasAttribute(name))
            continue;
        signature += kFieldSeparator;
        signature += name;
        signature += QLatin1Char('=');
        signature += attributes.value(name).toString();
    }
    return signature;
}

///
/// \brief anchorKey flattens an anchor into a lookup key.
/// \param anchor The anchor to flatten.
/// \return The key identifying the insertion point.
///
QString anchorKey(const ProjectCommentAnchor& anchor)
{
    QString key = anchor.Path.join(kPathSeparator);
    key += kAnchorSeparator;
    key += anchor.Signature;
    key += kAnchorSeparator;
    key += QString::number(anchor.Occurrence);
    return key;
}

///
/// \brief The DocumentWalk struct keeps the bookkeeping shared by both passes: the
/// current element path and, per level, how many siblings of each signature were seen.
///
struct DocumentWalk
{
    QStringList Path;
    QList<QHash<QString, int>> Counters{ QHash<QString, int>() };

    int nextOccurrence(const QString& signature)
    {
        return Counters.last()[signature]++;
    }

    void enter(const QString& name)
    {
        Path.append(name);
        Counters.append(QHash<QString, int>());
    }

    void leave()
    {
        Path.removeLast();
        Counters.removeLast();
    }
};

}

///
/// \brief collectProjectComments extracts the comments of a project document,
/// anchoring each to the element that follows it.
/// \param document The raw project XML.
/// \return The collected comments in document order, empty when the XML is malformed.
///
ProjectComments collectProjectComments(const QByteArray& document)
{
    ProjectComments comments;
    QXmlStreamReader xml(document);
    xml.setNamespaceProcessing(false);

    DocumentWalk walk;
    // Comments inside an element carrying text or CDATA are dropped: script bodies and
    // descriptions are read back with readElementText, which would not preserve them anyway.
    QList<bool> carriesText;
    QStringList pending;

    const auto flush = [&comments, &pending](const ProjectCommentAnchor& anchor) {
        for (const auto& text : pending)
            comments.append({ anchor, text });
        pending.clear();
    };

    while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::Comment:
            pending.append(xml.text().toString());
            break;

        case QXmlStreamReader::StartElement: {
            const auto signature = elementSignature(xml);
            flush({ walk.Path, signature, walk.nextOccurrence(signature) });
            walk.enter(xml.name().toString());
            carriesText.append(false);
            break;
        }

        case QXmlStreamReader::EndElement:
            if (carriesText.isEmpty() || carriesText.takeLast())
                pending.clear();
            else
                flush({ walk.Path, QString(), 0 });
            walk.leave();
            break;

        case QXmlStreamReader::Characters:
            if (!carriesText.isEmpty() && (xml.isCDATA() || !xml.isWhitespace()))
                carriesText.last() = true;
            break;

        default:
            break;
        }
    }

    if (xml.hasError())
        return {};

    flush({ QStringList(), QString(), 0 });
    return comments;
}

///
/// \brief injectProjectComments rewrites a generated project document, re-inserting
/// the comments whose anchors still resolve. Comments anchored to elements that are
/// no longer written are dropped.
/// \param document The generated project XML.
/// \param comments The comments collected on load.
/// \return The document with comments, or the input unchanged on error.
///
QByteArray injectProjectComments(const QByteArray& document, const ProjectComments& comments)
{
    if (comments.isEmpty())
        return document;

    QHash<QString, QStringList> byAnchor;
    for (const auto& comment : comments)
        byAnchor[anchorKey(comment.Anchor)].append(comment.Text);

    QByteArray output;
    QXmlStreamReader xml(document);
    xml.setNamespaceProcessing(false);
    QXmlStreamWriter w(&output);
    w.setAutoFormatting(true);

    DocumentWalk walk;

    const auto writeAnchored = [&byAnchor, &w](const ProjectCommentAnchor& anchor) {
        const auto it = byAnchor.constFind(anchorKey(anchor));
        if (it == byAnchor.constEnd())
            return;
        for (const auto& text : *it)
            w.writeComment(text);
    };

    while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::StartDocument:
            w.writeStartDocument();
            break;

        case QXmlStreamReader::EndDocument:
            writeAnchored({ QStringList(), QString(), 0 });
            w.writeEndDocument();
            break;

        case QXmlStreamReader::StartElement: {
            const auto signature = elementSignature(xml);
            writeAnchored({ walk.Path, signature, walk.nextOccurrence(signature) });
            w.writeStartElement(xml.name().toString());
            w.writeAttributes(xml.attributes());
            walk.enter(xml.name().toString());
            break;
        }

        case QXmlStreamReader::EndElement:
            writeAnchored({ walk.Path, QString(), 0 });
            w.writeEndElement();
            walk.leave();
            break;

        case QXmlStreamReader::Characters:
            if (xml.isCDATA())
                w.writeCDATA(xml.text().toString());
            else if (!xml.isWhitespace())
                w.writeCharacters(xml.text().toString());
            break;

        // Comment tokens are not forwarded: the input is a freshly generated document,
        // and echoing one would duplicate the copy the store re-inserts.
        case QXmlStreamReader::ProcessingInstruction:
            w.writeProcessingInstruction(xml.processingInstructionTarget().toString(),
                                         xml.processingInstructionData().toString());
            break;

        default:
            break;
        }
    }

    if (xml.hasError() || w.hasError())
        return document;

    return output;
}
