// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectuserstate.cpp
/// \brief Implements the split of a project document into shared project data and
/// machine-local user state.
///

#include <QBuffer>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "projectuserstate.h"
#include "projectxmlanchor.h"

namespace {

constexpr const char* kProjectRoot = "OpenModSim";
constexpr const char* kUserRoot = "OpenModSimUser";

///
/// \brief userElements lists the elements whose whole subtree is user state.
///
const QSet<QString>& userElements()
{
    static const QSet<QString> elements = {
        QStringLiteral("ViewSettings"),
        QStringLiteral("TabOrder"),
        QStringLiteral("Window"),
        QStringLiteral("Colors"),
        QStringLiteral("Font"),
        QStringLiteral("Zoom")
    };
    return elements;
}

///
/// \brief userAttributes lists the user-state attributes of elements that otherwise carry
/// project data, keyed by the owning element name.
///
const QHash<QString, QStringList>& userAttributes()
{
    static const QHash<QString, QStringList> attributes = {
        { QStringLiteral("Script"), { QStringLiteral("CursorPosition"), QStringLiteral("ScrollPosition") } },
        { QStringLiteral("DataViewDefinitions"), { QStringLiteral("DataViewColumnsDistance"), QStringLiteral("LeadingZeros") } }
    };
    return attributes;
}

///
/// \brief isUserAttribute
/// \param element The owning element name.
/// \param attribute The attribute name.
/// \return True when the attribute belongs in the user-state document.
///
bool isUserAttribute(const QString& element, const QString& attribute)
{
    const auto it = userAttributes().constFind(element);
    return it != userAttributes().constEnd() && it->contains(attribute);
}

///
/// \brief copyCurrentElement copies the element the reader is positioned on, and its whole
/// subtree, to the writer. The reader is left on the closing tag of that element.
/// \param xml The reader positioned on a start element.
/// \param w The writer.
///
void copyCurrentElement(QXmlStreamReader& xml, QXmlStreamWriter& w)
{
    int depth = 0;
    for (;;) {
        switch (xml.tokenType()) {
        case QXmlStreamReader::StartElement:
            w.writeStartElement(xml.name().toString());
            w.writeAttributes(xml.attributes());
            depth++;
            break;
        case QXmlStreamReader::EndElement:
            w.writeEndElement();
            depth--;
            break;
        case QXmlStreamReader::Characters:
            if (xml.isCDATA())
                w.writeCDATA(xml.text().toString());
            else if (!xml.isWhitespace())
                w.writeCharacters(xml.text().toString());
            break;
        case QXmlStreamReader::Comment:
            w.writeComment(xml.text().toString());
            break;
        default:
            break;
        }

        if (depth == 0 || xml.atEnd())
            return;
        xml.readNext();
    }
}

///
/// \brief writeFragment streams a stored element fragment into the writer.
/// \param fragment The serialized element.
/// \param w The writer.
///
void writeFragment(const QByteArray& fragment, QXmlStreamWriter& w)
{
    QXmlStreamReader xml(fragment);
    xml.setNamespaceProcessing(false);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            copyCurrentElement(xml, w);
            return;
        }
    }
}

///
/// \brief The UserStateIndex struct is a user-state document turned into lookups keyed by
/// the anchor of the project element the state belongs to.
///
struct UserStateIndex
{
    QHash<QString, QList<QByteArray>> Children;   ///< Whole elements to insert into that element.
    QHash<QString, QSet<QString>> ReplacedNames;  ///< Their names, to drop stale inline copies.
    QHash<QString, QList<QPair<QString, QString>>> Attributes;
    bool Valid = false;
};

