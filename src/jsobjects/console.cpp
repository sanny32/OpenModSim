#include <QTextCharFormat>
#include "console.h"

///
/// \brief console::console
/// \param console
///
console::console(ConsoleOutput* console)
    : _console(console)
{
}

///
/// \brief console::clear
///
void console::clear()
{
    _console->clear();
}

///
/// \brief console::log
/// \param msg
///
void console::log(const QString& msg)
{
    _console->addText(msg, Qt::black);
}

///
/// \brief console::debug
/// \param msg
///
void console::debug(const QString& msg)
{
    log(msg);
}

///
/// \brief console::warning
/// \param msg
///
void console::warning(const QString& msg)
{
    _console->addText(msg, "#F7C600");
}

///
/// \brief console::error
/// \param msg
///
void console::error(const QString& msg)
{
    _console->addText(msg, Qt::red);
}

