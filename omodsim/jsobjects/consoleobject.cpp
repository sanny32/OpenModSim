#include "consoleobject.h"

///
/// \brief ConsoleObject::ConsoleObject
/// \param doc
///
ConsoleObject::ConsoleObject(QTextDocument* doc)
    : _document(doc)
{
}

void ConsoleObject::clear()
{
    _document->setPlainText(QString());
}

///
/// \brief Console::log
/// \param msg
///
void ConsoleObject::log(const QString& msg)
{
    const auto text = _document->toPlainText();
    if(text.isEmpty())
    {
        _document->setPlainText(msg);
    }
    else
    {
        _document->setPlainText(QString("%1\n%2").arg(text, msg));
    }
}
