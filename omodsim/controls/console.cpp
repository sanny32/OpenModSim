#include "console.h"

///
/// \brief Console::Console
/// \param parent
///
Console::Console(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setBackgroundColor(Qt::white);
}

///
/// \brief Console::clear
///
void Console::clear()
{
    setPlainText(QString());
}

///
/// \brief Console::log
/// \param msg
///
void Console::log(QString msg)
{
    const auto text = toPlainText();
    if(text.isEmpty())
    {
        setPlainText(msg);
    }
    else
    {
        setPlainText(QString("%1\n%2").arg(text, msg));
    }
}

///
/// \brief Console::setBackgroundColor
/// \param clr
///
void Console::setBackgroundColor(const QColor& clr)
{
    auto pal = palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    setPalette(pal);
}
