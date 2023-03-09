#include "consoleoutput.h"

///
/// \brief ConsoleOutput::ConsoleOutput
/// \param parent
///
ConsoleOutput::ConsoleOutput(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setBackgroundColor(Qt::white);
    setFont(QFont("Fira Code"));
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 2);

    connect(this, &QPlainTextEdit::textChanged, this, &ConsoleOutput::on_textChanged);
}

///
/// \brief ConsoleOutput::setBackgroundColor
/// \param clr
///
void ConsoleOutput::setBackgroundColor(const QColor& clr)
{
    auto pal = palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    setPalette(pal);
}

///
/// \brief ConsoleOutput::on_textChanged
///
void ConsoleOutput::on_textChanged()
{
    moveCursor(QTextCursor::End);
}
