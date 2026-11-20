// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectxmlanchor.cpp
/// \brief Implements the element identity used to match project XML elements across a save.
///

#include "projectxmlanchor.h"

namespace {

// Control characters are illegal in XML 1.0 content, so they cannot occur inside an
// element name or attribute value and need no escaping when used as separators.
constexpr QLatin1Char kFieldSeparator('\x1f');
constexpr QLatin1Char kPathSeparator('\x1e');
constexpr QLatin1Char kAnchorSeparator('\x1d');

///
/// \brief identifyingAttributes lists the attributes that distinguish one sibling from another.
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

}

///
/// \brief elementSignature builds the identity of the element the reader is positioned on.
/// \param xml The reader positioned on a start element.
/// \return The element identity.
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
/// \brief identifyingAttributesOf returns the subset of the element's attributes that
/// elementSignature is built from.
/// \param xml The reader positioned on a start element.
/// \return The identifying attributes.
///
QXmlStreamAttributes identifyingAttributesOf(const QXmlStreamReader& xml)
{
    QXmlStreamAttributes result;
    const auto attributes = xml.attributes();
    for (const auto& name : identifyingAttributes()) {
        if (attributes.hasAttribute(name))
            result.append(name, attributes.value(name).toString());
    }
    return result;
}

///
/// \brief projectXmlAnchorKey flattens a location into a lookup key.
/// \param path Parent element names from the root.
/// \param signature Signature of the element itself.
/// \param occurrence Index among siblings sharing the signature.
/// \return The key identifying the location.
///
QString projectXmlAnchorKey(const QStringList& path, const QString& signature, int occurrence)
{
    QString key = path.join(kPathSeparator);
    key += kAnchorSeparator;
    key += signature;
    key += kAnchorSeparator;
    key += QString::number(occurrence);
    return key;
}