///
/// \brief indexUserState walks a user-state document and records, per project anchor, the
/// elements and attributes it carries.
///
/// The user document mirrors only the branches that lead to stored state, so a sibling
/// without state is missing from it. Occurrence counters stay aligned because they are kept
/// per signature, and every element that can carry user state is identified by an attribute
/// its siblings do not share.
///
/// \param user The user-state document.
/// \return The index; invalid when the document is malformed.
///
UserStateIndex indexUserState(const QByteArray& user)
{
    UserStateIndex index;
    if (user.isEmpty()) {
        index.Valid = true;
        return index;
    }

    QXmlStreamReader xml(user);
    xml.setNamespaceProcessing(false);

    ProjectXmlWalk walk;
    QStringList anchors;

    while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::StartElement: {
            const bool isRoot = walk.Path.isEmpty();
            const auto name = isRoot ? QString::fromLatin1(kProjectRoot) : xml.name().toString();
            const auto signature = isRoot ? name : elementSignature(xml);
            const auto anchor = projectXmlAnchorKey(walk.Path, signature, walk.nextOccurrence(signature));

            if (!isRoot && userElements().contains(name)) {
                QByteArray fragment;
                QBuffer buffer(&fragment);
                buffer.open(QIODevice::WriteOnly);
                QXmlStreamWriter w(&buffer);
                copyCurrentElement(xml, w);
                buffer.close();

                const auto& owner = anchors.last();
                index.Children[owner].append(fragment);
                index.ReplacedNames[owner].insert(name);
                break;
            }

            for (const auto& attribute : xml.attributes()) {
                const auto attributeName = attribute.name().toString();
                if (isUserAttribute(name, attributeName))
                    index.Attributes[anchor].append({ attributeName, attribute.value().toString() });
            }

            walk.enter(signature);
            anchors.append(anchor);
            break;
        }

        case QXmlStreamReader::EndElement:
            if (!walk.Path.isEmpty()) {
                walk.leave();
                anchors.removeLast();
            }
            break;

        default:
            break;
        }
    }

    index.Valid = !xml.hasError();
    return index;
}

///
/// \brief The MirrorElement struct is an element of the project document that the user
/// document may have to mirror, so the state below it can be located again. It is written
/// out only once something underneath it actually needs storing.
///
struct MirrorElement
{
    QString Name;
    QXmlStreamAttributes Identity;
    QXmlStreamAttributes UserState;
    bool Emitted = false;
};

}

///
/// \brief projectUserStatePath derives the user-state file path of a project.
/// \param projectPath Path of the .omsim file.
/// \return The .omsim.user path.
///
QString projectUserStatePath(const QString& projectPath)
{
    const QFileInfo info(projectPath);
    const auto name = QLatin1Char('.') + info.fileName() + QStringLiteral(".user");
    const auto directory = info.path();
    if (directory.isEmpty() || directory == QLatin1String("."))
        return name;
    return directory + QLatin1Char('/') + name;
}

///
/// \brief splitProjectUserState divides a generated project document into the shared
/// project data and the user state.
/// \param document The generated project XML.
/// \return Both halves; the user half is empty when there is no user state to store.
///
ProjectDocuments splitProjectUserState(const QByteArray& document)
{
    ProjectDocuments result;

    QByteArray projectOut;
    QByteArray userOut;
    QXmlStreamReader xml(document);
    xml.setNamespaceProcessing(false);
    QXmlStreamWriter projectWriter(&projectOut);
    QXmlStreamWriter userWriter(&userOut);
    projectWriter.setAutoFormatting(true);
    userWriter.setAutoFormatting(true);

    QList<MirrorElement> mirrors;
    bool userHasContent = false;

    const auto flushMirrors = [&mirrors, &userWriter, &userHasContent]() {
        for (int i = 0; i < mirrors.size(); ++i) {
            auto& mirror = mirrors[i];
            if (mirror.Emitted)
                continue;
            userWriter.writeStartElement(i == 0 ? QString::fromLatin1(kUserRoot) : mirror.Name);
            userWriter.writeAttributes(mirror.Identity);
            userWriter.writeAttributes(mirror.UserState);
            mirror.Emitted = true;
            userHasContent = true;
        }
    };

    while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::StartDocument:
            projectWriter.writeStartDocument();
            userWriter.writeStartDocument();
            break;

        case QXmlStreamReader::EndDocument:
            projectWriter.writeEndDocument();
            if (userHasContent)
                userWriter.writeEndDocument();
            break;

        case QXmlStreamReader::StartElement: {
            const auto name = xml.name().toString();

            if (!mirrors.isEmpty() && userElements().contains(name)) {
                flushMirrors();
                copyCurrentElement(xml, userWriter);
                break;
            }

            QXmlStreamAttributes projectAttributes;
            QXmlStreamAttributes userState;
            for (const auto& attribute : xml.attributes()) {
                if (isUserAttribute(name, attribute.name().toString()))
                    userState.append(attribute);
                else
                    projectAttributes.append(attribute);
            }

            mirrors.append({ name, identifyingAttributesOf(xml), userState, false });
            if (!userState.isEmpty())
                flushMirrors();

            projectWriter.writeStartElement(name);
            projectWriter.writeAttributes(projectAttributes);
            break;
        }

        case QXmlStreamReader::EndElement: {
            if (mirrors.isEmpty())
                break;
            const auto mirror = mirrors.takeLast();
            if (mirror.Emitted)
                userWriter.writeEndElement();
            projectWriter.writeEndElement();
            break;
        }

        case QXmlStreamReader::Characters:
            if (xml.isCDATA())
                projectWriter.writeCDATA(xml.text().toString());
            else if (!xml.isWhitespace())
                projectWriter.writeCharacters(xml.text().toString());
            break;

        case QXmlStreamReader::Comment:
            projectWriter.writeComment(xml.text().toString());
            break;

        default:
            break;
        }
    }

    if (xml.hasError() || projectWriter.hasError() || userWriter.hasError()) {
        result.Project = document;
        return result;
    }

    result.Project = projectOut;
    result.User = userHasContent ? userOut : QByteArray();
    return result;
}

