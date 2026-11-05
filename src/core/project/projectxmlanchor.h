// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectxmlanchor.h
/// \brief Declares the element identity used to match project XML elements across a
/// save, when the document is regenerated in a canonical order.
///

#ifndef PROJECTXMLANCHOR_H
#define PROJECTXMLANCHOR_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>

///
/// \brief elementSignature builds the identity of the element the reader is positioned on:
/// its name followed by the attributes that distinguish it from its siblings.
///
/// Attributes carrying mutable state are deliberately left out, since an identity built
/// from them would break as soon as the data changes: the root Version, window geometry
/// and cursor positions, and Panel, which saving rewrites to the left panel for every
/// closed form.
///
/// \param xml The reader positioned on a start element.
/// \return The element identity.
///
QString elementSignature(const QXmlStreamReader& xml);

///
/// \brief identifyingAttributesOf returns the subset of the element's attributes that
/// elementSignature is built from, for mirroring the element into another document.
/// \param xml The reader positioned on a start element.
/// \return The identifying attributes.
///
QXmlStreamAttributes identifyingAttributesOf(const QXmlStreamReader& xml);

///
/// \brief The ProjectXmlWalk struct keeps the bookkeeping shared by every pass over a
/// project document: the current element path and, per level, how many siblings of each
/// signature were seen. Both passes of a split or a merge must walk identically for the
/// resulting keys to line up.
///
struct ProjectXmlWalk
{
    QStringList Path;
    QList<QHash<QString, int>> Counters{ QHash<QString, int>() };

    ///
    /// \brief nextOccurrence consumes the index of the next sibling with this signature.
    ///
    int nextOccurrence(const QString& signature)
    {
        return Counters.last()[signature]++;
    }

    ///
    /// \brief enter descends into the element. The path is built from signatures, not
    /// names: sibling forms share an element name, so a path of names cannot tell the
    /// children of one form from the children of the next.
    ///
    void enter(const QString& signature)
    {
        Path.append(signature);
        Counters.append(QHash<QString, int>());
    }

    ///
    /// \brief leave returns to the parent element.
    ///
    void leave()
    {
        Path.removeLast();
        Counters.removeLast();
    }
};

///
/// \brief projectXmlAnchorKey flattens a location into a lookup key.
/// \param path Parent element names from the root.
/// \param signature Signature of the element itself.
/// \param occurrence Index among siblings sharing the signature.
/// \return The key identifying the location.
///
QString projectXmlAnchorKey(const QStringList& path, const QString& signature, int occurrence);

#endif // PROJECTXMLANCHOR_H
