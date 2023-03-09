#include "console.h"

///
/// \brief Console::Console
/// \param doc
///
Console::Console(QTextDocument* doc)
    : _document(doc)
{
}

void Console::clear()
{
    _document->setPlainText(QString());
}

///
/// \brief Console::log
/// \param msg
///
void Console::log(const QString& msg)
{
    const auto text = _document->toPlainText();
    if(text.isEmpty())
    {
        _document->setPlainText(QString(">>\t%1").arg(msg));
    }
    else
    {
        _document->setPlainText(QString("%1\n>>\t%2").arg(text, msg));
    }
}
