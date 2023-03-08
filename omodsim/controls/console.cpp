#include "console.h"

///
/// \brief Console::Console
/// \param parent
///
Console::Console(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setBackgroundColor(Qt::white);
    setFont(QFont("Fira Code"));
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
