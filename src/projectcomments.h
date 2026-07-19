// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectcomments.h
/// \brief Declares collection and re-injection of XML comments across a project round-trip.
///

#ifndef PROJECTCOMMENTS_H
#define PROJECTCOMMENTS_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

///
/// \brief The ProjectCommentAnchor struct locates a comment by the element that
/// follows it rather than by its position, because saving regenerates the document
/// in a canonical element order that need not match the loaded file.
///
struct ProjectCommentAnchor
{
    QStringList Path;    ///< Parent element names from the root, empty at document level.
    QString Signature;   ///< Signature of the following sibling; empty means end of parent.
    int Occurrence = 0;  ///< Index among siblings sharing the signature.
};

///
/// \brief The ProjectComment struct is one comment together with its anchor.
///
struct ProjectComment
{
    ProjectCommentAnchor Anchor;
    QString Text;
};

using ProjectComments = QList<ProjectComment>;

///
/// \brief collectProjectComments extracts the comments of a project document,
/// anchoring each to the element that follows it.
/// \param document The raw project XML.
/// \return The collected comments in document order, empty when the XML is malformed.
///
ProjectComments collectProjectComments(const QByteArray& document);

///
/// \brief injectProjectComments rewrites a generated project document, re-inserting
/// the comments whose anchors still resolve. Comments anchored to elements that are
/// no longer written are dropped.
/// \param document The generated project XML.
/// \param comments The comments collected on load.
/// \return The document with comments, or the input unchanged on error.
///
QByteArray injectProjectComments(const QByteArray& document, const ProjectComments& comments);

#endif // PROJECTCOMMENTS_H
