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
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 2);

    connect(this, &QPlainTextEdit::textChanged, this, &Console::on_textChanged);
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

///
/// \brief Console::on_textChanged
///
void Console::on_textChanged()
{
    moveCursor(QTextCursor::End);
}
