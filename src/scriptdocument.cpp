#include <QXmlStreamWriter>
#include "scriptdocument.h"
#include "enums.h"

///
/// \brief ScriptDocument::ScriptDocument
///
ScriptDocument::ScriptDocument(const QString& name, QObject* parent)
    : QObject(parent)
    , _name(name)
{
}

///
/// \brief ScriptDocument::setName
///
void ScriptDocument::setName(const QString& name)
{
    if (_name == name) return;
    _name = name;
    emit nameChanged(name);
}

///
/// \brief ScriptDocument::script
///
QString ScriptDocument::script() const
{
    return _document.toPlainText();
}

///
/// \brief ScriptDocument::setScript
///
void ScriptDocument::setScript(const QString& text)
{
    _document.setPlainText(text);
}

///
/// \brief ScriptDocument::setSettings
///
void ScriptDocument::setSettings(const ScriptSettings& ss)
{
    _settings = ss;
    emit settingsChanged(ss);
}

///
/// \brief operator <<  (XML write)
///
QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const ScriptDocument* doc)
{
    if (!doc) return xml;

    xml.writeStartElement("ScriptDocument");
    xml.writeAttribute("Name", doc->name());

    xml << doc->settings();

    xml.writeStartElement("Script");
    xml.writeAttribute("CursorPosition", QString::number(doc->cursorPosition()));
    xml.writeAttribute("ScrollPosition", QString::number(doc->scrollPosition()));
    xml.writeCDATA(doc->script());
    xml.writeEndElement(); // Script

    xml.writeEndElement(); // ScriptDocument
    return xml;
}

///
/// \brief operator >>  (XML read)
///
QXmlStreamReader& operator >>(QXmlStreamReader& xml, ScriptDocument* doc)
{
    if (!doc) return xml;

    if (xml.isStartElement() && xml.name() == QLatin1String("ScriptDocument")) {
        const auto attrs = xml.attributes();
        if (attrs.hasAttribute("Name"))
            doc->setName(attrs.value("Name").toString());

        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("ScriptSettings")) {
                ScriptSettings ss;
                const auto sattrs = xml.attributes();
                if (sattrs.hasAttribute("Mode"))
                    ss.Mode = enumFromString<RunMode>(sattrs.value("Mode").toString());
                if (sattrs.hasAttribute("Interval")) {
                    bool ok; const uint v = sattrs.value("Interval").toUInt(&ok);
                    if (ok) ss.Interval = v;
                }
                if (sattrs.hasAttribute("RunOnStartup"))
                    ss.RunOnStartup = stringToBool(sattrs.value("RunOnStartup").toString());
                ss.normalize();
                doc->setSettings(ss);
                xml.skipCurrentElement();
            }
            else if (xml.name() == QLatin1String("Script")) {
                const auto sattrs = xml.attributes();
                int cursorPos = -1, scrollPos = -1;
                if (sattrs.hasAttribute("CursorPosition"))
                    cursorPos = sattrs.value("CursorPosition").toInt();
                if (sattrs.hasAttribute("ScrollPosition"))
                    scrollPos = sattrs.value("ScrollPosition").toInt();
                const QString text = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                doc->setScript(text);
                if (cursorPos >= 0) doc->setCursorPosition(cursorPos);
                if (scrollPos >= 0) doc->setScrollPosition(scrollPos);
            }
            else {
                xml.skipCurrentElement();
            }
        }
    }

    return xml;
}