///
/// \brief mergeProjectUserState folds a user-state document back into a project document.
/// \param project The project document.
/// \param user The user-state document; may be empty.
/// \return The merged document, or the project document unchanged on error.
///
QByteArray mergeProjectUserState(const QByteArray& project, const QByteArray& user)
{
    if (user.isEmpty())
        return project;

    const auto index = indexUserState(user);
    if (!index.Valid)
        return project;

    QByteArray output;
    QXmlStreamReader xml(project);
    xml.setNamespaceProcessing(false);
    QXmlStreamWriter w(&output);
    w.setAutoFormatting(true);

    ProjectXmlWalk walk;
    QStringList anchors;

    while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::StartDocument:
            w.writeStartDocument();
            break;

        case QXmlStreamReader::EndDocument:
            w.writeEndDocument();
            break;

        case QXmlStreamReader::StartElement: {
            const auto name = xml.name().toString();
            const auto signature = elementSignature(xml);
            const auto anchor = projectXmlAnchorKey(walk.Path, signature, walk.nextOccurrence(signature));

            // A document written before the split still carries this element inline; the
            // user document is the newer copy, so the inline one is dropped.
            if (!anchors.isEmpty() && index.ReplacedNames.value(anchors.last()).contains(name)) {
                xml.skipCurrentElement();
                break;
            }

            const auto overrides = index.Attributes.value(anchor);
            QSet<QString> overridden;
            for (const auto& override : overrides)
                overridden.insert(override.first);

            w.writeStartElement(name);
            for (const auto& attribute : xml.attributes()) {
                if (!overridden.contains(attribute.name().toString()))
                    w.writeAttribute(attribute.name().toString(), attribute.value().toString());
            }
            for (const auto& override : overrides)
                w.writeAttribute(override.first, override.second);

            // Written first so that ViewSettings precedes Forms, which the project reader
            // relies on to know the view mode before it creates any window.
            for (const auto& fragment : index.Children.value(anchor))
                writeFragment(fragment, w);

            walk.enter(signature);
            anchors.append(anchor);
            break;
        }

        case QXmlStreamReader::EndElement:
            w.writeEndElement();
            walk.leave();
            anchors.removeLast();
            break;

        case QXmlStreamReader::Characters:
            if (xml.isCDATA())
                w.writeCDATA(xml.text().toString());
            else if (!xml.isWhitespace())
                w.writeCharacters(xml.text().toString());
            break;

        case QXmlStreamReader::Comment:
            w.writeComment(xml.text().toString());
            break;

        default:
            break;
        }
    }

    if (xml.hasError() || w.hasError())
        return project;

    return output;
}
