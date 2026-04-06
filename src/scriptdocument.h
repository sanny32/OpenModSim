#ifndef SCRIPTDOCUMENT_H
#define SCRIPTDOCUMENT_H

#include <QObject>
#include <QTextDocument>
#include <QXmlStreamReader>
#include "scriptsettings.h"
#include "enums.h"

///
/// \brief The ScriptDocument class
/// Model for an independent script (not bound to any FormModSim).
///
class ScriptDocument : public QObject
{
    Q_OBJECT
public:
    explicit ScriptDocument(const QString& name, QObject* parent = nullptr);

    QString name() const { return _name; }
    void setName(const QString& name);

    QString script() const;
    void setScript(const QString& text);
    QTextDocument* textDocument() { return _document; }

    ScriptSettings settings() const { return _settings; }
    void setSettings(const ScriptSettings& ss);

    ByteOrder byteOrder() const { return _byteOrder; }
    void setByteOrder(ByteOrder order) { _byteOrder = order; }

    AddressBase addressBase() const { return _addressBase; }
    void setAddressBase(AddressBase base) { _addressBase = base; }

    int cursorPosition() const { return _cursorPos; }
    void setCursorPosition(int pos) { _cursorPos = pos; }

    int scrollPosition() const { return _scrollPos; }
    void setScrollPosition(int pos) { _scrollPos = pos; }

    bool isModified() const { return _document->isModified(); }
    void setModified(bool modified) { _document->setModified(modified); }

signals:
    void nameChanged(const QString& name);
    void settingsChanged(const ScriptSettings& settings);

private:
    QString       _name;
    QTextDocument* _document;
    ScriptSettings _settings;
    ByteOrder _byteOrder = ByteOrder::Direct;
    AddressBase _addressBase = AddressBase::Base1;
    int _cursorPos = 0;
    int _scrollPos = 0;
};

QXmlStreamWriter& operator <<(QXmlStreamWriter& xml, const ScriptDocument* doc);
QXmlStreamReader& operator >>(QXmlStreamReader& xml, ScriptDocument* doc);

#endif // SCRIPTDOCUMENT_H

